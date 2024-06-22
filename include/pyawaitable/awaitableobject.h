#ifndef PYAWAITABLE_AWAITABLE_H
#define PYAWAITABLE_AWAITABLE_H

#include <Python.h>
#include <pyawaitable.h>
#include <stdbool.h>

typedef struct _pyawaitable_callback
{
    PyObject *coro;
    awaitcallback callback;
    awaitcallback_err err_callback;
    bool done;
} pyawaitable_callback;

struct _PyAwaitableObject
{
    PyObject_HEAD pyawaitable_callback **aw_callbacks;
    Py_ssize_t aw_callback_size;
    PyObject *aw_result;
    PyObject *aw_gen;
    PyObject **aw_values;
    Py_ssize_t aw_values_size;
    void **aw_arb_values;
    Py_ssize_t aw_arb_values_size;
    Py_ssize_t aw_state;
    bool aw_done;
    bool aw_awaited;
};

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

#endif
