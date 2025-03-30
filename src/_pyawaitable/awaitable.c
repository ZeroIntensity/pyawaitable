#include <Python.h>
#include <stdlib.h>

#include <pyawaitable/array.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/coro.h>
#include <pyawaitable/genwrapper.h>
#include <pyawaitable/init.h>
#include <pyawaitable/optimize.h>

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
    if (PyAwaitable_UNLIKELY(self == NULL)) {
        return NULL;
    }

    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    aw->aw_gen = NULL;
    aw->aw_done = false;
    aw->aw_state = 0;
    aw->aw_result = NULL;
    aw->aw_recently_cancelled = 0;

    if (pyawaitable_array_init(&aw->aw_callbacks, callback_dealloc) < 0) {
        goto error;
    }

    if (
        pyawaitable_array_init(
            &aw->aw_object_values,
            (pyawaitable_array_deallocator) Py_DecRef
        ) < 0
    ) {
        goto error;
    }

    if (pyawaitable_array_init(&aw->aw_arbitrary_values, NULL) < 0) {
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
    if (aw->aw_done) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "PyAwaitable: Cannot reuse awaitable"
        );
        return NULL;
    }
    aw->aw_awaited = true;
    PyObject *gen = genwrapper_new(aw);
    aw->aw_gen = Py_XNewRef(gen);
    return gen;
}

static int
awaitable_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    pyawaitable_array *array = &aw->aw_object_values;
    if (array->items != NULL) {
        for (Py_ssize_t i = 0; i < pyawaitable_array_LENGTH(array); ++i) {
            PyObject *ref = pyawaitable_array_GET_ITEM(array, i);
            Py_VISIT(ref);
        }
    }
    Py_VISIT(aw->aw_gen);
    Py_VISIT(aw->aw_result);
    return 0;
}

static int
awaitable_clear(PyObject *self)
{
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    pyawaitable_array *array = &aw->aw_object_values;
    if (array->items != NULL) {
        pyawaitable_array_clear(array);
    }
    Py_CLEAR(aw->aw_gen);
    Py_CLEAR(aw->aw_result);
    return 0;
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
    CLEAR_IF_NON_NULL(aw->aw_arbitrary_values);
#undef CLEAR_IF_NON_NULL

    (void)awaitable_clear(self);

    if (!aw->aw_awaited) {
        if (
            PyErr_WarnEx(
                PyExc_ResourceWarning,
                "PyAwaitable object was never awaited",
                1
            ) < 0
        ) {
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
    if (aw->aw_gen != NULL) {
        GenWrapperObject *gw = (GenWrapperObject *)aw->aw_gen;
        Py_CLEAR(gw->gw_current_await);
    }

    aw->aw_recently_cancelled = 1;
    aw->aw_awaited = 1;
}

_PyAwaitable_API(int)
PyAwaitable_AddAwait(
    PyObject * self,
    PyObject * coro,
    PyAwaitable_Callback cb,
    PyAwaitable_Error err
)
{
    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    assert(Py_IS_TYPE(Py_TYPE(self), PyAwaitable_GetType()));
    if (coro == NULL) {
        PyErr_SetString(
            PyExc_ValueError,
            "PyAwaitable: NULL passed to PyAwaitable_AddAwait()! "
            "Did you forget an error check?"
        );
        return -1;
    }

    if (coro == self) {
        PyErr_Format(
            PyExc_ValueError,
            "PyAwaitable: Self (%R) was passed to PyAwaitable_AddAwait()! "
            "This would result in a recursive nightmare.",
            self
        );
        return -1;
    }

    if (!PyObject_HasAttrString(coro, "__await__")) {
        PyErr_Format(
            PyExc_TypeError,
            "PyAwaitable: %R is not an awaitable object",
            coro
        );
        return -1;
    }

    pyawaitable_callback *aw_c = PyMem_Malloc(sizeof(pyawaitable_callback));
    if (aw_c == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    aw_c->coro = Py_NewRef(coro);
    aw_c->callback = cb;
    aw_c->err_callback = err;
    aw_c->done = false;

    if (pyawaitable_array_append(&aw->aw_callbacks, aw_c) < 0) {
        PyMem_Free(aw_c);
        PyErr_NoMemory();
        return -1;
    }

    return 0;
}

_PyAwaitable_API(int)
PyAwaitable_DeferAwait(PyObject * awaitable, PyAwaitable_Defer cb)
{
    PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;
    assert(Py_IS_TYPE(Py_TYPE(awaitable), PyAwaitable_GetType()));
    pyawaitable_callback *aw_c = PyMem_Malloc(sizeof(pyawaitable_callback));
    if (aw_c == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    aw_c->coro = NULL;
    aw_c->callback = (PyAwaitable_Callback)cb;
    aw_c->err_callback = NULL;
    aw_c->done = false;

    if (pyawaitable_array_append(&aw->aw_callbacks, aw_c) < 0) {
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
    assert(Py_IS_TYPE(Py_TYPE(awaitable), PyAwaitable_GetType()));
    aw->aw_result = Py_NewRef(result);
    return 0;
}

_PyAwaitable_API(PyObject *)
PyAwaitable_New(void)
{
    // XXX Use a freelist?
    PyTypeObject *type = PyAwaitable_GetType();
    if (PyAwaitable_UNLIKELY(type == NULL)) {
        return NULL;
    }
    PyObject *result = awaitable_new_func(type, NULL, NULL);
    return result;
}

_PyAwaitable_INTERNAL_DATA_DEF(PyTypeObject) PyAwaitable_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_PyAwaitableType",
    .tp_basicsize = sizeof(PyAwaitableObject),
    .tp_dealloc = awaitable_dealloc,
    .tp_as_async = &pyawaitable_async_methods,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_doc = PyDoc_STR("Awaitable transport utility for the C API."),
    .tp_iternext = awaitable_next,
    .tp_new = awaitable_new_func,
    .tp_clear = awaitable_clear,
    .tp_traverse = awaitable_traverse,
    .tp_methods = pyawaitable_methods
};
