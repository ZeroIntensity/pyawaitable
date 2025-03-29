# Making Asynchronous Calls from C

Let's say we wanted to replicate the following in C:

```py
async def trampoline(coro: collections.abc.Coroutine) -> int:
    return await coro
```

This is simply a function that we pass a coroutine to, and it will await it for us. It's not particularly useful, but it's just for learning purposes.

We already know that `trampoline()` will evaluate to a magic coroutine object that supports `await`, via the `__await__` dunder, and needs to `yield from` its coroutines. So, if we wanted to break `trampoline` down into a synchronous Python function, it would look something like this:

```py
class _trampoline_coroutine:
    def __init__(self, coro: collections.abc.Coroutine) -> None:
        self.coro = coro

    def __await__(self) -> collections.abc.Generator:
        yield
        yield from self.coro.__await__()

def trampoline(coro: collections.abc.Coroutine) -> collections.abc.Coroutine:
    return _trampoline_coroutine(coro)
```

But, this is using `yield from`; there's no `yield from` in C, so how do we actually await things, or more importantly, use their return value? This is where things get tricky.

## Adding Awaits to a PyAwaitable Object

There's one big function for "adding" coroutines to a PyAwaitable object: `PyAwaitable_AddAwait`. By "add," we mean that the asynchronous call won't happen right then and there. Instead, the PyAwaitable will store it, and then when something comes to call the `__await__` on the PyAwaitable object, it will mimick a `yield from` on that coroutine.

`PyAwaitable_AddAwait` takes four arguments:

-   The PyAwaitable object.
-   The _coroutine_ to store. (Not an `async def` function, but the result of calling one without `await`.)
-   A callback.
-   An error callback.

Let's focus on the first two for now, and just pass `NULL` for the other two in the meantime. We can implement `trampoline` from our earlier example pretty easily:

```c
static PyObject *
trampoline(PyObject *self, PyObject *coro) // METH_O
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    if (PyAwaitable_AddAwait(awaitable, coro, NULL, NULL) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    return awaitable;
}
```

To your eyes, the `yield from` and all of that mess is completely hidden; you give PyAwaitable your coroutine, and it handles the rest! `trampoline` now acts like our pure-Python function from earlier:

```py
>>> from _yourmod import trampoline
>>> import asyncio
>>> await trampoline(asyncio.sleep(2))  # Sleeps for 2 seconds
```

Yay! We called an asynchronous function from C!

## Using the Return Value in a Callback
