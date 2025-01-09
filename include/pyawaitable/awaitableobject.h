#ifndef PYAWAITABLE_AWAITABLE_H
#define PYAWAITABLE_AWAITABLE_H

typedef int (*awaitcallback)(PyObject *, PyObject *);
typedef int (*awaitcallback_err)(PyObject *, PyObject *);
typedef int (*defer_callback)(PyObject *);

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
extern PyTypeObject _PyAwaitableType;

int pyawaitable_set_result_impl(PyObject *awaitable, PyObject *result);

int pyawaitable_await_impl(
    PyObject *aw,
    PyObject *coro,
    awaitcallback cb,
    awaitcallback_err err
);

int pyawaitable_defer_await_impl(PyObject *aw, defer_callback cb);

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

#endif
