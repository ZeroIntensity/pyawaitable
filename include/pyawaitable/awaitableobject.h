#ifndef PYAWAITABLE_AWAITABLE_H
#define PYAWAITABLE_AWAITABLE_H

#include <Python.h>
#include <stdbool.h>

typedef int (*awaitcallback)(PyObject *, PyObject *);
typedef int (*awaitcallback_err)(PyObject *, PyObject *);
#define CALLBACK_ARRAY_SIZE 128
#define VALUE_ARRAY_SIZE 32

typedef struct _pyawaitable_callback
{
    PyObject *coro;
    awaitcallback callback;
    awaitcallback_err err_callback;
    bool done;
} pyawaitable_callback;

struct _PyAwaitableObject
{
    PyObject_HEAD
    PyObject *aw_result;

    // Callbacks
    pyawaitable_callback *aw_callbacks[CALLBACK_ARRAY_SIZE];
    Py_ssize_t aw_callback_index;

    // Stored Values
    PyObject *aw_values[VALUE_ARRAY_SIZE];
    Py_ssize_t aw_values_index;

    // Arbitrary Values
    void *aw_arb_values[VALUE_ARRAY_SIZE];
    Py_ssize_t aw_arb_values_index;

    // Integer Values
    long aw_int_values[VALUE_ARRAY_SIZE];
    Py_ssize_t aw_int_values_index;

    // Awaitable State
    Py_ssize_t aw_state;
    bool aw_done;
    bool aw_awaited;
    bool aw_used;
};

typedef struct _PyAwaitableObject PyAwaitableObject;
extern PyTypeObject _PyAwaitableType;

int pyawaitable_set_result_impl(PyObject *awaitable, PyObject *result);

int pyawaitable_await_impl(
    PyObject *aw,
    PyObject *coro,
    awaitcallback cb,
    awaitcallback_err err
);

void pyawaitable_cancel_impl(PyObject *aw);

PyObject *
awaitable_next(PyObject *self);

PyObject *
pyawaitable_new_impl(void);

int
pyawaitable_await_function_impl(
    PyObject *awaitable,
    PyObject *func,
    const char *fmt,
    awaitcallback cb,
    awaitcallback_err err,
    ...
);

int
alloc_awaitable_pool(void);
void
dealloc_awaitable_pool(void);

#endif
