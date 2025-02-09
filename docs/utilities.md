---
hide:
    - navigation
---

# Useful Utilities

## Introduction

So far, it might seem like this is a lot of boilerplate. That's unfortunately decently common for the C API, but you get used to it. With that being said, how can we help eliminate at least _some_ of this boilerplate?

## Asynchronous Contexts

What about using `async with` from C? Well, asynchronous context managers are sort of simple, you just have to deal with calling `__aenter__` and `__aexit__`. But that's no fun--can we do it automatically? Yes you can!

PyAwaitable supports asynchronous context managers via `pyawaitable_async_with`. To start, let's start with some Python code that we want to replicate in C:

```py
async def my_function(async_context):
    async with async_context as value:
        print(f"My async value is {value}")
```

`pyawaitable_async_with` is pretty similiar to `pyawaitable_await`, but instead of taking a callback with the result of an awaited coroutine, it takes a callback that is ran when inside the context. So, translating the above snippet in C would look like:

```c
static int
inner(PyObject *awaitable, PyObject *value)
{
    // Inside the context!
    printf("My async value is: ");
    PyObject_Print(value, stdout, Py_PRINT_RAW);
    return 0;
}

static PyObject *
my_function(PyObject *self, PyObject *async_context)
{
    PyObject *awaitable = pyawaitable_new();

    // Equivalent to async with async_context
    if (pyawaitable_async_with(awaitable, async_context, inner, NULL) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }

    return awaitable;
}
```

Again, the `NULL` parameter here is an error callback. It's equivalent to what would happen if you wrapped a `try` block around an `async with`.

## Next Steps

Congratulations, you now know how to fully use PyAwaitable! If you're interested in reading about the internals, be sure to take a look at the [scrapped PEP draft](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926), where this was originally designed to be part of CPython.

Moreover, this project was conceived due to being needed in [view.py](https://github.com/ZeroIntensity/view.py). If you would like to see some very complex examples of PyAwaitable usage, take a look at their [C ASGI implementation](https://github.com/ZeroIntensity/view.py/blob/master/src/_view/app.c#L273), which is powered by PyAwaitable.
