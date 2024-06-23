#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>
#include <pyawaitable/coro.h>
#include <stdlib.h>
#define AWAITABLE_POOL_SIZE 256

PyDoc_STRVAR(
    awaitable_doc,
    "Awaitable transport utility for the C API."
);

static Py_ssize_t pool_index = 0;
static PyObject *pool[AWAITABLE_POOL_SIZE];

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
    aw->aw_result = Py_NewRef(Py_None);
    return (PyObject *) aw;
}

PyObject *
awaitable_next(PyObject *self)
{
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    aw->aw_awaited = true;

    if (aw->aw_done)
    {
        PyErr_SetString(
            PyExc_RuntimeError,
            "pyawaitable: cannot reuse awaitable"
        );
        return NULL;
    }

    PyObject *gen = genwrapper_new(aw);

    if (gen == NULL)
    {
        return NULL;
    }

    aw->aw_gen = gen;
    return gen;
}

static void
awaitable_dealloc(PyObject *self)
{
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    for (int i = 0; i < VALUE_ARRAY_SIZE; ++i)
    {
        if (!aw->aw_values[i])
            break;
        Py_DECREF(aw->aw_values[i]);
    }

    Py_XDECREF(aw->aw_gen);
    Py_XDECREF(aw->aw_result);

    for (int i = 0; i < CALLBACK_ARRAY_SIZE; ++i)
    {
        pyawaitable_callback *cb = aw->aw_callbacks[i];
        if (!cb)
            break;

        if (!cb->done)
            Py_DECREF(cb->coro);
        PyMem_Free(cb);
    }

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

    PyAwaitableObject *a = (PyAwaitableObject *) aw;

    for (int i = 0; i < CALLBACK_ARRAY_SIZE; ++i)
    {
        pyawaitable_callback *cb = a->aw_callbacks[i];
        if (!cb)
            break;

        if (!cb->done)
            Py_DECREF(cb->coro);
    }

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
    PyAwaitableObject *a = (PyAwaitableObject *) aw;
    if (a->aw_callback_index == CALLBACK_ARRAY_SIZE)
    {
        PyErr_SetString(
            PyExc_SystemError,
            "pyawaitable: awaitable object cannot store more than 128 coroutines"
        );
        return -1;
    }


    pyawaitable_callback *aw_c = PyMem_Malloc(sizeof(pyawaitable_callback));
    if (aw_c == NULL)
    {
        Py_DECREF(aw);
        Py_DECREF(coro);
        PyErr_NoMemory();
        return -1;
    }

    aw_c->coro = coro; // Steal our own reference
    aw_c->callback = cb;
    aw_c->err_callback = err;
    a->aw_callbacks[a->aw_callback_index++] = aw_c;
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
    if (pool_index == AWAITABLE_POOL_SIZE)
    {
        PyObject *aw = awaitable_new_func(&_PyAwaitableType, NULL, NULL);
        return aw;
    }

    return pool[pool_index++];
}

void
dealloc_awaitable_pool(void)
{
    for (Py_ssize_t i = pool_index; i < AWAITABLE_POOL_SIZE; ++i)
        Py_DECREF(pool[i]);
}

int
alloc_awaitable_pool(void)
{
    for (Py_ssize_t i = 0; i < AWAITABLE_POOL_SIZE; ++i)
    {
        pool[i] = awaitable_new_func(&_PyAwaitableType, NULL, NULL);
        if (!pool[i])
        {
            for (Py_ssize_t x = 0; x < i; ++x)
                Py_DECREF(pool[x]);
            return -1;
        }
    }

    return 0;
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
