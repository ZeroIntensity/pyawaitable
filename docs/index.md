# PyAwaitable

!!! note

    This project originates from a scrapped PEP. For the original text, see [here](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926).

## Motivation

CPython currently has no existing C interface for writing asynchronous functions or doing any sort of `await` operations, other than defining extension types and manually implementing methods like `__await__` from scratch. This lack of an API can be seen in some Python-to-C transpilers (such as `mypyc`) having limited support for asynchronous code.

In the C API, developers are forced to do one of three things when it comes to asynchronous code:

-   Manually implementing coroutines using extension types.
-   Use an external tool to compile their asynchronous code to C.
-   Defer their asynchronous logic to a synchronous Python function, and then call that natively.

## Introduction

Since there are other event loop implementations, PyAwaitable aims to be a _generic_ interface for working with asynchronous operations from C (as in, we'll only be implementing features like `async def` and `await`, but not things like `asyncio.create_task`.)

This documentation assumes that you're familiar with the C API already, and understand some essential concepts like reference counting (as well as borrowed and strong references). If you don't know what any of that means, it's highly advised that you read through the [Python docs](https://docs.python.org/3/extending/extending.html) before trying to use PyAwaitable.

## Quickstart

Add PyAwaitable as a build dependency:

```toml
# pyproject.toml
[build-system]
requires = ["your_preferred_build_system", "pyawaitable>=2.0.0"]
build-backend = "your_preferred_build_system.build"
```

Use it in your extension:

```c
/*
 Equivalent to the following Python function:

 async def async_function(coro: collections.abc.Awaitable) -> None:
    await coro

 */
static PyObject *
async_function(PyObject *self, PyObject *coro)
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

!!! note

    You need to call `PyAwaitable_Init` upon initializing your extension! This can be done in the `PyInit_` function, or a module-exec function if using [multi-phase initialization](https://docs.python.org/3/c-api/module.html#initializing-c-modules).

## Acknowledgements

Special thanks to:

-   [Petr Viktorin](https://github.com/encukou), for his feedback on the initial API design and PEP.
-   [Sean Hunt](https://github.com/AraHaan), for beta testing.
