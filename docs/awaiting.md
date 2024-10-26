---
hide:
    - navigation
---

# Awaiting Coroutines

## Introduction

Now, let's await some coroutines! As you likely know from working with Python code, you cannot `await` something from outside an asynchronous function. So, how do we make a C function asynchronous? Well, we're getting ahead of ourselves, we don't even have a method to call yet! For simplicitly, let's configure a function with `METH_O`:

```c
static PyObject *
test(PyObject *self, PyObject *arg)
{
    Py_RETURN_NONE;
}

static PyMethodDef methods[] = {
    {"test", test, METH_O, NULL},
    {NULL, NULL, 0, NULL} // sentinel
}
```

## Making an asynchronous C function

Now let's make this asynchronous. First, we need to create a new PyAwaitable object, through `pyawaitable_new`. This is where the magic happens! Returning one of these objects from the C function inherently makes it asynchronous (in a sense, at least), as if it were defined with `async def`. Using `test` from above, this looks like:

```c
static PyObject *
test(PyObject *self, PyObject *arg)
{
    return pyawaitable_new();
}
```

If you were to use `test` from Python, you can now execute it via `await` (note that the passed `42` is just to make `METH_O` happy):

```py
>>> import test
>>> await test.test(42)
```

This doesn't do anything yet, we simply have an awaitable function that's defined in C. However, we overrode the return value with our PyAwaitable object! If we want the `await` expression to return a value other than `None`, you can use `pyawaitable_set_result`:

```c
static PyObject *
test(PyObject *self, PyObject *arg)
{
    PyObject *aw = pyawaitable_new();
    if (aw == NULL)
    {
        return NULL;
    }

    PyObject *value = PyLong_FromLong(42);
    if (value == NULL)
    {
        Py_DECREF(aw);
        return NULL;
    }

    if (pyawaitable_set_result(aw, value) < 0)
    {
        Py_DECREF(aw);
        Py_DECREF(value);
        return NULL;
    }

    Py_DECREF(value);
    return aw;
}
```

Now, if we try this from Python:

```py
>>> import test
>>> await test.test(None)  # To satisfy METH_O, once again
42
```

## What's going on here?

To understand PyAwaitable, let's dive deeper into what `async def` actually does. We can get a clear idea of what we're doing with PyAwaitable by looking at what an asynchronous function returns, without the `await`:

```py
async def my_func() -> None:
    pass

print(type(my_func()))
```

In Python, all `async def` functions do is return a _coroutine_, which is usable via `await`. Our PyAwaitable is no different: it is also a coroutine! We mimic what an `async def` function would do by returning an object that can be awaited.

It's worth noting that a key difference between Python coroutines and PyAwaitable coroutines is the amount of lazy-evaluation. For example, in the following Python code:

```py
async def my_func() -> None:
    print(0 / 0)

my_func()
```

This _does not_ raise a `ZeroDivisionError`, because the coroutine is lazy: it won't execute that until we `await` it. However, with a C function, we would return `NULL` instead of the PyAwaitable object, so we can't be lazy.

## Awaiting from C

!!! note

    In technical terms, when we say "coroutine," what we really mean is *awaitable* (*i.e.*, you can perform `await` on it, or that it supports `__await__`.) Nothing in PyAwaitable is specific to coroutine objects, but really any awaitable object. For reference, see [collections.abc.Awaitable](https://docs.python.org/3/library/collections.abc.html#collections.abc.Awaitable).

Now that we understand defining asynchronous function, let's try and replicate the expression `await coro` in C. The function we need to use for awaiting a coroutine is `pyawaitable_await`. For reference, here's it's signature:

```c
int
pyawaitable_await(
    PyObject *aw,
    PyObject *coro,
    awaitcallback cb,
    awaitcallback_err err
);
```

There's a lot to unpack here! To keep it simple, let's just focus on the first two arguments: `aw` and `coro`. `aw` is the PyAwaitable object created by `pyawaitable_new`, and `coro` is a coroutine (or really, any object that supports `__await__`).

!!! note

    Note that once again, a coroutine is _not_ an `async def` function, but what's returned by it. You cannot `await` an `async def` function directly!

Ok, with that in mind, let's await the argument passed via `METH_O`:

```c
static PyObject *
test(PyObject *self, PyObject *arg)
{
    PyObject *aw = pyawaitable_new();
    if (pyawaitable_await(aw, arg, NULL, NULL) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    return aw;
}
```

The above is roughly equivalent to the following Python code:

```py
async def test(arg: Awaitable) -> None:
    await arg
```

However, a counter-intuitive property of this is that in C, `arg` is _not_ awaited directly after the `pyawaitable_await` call.

In fact, we don't actually `await` anything in our function body. Instead, we mark it for awaiting to PyAwaitable, and then that object we returned will deal with it when it is awaited by the user.

For example, if you wanted to `await` three coroutines: `foo`, `bar`, and `baz`, you would call `pyawaitable_await` on each of them, but they wouldn't actually get executed until _after_ the C function has returned.

### Calling Utilities

So far, it might seem like this is a lot of boilerplate. In general, calling functions in the C API is a lot of work--is there any way to make it prettier in PyAwaitable. CPython has `PyObject_CallFunction`, which allows you to call a function with a format string similar to `Py_BuildValue`. For example:

```c
static PyObject *
test(PyObject *self, PyObject *my_func)
{
    // Equivalent to my_func(42, -10)
    PyObject *result = PyObject_CallFunction(my_func, "ii", 42, -10);
    /* ... */
}
```

Much nicer, right? For convenience, PyAwaitable has an analogue of this function, called `pyawaitable_await_function`, which calls a function with a format string _and_ marks it for execution via `pyawaitable_await`. For example, if `my_func` from above was asynchronous:

```c
static PyObject *
test(PyObject *self, PyObject *my_func)
{
    PyObject *awaitable = pyawaitable_new();

    // Equivalent to await my_func(42, -10)
    if (pyawaitable_await_function(awaitable, my_func, "ii", NULL, NULL, 42, -10) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }
    /* ... */
}
```

## Next Steps

Now, how do we do something with the result of the awaited coroutine? We do that with a callback, which we'll talk about next.
