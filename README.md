# PyAwaitable

## Call asynchronous code from an extension module

[![Build](https://github.com/ZeroIntensity/pyawaitable/actions/workflows/build.yml/badge.svg)](https://github.com/ZeroIntensity/pyawaitable/actions/workflows/build.yml)
![Tests](https://github.com/ZeroIntensity/pyawaitable/actions/workflows/tests.yml/badge.svg)

-   [Docs](https://pyawaitable.zintensity.dev)
-   [Source](https://github.com/ZeroIntensity/pyawaitable)
-   [PyPI](https://pypi.org/project/pyawaitable)

## What is it?

PyAwaitable is the _only_ library to support defining and calling asynchronous Python functions from pure C code.

It was originally designed to be directly part of CPython; you can read the [scrapped PEP](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926) about it. But, since this library only uses the public ABI, it's better fit outside of CPython, as a library.

## Installation

Add it to your project's build process:

```toml
# pyproject.toml example with setuptools
[build-system]
requires = ["setuptools", "pyawaitable"]
build-backend = "setuptools.build_meta"
```

Include it in your extension:

```py
from setuptools import setup, Extension
import pyawaitable

if __name__ == "__main__":
    setup(
        ...,
        ext_modules=[Extension(..., include_dirs=[pyawaitable.include()])]
    )
```

## Example

```c
/*
 Equivalent to the following Python function:

 async def async_function(coro: collections.abc.Awaitable) -> None:
    await coro

 */
static PyObject *
async_function(PyObject *self, PyObject *coro)
{
    // Create our transport between the C world and the asynchronous world.
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    // Mark our Python coroutine, *coro*, for being executed by the event loop.
    if (PyAwaitable_AddAwait(awaitable, coro, NULL, NULL) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    // Return our transport, allowing *coro* to be eventually executed.
    return awaitable;
}
```

## Copyright

`pyawaitable` is distributed under the terms of the [MIT](https://spdx.org/licenses/MIT.html) license.
