---
hide:
    - navigation
---

# Callbacks

## Result Callbacks

A result callback is essentially a C function that is called with the result of the awaited coroutine, whenever it's ready. In terms of functionality, you can think of result callbacks in the same way you would think about a function passed to [add_done_callback](https://docs.python.org/3/library/asyncio-task.html#asyncio.Task.add_done_callback).

The type of a result callback is an `awaitcallback`:

```c
typedef int (*awaitcallback)(PyObject *, PyObject *);
```

The first argument in an `awaitcallback` is the underlying awaitable object (which of course can be used in all `pyawaitable_*` functions), and the second argument is the result of the coroutine. Both of these are _borrowed_ references, and should not be `Py_DECREF`'d by the user.

The return value of a callback function must be an integer and per CPython conventions, any value below `0` denotes an error occurred, but there are two different ways for PyAwaitable to handle it:

-   If the callback returned `-1`, it expects the error to be deferred to the error callback if it exists.
-   If the callback returned anything less than `-1`, the error callback is ignored, and the error propagates to the event loop.

## Error Callbacks

```c
typedef int (*awaitcallback_err)(PyObject *, PyObject *);
```

!!! note

    Both `awaitcallback` and `awaitcallback_err` have the same signature, but for semantic purposes, are different types (as the underlying `PyObject*` they take are different Python objects).

The first parameter is equivalent to that of `awaitcallback`, and the second is an exception object, via `PyErr_GetRaisedException` (this means that the error indicator is cleared before the execution of the error callback). Both of these are, once again, borrowed references.

!!! note

    ``PyErr_GetRaisedException`` was added in 3.12, meaning you cannot use it on previous versions. PyAwaitable gets around this with an internal backport, so you can expect an actual exception object on all versions.

Likewise, an error callback can also return an error, which is once again denoted by a value less than `0`, but also has two ways to handle exceptions:

-   `-1` denotes that the original error should be restored via `PyErr_SetRaisedException` (again handled by a backport on <3.12).
-   `-2` or lower says to _not_ restore the error, and instead use the current error set by the callback.

If either a result callback or an error callback return an error value without an exception set, a `SystemError` is raised.

For example:

```c
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

    PyObject *awaitable = pyawaitable_new();

    if (pyawaitable_await(awaitable, coro, spam_callback, NULL) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }

    return awaitable;
}
```

## Cancelling

The public interface for cancelling (_i.e._, stop all loaded coroutines from being executed, and decrement their reference count) is `pyawaitable_cancel`:

For example:

```c
static int
callback(PyObject *awaitable, PyObject *result)
{
    // Free all stored coroutines
    pyawaitable_cancel(awaitable);
    return 0;
}
```

Note that if you're in a callback it _is_ possible to add coroutines again after cancelling, but only from that callback, since the future callbacks will be skipped (because the coroutines are removed by you!)
