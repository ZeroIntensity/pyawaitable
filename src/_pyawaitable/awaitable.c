#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>
#include <pyawaitable/coro.h>
#include <stdlib.h>

PyDoc_STRVAR(
    awaitable_doc,
    "Awaitable transport utility for the C API."
);

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

    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    aw->aw_callbacks = NULL;
    aw->aw_callback_size = 0;
    aw->aw_result = Py_NewRef(Py_None);
    aw->aw_gen = NULL;
    aw->aw_values = NULL;
    aw->aw_values_size = 0;
    aw->aw_state = 0;
    aw->aw_done = false;
    aw->aw_awaited = false;

    return (PyObject *)aw;
}

PyObject *
awaitable_next(PyObject *self)
{
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    aw->aw_awaited = true;

    if (aw->aw_done)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot reuse awaitable");
        return NULL;
    }

    PyObject *gen = genwrapper_new(aw);

    if (gen == NULL)
    {
        return NULL;
    }

    aw->aw_gen = Py_NewRef(gen);
    aw->aw_done = true;
    return gen;
}

static void
awaitable_dealloc(PyObject *self)
{
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    if (aw->aw_values)
    {
        for (int i = 0; i < aw->aw_values_size; i++)
            Py_DECREF(aw->aw_values[i]);
        PyMem_Free(aw->aw_values);
    }

    Py_XDECREF(aw->aw_gen);
    Py_XDECREF(aw->aw_result);

    for (int i = 0; i < aw->aw_callback_size; i++)
    {
        pyawaitable_callback *cb = aw->aw_callbacks[i];
        if (!cb->done)
            Py_DECREF(cb->coro);
        PyMem_Free(cb);
    }

    if (aw->aw_arb_values)
        PyMem_Free(aw->aw_arb_values);

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
pyawaitable_cancel_impl(PyObject *aw)
{
    assert(aw != NULL);
    Py_INCREF(aw);

    PyAwaitableObject *a = (PyAwaitableObject *)aw;

    for (int i = 0; i < a->aw_callback_size; i++)
    {
        pyawaitable_callback *cb = a->aw_callbacks[i];
        if (!cb->done)
            Py_DECREF(cb->coro);
    }

    PyMem_Free(a->aw_callbacks);
    a->aw_callback_size = 0;
    Py_DECREF(aw);
}

int
pyawaitable_await_impl(
    PyObject *aw,
    PyObject *coro,
    awaitcallback cb,
    awaitcallback_err err
)
{
    assert(aw != NULL);
    assert(coro != NULL);
    Py_INCREF(coro);
    Py_INCREF(aw);
    PyAwaitableObject *a = (PyAwaitableObject *)aw;

    pyawaitable_callback *aw_c = PyMem_Malloc(sizeof(pyawaitable_callback));
    if (aw_c == NULL)
    {
        Py_DECREF(aw);
        Py_DECREF(coro);
        PyErr_NoMemory();
        return -1;
    }

    ++a->aw_callback_size;
    if (a->aw_callbacks == NULL)
    {
        a->aw_callbacks = PyMem_Calloc(
            a->aw_callback_size,
            sizeof(pyawaitable_callback *)
        );
    }else
    {
        a->aw_callbacks = PyMem_Realloc(
            a->aw_callbacks,
            sizeof(pyawaitable_callback *) *
            a->aw_callback_size
        );
    }

    if (a->aw_callbacks == NULL)
    {
        --a->aw_callback_size;
        Py_DECREF(aw);
        Py_DECREF(coro);
        PyMem_Free(aw_c);
        PyErr_NoMemory();
        return -1;
    }

    aw_c->coro = coro; // steal our own reference
    aw_c->callback = cb;
    aw_c->err_callback = err;
    a->aw_callbacks[a->aw_callback_size - 1] = aw_c;
    Py_DECREF(aw);

    return 0;
}

int
pyawaitable_set_result_impl(PyObject *awaitable, PyObject *result)
{
    assert(awaitable != NULL);
    assert(result != NULL);
    Py_INCREF(result);
    Py_INCREF(awaitable);

    PyAwaitableObject *aw = (PyAwaitableObject *)awaitable;
    aw->aw_result = Py_NewRef(result);
    Py_DECREF(awaitable);
    Py_DECREF(result);
    return 0;
}

PyObject *
pyawaitable_new_impl(void)
{
    PyObject *aw = awaitable_new_func(&_PyAwaitableType, NULL, NULL);
    return aw;
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
    char *tup_format = PyMem_Malloc(len + 3);
    if (!tup_format)
    {
        PyErr_NoMemory();
        return -1;
    }

    tup_format[0] = '(';
    strcpy(tup_format + 1, fmt);
    tup_format[len - 2] = ')';
    tup_format[len - 1] = '\0';

    va_list vargs;
    va_start(vargs, err);
    PyObject *args = Py_VaBuildValue(tup_format, vargs);
    va_end(vargs);
    PyMem_Free(tup_format);

    if (!args)
        return -1;

    PyObject *coro = PyObject_Call(func, args, NULL);

    if (!coro)
        return -1;

    if (pyawaitable_await_impl(awaitable, coro, cb, err) < 0)
    {
        Py_DECREF(coro);
        return -1;
    }

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
