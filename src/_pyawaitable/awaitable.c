#include <Python.h>
#include <stdlib.h>

#include <pyawaitable/array.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/coro.h>
#include <pyawaitable/genwrapper.h>

static void
callback_dealloc(void *ptr)
{
    assert(ptr != NULL);
    pyawaitable_callback *cb = (pyawaitable_callback *) ptr;
    Py_CLEAR(cb->coro);
    PyMem_Free(cb);
}

static PyObject *
awaitable_new_func(PyTypeObject *tp, PyObject *args, PyObject *kwds)
{
    assert(tp != NULL);
    assert(tp->tp_alloc != NULL);

    PyObject *self = tp->tp_alloc(tp, 0);
    if (self == NULL)
    {
        return NULL;
    }

    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    aw->aw_gen = NULL;
    aw->aw_done = false;
    aw->aw_state = 0;
    aw->aw_result = NULL;
    aw->aw_recently_cancelled = 0;

    if (pyawaitable_array_init(&aw->aw_callbacks, callback_dealloc) < 0)
    {
        goto error;
    }

    if (
        pyawaitable_array_init(
            &aw->aw_object_values,
            (pyawaitable_array_deallocator) Py_DecRef
        ) < 0
    )
    {
        goto error;
    }

    if (pyawaitable_array_init(&aw->aw_arbitrary_values, NULL) < 0)
    {
        goto error;
    }

    if (pyawaitable_array_init(&aw->aw_integer_values, NULL) < 0)
    {
        goto error;
    }

    return self;
error:
    PyErr_NoMemory();
    Py_DECREF(self);
    return NULL;
}

_PyAwaitable_INTERNAL(PyObject *)
awaitable_next(PyObject * self)
{
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    if (aw->aw_awaited)
    {
        PyErr_SetString(
            PyExc_RuntimeError,
            "pyawaitable: cannot reuse awaitable"
        );
        return NULL;
    }
    aw->aw_awaited = true;
    PyObject *gen = genwrapper_new(aw);
    aw->aw_gen = Py_XNewRef(gen);
    return gen;
}

static void
awaitable_dealloc(PyObject *self)
{
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
#define CLEAR_IF_NON_NULL(array)             \
        if (array.items != NULL) {           \
            pyawaitable_array_clear(&array); \
        }
    CLEAR_IF_NON_NULL(aw->aw_callbacks);
    CLEAR_IF_NON_NULL(aw->aw_object_values);
    CLEAR_IF_NON_NULL(aw->aw_arbitrary_values);
    CLEAR_IF_NON_NULL(aw->aw_integer_values);
#undef CLEAR_IF_NON_NULL

    Py_XDECREF(aw->aw_gen);
    Py_XDECREF(aw->aw_result);

    if (!aw->aw_done)
    {
        if (
            PyErr_WarnEx(
                PyExc_RuntimeWarning,
                "pyawaitable object was never awaited",
                1
            ) < 0
        )
        {
            PyErr_WriteUnraisable(self);
        }
    }

    Py_TYPE(self)->tp_free(self);
}

_PyAwaitable_API(void)
PyAwaitable_Cancel(PyObject * self)
{
    assert(self != NULL);
    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    pyawaitable_array_clear_items(&aw->aw_callbacks);
    aw->aw_state = 0;
    if (aw->aw_gen != NULL)
    {
        GenWrapperObject *gw = (GenWrapperObject *)aw->aw_gen;
        Py_CLEAR(gw->gw_current_await);
    }

    aw->aw_recently_cancelled = 1;
}

_PyAwaitable_API(int)
PyAwaitable_AddAwait(
    PyObject * self,
    PyObject * coro,
    awaitcallback cb,
    awaitcallback_err err
)
{
    PyAwaitableObject *aw = (PyAwaitableObject *) self;

    pyawaitable_callback *aw_c = PyMem_Malloc(sizeof(pyawaitable_callback));
    if (aw_c == NULL)
    {
        PyErr_NoMemory();
        return -1;
    }

    aw_c->coro = Py_NewRef(coro);
    aw_c->callback = cb;
    aw_c->err_callback = err;
    aw_c->done = false;

    if (pyawaitable_array_append(&aw->aw_callbacks, aw_c) < 0)
    {
        PyMem_Free(aw_c);
        PyErr_NoMemory();
        return -1;
    }

    return 0;
}

_PyAwaitable_API(int)
PyAwaitable_DeferAwait(PyObject *awaitable, defer_callback cb)
{
    PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;
    pyawaitable_callback *aw_c = PyMem_Malloc(sizeof(pyawaitable_callback));
    if (aw_c == NULL)
    {
        PyErr_NoMemory();
        return -1;
    }

    aw_c->coro = NULL;
    aw_c->callback = (awaitcallback)cb;
    aw_c->err_callback = NULL;
    aw_c->done = false;

    if (pyawaitable_array_append(&aw->aw_callbacks, aw_c) < 0)
    {
        PyMem_Free(aw_c);
        PyErr_NoMemory();
        return -1;
    }

    return 0;
}

_PyAwaitable_API(int)
PyAwaitable_SetResult(PyObject * awaitable, PyObject * result)
{
    PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;
    aw->aw_result = Py_NewRef(result);
    return 0;
}

_PyAwaitable_INTERNAL(PyObject *)
_PyAwaitable_GetType(PyObject * mod, const char * type)
{
    PyObject *_type = PyObject_GetAttrString(mod, type);
    if (!PyCallable_Check(_type))
    {
        Py_XDECREF(_type);
        return NULL;
    }

    return _type;
}

_PyAwaitable_API(PyObject *)
PyAwaitable_GetType(PyObject * mod)
{
    return _PyAwaitable_GetType(mod, "_PyAwaitableType");
}

_PyAwaitable_API(PyObject *)
PyAwaitable_New(PyObject * mod)
{
    // XXX Use a freelist?
    PyObject *type = PyAwaitable_GetType(mod);
    PyObject *result = awaitable_new_func((PyTypeObject *)type, NULL, NULL);

    // TODO: Store Weak Reference here.
    ((PyAwaitableObject *)result)->aw_mod = mod;
    Py_DECREF(type);
    return result;
}

_PyAwaitable_API(PyTypeObject) PyAwaitable_Type =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_PyAwaitableType",
    .tp_basicsize = sizeof(PyAwaitableObject),
    .tp_dealloc = awaitable_dealloc,
    .tp_as_async = &pyawaitable_async_methods,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("Awaitable transport utility for the C API."),
    .tp_iternext = awaitable_next,
    .tp_new = awaitable_new_func,
    .tp_methods = pyawaitable_methods
};
