# Callbacks

The first argument in an ``awaitcallback`` is the ``AwaitableObject*`` (casted to a ``PyObject*``, once again), and the second argument is the result of the coroutine. Both of these are borrowed references, and should not be ``Py_DECREF``'d by the user. The return value of this function must be an integer. Any value below ``0`` denotes an error occurred, but there are two different ways to handle it:

- If the function returned ``-1``, it expects the error to be deferred to the error callback if it exists.
- If the function returned anything less than ``-1``, the error callback is ignored, and the error is deferred to the event loop (*i.e.*, ``__next__`` on the object's coroutine returns ``NULL``).

In an ``awaitcallback_err``, there are once again two arguments, both of which are again, borrowed references. The first argument is a ``AwaitableObject*``casted to a ``PyObject*``, and the second argument is the current exception (via ``PyErr_GetRaisedException``). Likewise, this function can also return an error, which is once again denoted by a value less than ``0``. This function also has two ways to handle exceptions:

- ``-1`` denotes that the original error should be restored via ``PyErr_SetRaisedException``.
- ``-2`` or lower says to not restore the error, and instead use the current error set by the callback. If no error is set, a ``SystemError`` is raised.

If either of these callbacks return an error value without an exception set, a ``SystemError`` is raised.

An example of using callbacks is shown below:

```d
static int
spam_callback(PyObject *awaitable, PyObject *result)
{
    printf("coro returned result: ");
    PyObject_Print(result, stdout, Py_PRINT_RAW);
    putc('\n');

    return 0;
}


static PyObject *
spam(PyObject *self, PyObject *args)
{
    PyObject *coro;
    if (!PyArg_ParseTuple(args, "O", &coro))
        return NULL;

    PyObject *awaitable = awaitable_new();

    if (awaitable_await(awaitable, coro, spam_callback, NULL) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }

    return awaitable;
}
```