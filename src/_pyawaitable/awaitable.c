#include <Python.h>
#include <stdlib.h>

#include <pyawaitable/array.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/coro.h>
#include <pyawaitable/genwrapper.h>

PyDoc_STRVAR(
    awaitable_doc,
    "Awaitable transport utility for the C API."
);

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

PyObject *
awaitable_next(PyObject *self)
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

void
pyawaitable_cancel_impl(PyObject *self)
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

int
pyawaitable_await_impl(
    PyObject *self,
    PyObject *coro,
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

int
pyawaitable_defer_await_impl(PyObject *awaitable, defer_callback cb)
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

int
pyawaitable_set_result_impl(PyObject *awaitable, PyObject *result)
{
    PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;
    aw->aw_result = Py_NewRef(result);
    return 0;
}

PyObject *
pyawaitable_new_impl(void)
{
    // XXX Use a freelist?
    return awaitable_new_func(&_PyAwaitableType, NULL, NULL);
}

int
pyawaitable_await_function_impl(
    PyObject *awaitable,
    PyObject *func,
    const char *fmt,
    awaitcallback cb,
    awaitcallback_err err,
    ...
)
{
    size_t len = strlen(fmt);
    size_t size = len + 3;
    char *tup_format = PyMem_Malloc(size);
    if (!tup_format)
    {
        PyErr_NoMemory();
        return -1;
    }

    tup_format[0] = '(';
    for (size_t i = 0; i < len; ++i)
    {
        tup_format[i + 1] = fmt[i];
    }

    tup_format[size - 2] = ')';
    tup_format[size - 1] = '\0';

    va_list vargs;
    va_start(vargs, err);
    PyObject *args = Py_VaBuildValue(tup_format, vargs);
    va_end(vargs);
    PyMem_Free(tup_format);

    if (!args)
        return -1;
    PyObject *coro = PyObject_Call(func, args, NULL);
    Py_DECREF(args);

    if (!coro)
        return -1;

    if (pyawaitable_await_impl(awaitable, coro, cb, err) < 0)
    {
        Py_DECREF(coro);
        return -1;
    }

    Py_DECREF(coro);
    return 0;
}

PyTypeObject _PyAwaitableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_PyAwaitableType",
    .tp_basicsize = sizeof(PyAwaitableObject),
    .tp_dealloc = awaitable_dealloc,
    .tp_as_async = &pyawaitable_async_methods,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = awaitable_doc,
    .tp_iternext = awaitable_next,
    .tp_new = awaitable_new_func,
    .tp_methods = pyawaitable_methods
};
