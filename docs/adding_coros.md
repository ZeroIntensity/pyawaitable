# Adding Coroutines

## Basics

The public interface for adding a coroutine to be executed by the event loop is ``awaitable_await``, which takes four parameters:

```c
// Signature of awaitable_await, for reference
int
awaitable_await(
    PyObject *aw,
    PyObject *coro,
    awaitcallback cb,
    awaitcallback_err err
);
```

!!! warning

    If you are using the `PyAwaitable_` prefix, the function is ``PyAwaitable_AddAwait`` instead of ``PyAwaitable_Await``, per previous implementations of PyAwaitable.

- ``aw`` is the ``AwaitableObject*``.
- ``coro`` is the coroutine (or again, any object supporting ``__await__``).
- ``cb`` is the callback that will be run with the result of ``coro``. This may be ``NULL``, in which case the result will be discarded.
- ``err`` is a callback in the event that an exception occurs during the execution of ``coro``. This may be ``NULL``, in which case the error is simply raised.

`awaitable_await` may return `0`, indicating a success, or `-1`. 


!!! note

    The awaitable is guaranteed to yield (or ``await``) each coroutine in the order they were added to the awaitable. For example, if ``foo`` was added, then ``bar``, then ``baz``, first ``foo`` would be awaited (with its respective callbacks), then ``bar``, and finally ``baz``.

The `coro` parameter is not a *function* defined with `async def`, but instead an object supporting `__await__`. In the case of an `async def`, that would be a coroutine. In the example below, you would pass `bar` to `awaitable_await`, **not** `foo`:

```py
async def foo():
    ...

bar = foo()
```

`awaitable_await` does *not* check that the object supports the await protocol, but instead stores the object, and then checks it once the `AwaitableObject*` begins yielding it. This behavior prevents an additional lookup, and also allows you to pass another `AwaitableObject*` to `awaitable_await`, making it possible to chain `AwaitableObject*`'s. Note that even after the object is finished awaiting, the `AwaitableObject*` will still hold a reference to it (*i.e.*, it will not be deallocated until the `AwaitableObject*` gets deallocated).

!!! danger

    Do not pass the awaitable object itself as a coroutine:

    ```c
    static PyObject *
    spam(PyObject *self, PyObject *args)
    {
        PyObject *awaitable = awaitable_new();
        if (awaitable == NULL)
            return NULL;

        // DO NOT DO THIS
        if (awaitable_await(awaitable, awaitable, NULL, NULL) < 0)
        {
            Py_DECREF(awaitable);
            return NULL;
        }

        return awaitable;
    }
    ```


Here's an example of awaiting a coroutine from C:

```c
static PyObject *
spam(PyObject *self, PyObject *args)
{
    PyObject *foo;
    // In this example, this is a coroutines, not an asynchronous function
    
    if (!PyArg_ParseTuple(args, "O", &foo))
        return NULL;

    PyObject *awaitable = awaitable_new();

    if (awaitable == NULL)
        return NULL;

    if (awaitable_await(awaitable, foo, NULL, NULL) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }
    
    return awaitable;
}
```

This would be equivalent to `await foo` from Python.

## Return Values

You can set a return value (the thing that `await c_func()` will evaluate to) via `awaitable_set_result` (`PyAwaitable_SetResult` in the Python prefixes). By default, the return value is `None`.

!!! warning

    `awaitable_set_result` can *only* be called from a callback. Otherwise, a `TypeError` is raised.

For example:

```c
static int
callback(PyObject *awaitable, PyObject *result)
{
    if (awaitable_set_result(awaitable, result) < 0)
        return -1;

    // Do something with the result...
    return 0;
}
```


