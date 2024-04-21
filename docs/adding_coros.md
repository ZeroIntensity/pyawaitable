# Adding Coroutines

The public interface for adding a coroutine to be executed by the event loop is ``awaitable_await``, which takes four parameters:

- ``aw`` is the ``AwaitableObject*``.
- ``coro`` is the coroutine (or again, any object supporting ``__await__``). This is not checked by this function, but is checked later inside the ``__next__`` of the ``AwaitableObject*``'s coroutine iterator). This is not a function defined with ``async def``, but instead the return value of one (called without ``await``). This value is stored for the lifetime of the object, or until ``awaitable_cancel`` is called.
- ``cb`` is the callback that will be run with the result of ``coro``. This may be ``NULL``, in which case the result will be discarded.
- ``err`` is a callback in the event that an exception occurs during the execution of ``coro``. This may be ``NULL``, in which case the error is simply raised.

The awaitable is guaranteed to yield (or ``await``) each coroutine in the order they were added to the awaitable. For example, if ``foo`` was added, then ``bar``, then ``baz``, first ``foo`` would be awaited (with its respective callbacks), then ``bar``, and finally ``baz``.

An example of ``awaitable_await`` (without callbacks) is as follows:

```c
static PyObject *
spam(PyObject *self, PyObject *args)
{
    PyObject *foo;
    PyObject *bar;
    // In this example, these are both coroutines, not asynchronous functions
    
    if (!PyArg_ParseTuple(args, "OOO", &foo, &bar))
        return NULL;

    PyObject *awaitable = awaitable_new();

    if (awaitable == NULL)
        return NULL;

    if (awaitable_await(awaitable, foo, NULL, NULL) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }
    
    if (awaitable_await(awaitable, bar, NULL, NULL) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }
    
    return awaitable;
}
```

```py
import asyncio

async def foo():
    print("foo!")

async def bar():
    print("bar!")

asyncio.run(spam(foo(), bar()))
# foo! is printed, then bar!
```

