# Callbacks

## Introduction

In PyAwaitable, coroutines are not executed in the body of the C function, but after it's already returned. This is a bit of a problem when you want to actually do something with the result of the call. PyAwaitable solves this by using callbacks that are executed when something happens to a coroutine asynchronously.

Unfortunately, due to C's lack of closures, this isn't the prettiest solution, but value storage (which you'll learn about it) makes this somewhat nicer.

## Result Callbacks

A result callback is essentially a C function that is called with the result of the awaited coroutine, whenever it's ready. In terms of functionality, you can think of result callbacks in the same way you would think about a function passed to [add_done_callback](https://docs.python.org/3/library/asyncio-task.html#asyncio.Task.add_done_callback).

The type of a result callback is an `awaitcallback`:

```c
typedef int (*awaitcallback)(PyObject *, PyObject *);
```

The first argument in an `awaitcallback` is the underlying awaitable object (which of course can be used in all `pyawaitable_*` functions), and the second argument is the result of the coroutine.

Both of these are _borrowed_ references, and should not be `Py_DECREF`'d by the user!

Ok, ignoring the return value, let's try and write `print(await coro)` in C. Result callbacks are always set via the third argument of `pyawaitable_await`. For example:

```c
static int
callback(PyObject *awaitable, PyObject *result)
{
    PyObject_Print(result, stdout, Py_PRINT_RAW);
    puts("");
    return 0;
}

static PyObject *
test(PyObject *self, PyObject *arg)
{
    PyObject *aw = pyawaitable_new();
    if (pyawaitable_await(aw, arg, callback, NULL) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    return aw;
}
```

As shown above, the return value of a callback function must be an integer, and per CPython conventions, any value below `0` denotes an error occurred. Howver, unlike the rest of the C API, there are two different ways for PyAwaitable to handle an error return from a callback.

For most cases, you'll just want to return `-1`. For example:

```c
static int
callback(PyObject *awaitable, PyObject *result)
{
    PyErr_SetString(PyExc_RuntimeError, "No good!");
    return -1;
}

static PyObject *
test(PyObject *self, PyObject *arg)
{
    PyObject *aw = pyawaitable_new();
    if (pyawaitable_await(aw, arg, callback, NULL) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    return aw;
}
```

In this case, the error is propagated to the event loop, since we don't define an error callback (which you'll learn about in the next section). However, if the callback returned anything less than `-1`, the error callback is ignored, and the error _always_ propagates to the event loop.

Generally speaking, you should return `-1` where possible. We'll come back to this later.

## Error Callbacks

What happens if the coroutine, or it's result callback, throws an error? Error callbacks are the solution, and can be set via the fourth argument to `pyawaitable_await`.

Both `awaitcallback` and `awaitcallback_err` have the same signature, but are _semantically_ different, which is why they are seperated into different types.

The first parameter is equivalent to that of `awaitcallback` (it's the PyAwaitable object that was called), and the second is an exception object, via `PyErr_GetRaisedException` (this means that the error indicator is cleared before the execution of the error callback). Both of these are, once again, borrowed references.

!!! info "Technical Detail"

    If you're familiar with the C API, you might have been a bit confused to see that PyAwaitable is using ``PyErr_GetRaisedException`` under the hood, since that was added in 3.12, meaning you cannot use it on previous versions. PyAwaitable gets around this with an internal backport, so really, on <3.12 you're getting the first parameter from `PyErr_Fetch` passed to `PyErr_NormalizeException`.

Likewise, an error callback can also return an error, which is once again denoted by a value less than `0`, but also has two different types of error returns. For now, let's focus on an `0` return, which indicates that the error was properly handled, and it is _not_ reraised. As a quick example, let's just log that someting went wrong, instead of raising an error:

```c
static int
err_callback(PyObject *awaitable, PyObject *err)
{
    fputs("Something bad happened!", stderr);
    return 0;
}

static PyObject *
test(PyObject *self, PyObject *arg)
{
    PyObject *aw = pyawaitable_new();
    if (pyawaitable_await(aw, arg, NULL, err_callback) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    return aw;
}
```

Now, from Python:

```py
>>> async def foo():
>>>     0 / 0
>>> await test.test(foo())
Something bad happened!
>>>
```

As you can see, returning `0` from an error callback denotes that we already dealt with the error, so it is cleared and never propagated. It's worth noting that the error callback can be called if `-1` was returned from the result callback. As an example, let's mix our result and error callbacks from above, like so:

```c
static int
callback(PyObject *awaitable, PyObject *result)
{
    PyErr_SetString(PyExc_RuntimeError, "No good!");
    return -1;
}

static int
err_callback(PyObject *awaitable, PyObject *err)
{
    fputs("Something bad happened!", stderr);
    return 0;
}
```

In this case, the error callback would be called with the `RuntimeError` that was raised by `callback`! As mentioned, you can override this behavior by returning `-2` from the callback, which skips error callback handling:

```c
static int
callback(PyObject *awaitable, PyObject *result)
{
    PyErr_SetString(PyExc_RuntimeError, "No good!");
    return -2;
}

static int
err_callback(PyObject *awaitable, PyObject *err)
{
    fputs("Something bad happened!", stderr);
    return 0;
}
```

In the above, the `RuntimeError` is now _immediately_ raised, instead of going through the error callback.

## Error Returns

Ok, so how do we fail an error callback?

A value of `-1` denotes that something failed, and the original error (the one that you're catching, via the second argument of the callback) should be restored and propagated to the event loop. This means that, technically, you can return a failure from the error callback _without_ an exception present.

For example:

```c
static int
err_callback(PyObject *awaitable, PyObject *err)
{
    if (PyErr_GivenExceptionMatches(err, PyExc_RuntimeError)) {
        // That's ok, we want to suppress this error.
        return 0;
    }

    // Nope! Reraise it!
    // At this point, it's worth noting that no error
    // indicator is *currently* set.
    return -1;
}

static PyObject *
test(PyObject *self, PyObject *arg)
{
    PyObject *aw = pyawaitable_new();
    if (pyawaitable_await(aw, arg, NULL, err_callback) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    return aw;
}
```

Now, we could use this from Python, like so:

```py
>>> async def foo():
>>>     raise RuntimeError("Nobody expects the spanish inquisition")
>>> await test.test(foo())  # OK
>>> async def bar():
>>>     raise TypeError("No good!")
>>> await test.test(bar())  # TypeError!
```

Ok, so what if we have our own error that we want to propagate? If the error callback returns `-2`, then PyAwaitable will use _not_ reraise the passed exception, and instead use whatever the callback set as the error indicator. For example:

```c
static int
err_callback(PyObject *awaitable, PyObject *err)
{
    PyObject *my_str = PyUnicode_FromString("...");
    if (my_str == NULL)
    {
        // We have our own error, let's propagate it.
        return -2;
    }

    /* ... */

    return 0;
}
```

Note that if you return `-2` without an exception set, a `SystemError` is raised, per the rest of the CPython API:

```c
static int
err_callback(PyObject *awaitable, PyObject *err)
{
    return -2; // SystemError!
}
```

## Next Steps

Now that you've learned to use callbacks, how do we retain state from our original function (_i.e._, the thing that returned our PyAwaitable object) to a callback? In most other languages, this would be a stupid question, but if you've used callbacks in C before, you may have found yourself wondering what to do with the lack of closures.

For that, we have our own value storage system, which you'll learn about next.
