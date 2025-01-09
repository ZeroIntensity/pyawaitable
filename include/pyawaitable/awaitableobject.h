#ifndef PYAWAITABLE_AWAITABLE_H
#define PYAWAITABLE_AWAITABLE_H

#include <Python.h>
#include <stdbool.h>

#include <pyawaitable/array.h>
#include <pyawaitable/dist.h>

typedef int (*awaitcallback)(PyObject *, PyObject *);
typedef int (*awaitcallback_err)(PyObject *, PyObject *);

typedef struct _pyawaitable_callback
{
    PyObject *coro;
    awaitcallback callback;
    awaitcallback_err err_callback;
    bool done;
} _PyAwaitable_MANGLE(pyawaitable_callback);

struct _PyAwaitableObject
{
    PyObject_HEAD

    pyawaitable_array aw_callbacks;
    pyawaitable_array aw_object_values;
    pyawaitable_array aw_arbitrary_values;
    pyawaitable_array aw_integer_values;

    /* Index of current callback */
    Py_ssize_t aw_state;
    /* Is the awaitable done? */
    bool aw_done;
    /* Was the awaitable awaited? */
    bool aw_awaited;
    /* Strong reference to the result of the coroutine. */
    PyObject *aw_result;
    /* Strong reference to the genwrapper. */
    PyObject *aw_gen;
    /* Set to 1 if the object was cancelled, for introspection against callbacks */
    int aw_recently_cancelled;
};

typedef struct _PyAwaitableObject PyAwaitableObject;
PyTypeObject PyAwaitableType;

_PyAwaitable_API(int)
pyawaitable_set_result(PyObject * awaitable, PyObject * result);

_PyAwaitable_API(int)
pyawaitable_await(
    PyObject * aw,
    PyObject * coro,
    awaitcallback cb,
    awaitcallback_err err
);

_PyAwaitable_API(void)
pyawaitable_cancel(PyObject * aw);

_PyAwaitable_INTERNAL(PyObject *)
awaitable_next(PyObject * self);

_PyAwaitable_API(PyObject *)
pyawaitable_new(void);

_PyAwaitable_API(int)
pyawaitable_await_function(
    PyObject * awaitable,
    PyObject * func,
    const char *fmt,
    awaitcallback cb,
    awaitcallback_err err,
    ...
);

#endif
