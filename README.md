# PyAwaitable

## Call asynchronous code from an extension module

[![Build Wheels](https://github.com/ZeroIntensity/pyawaitable/actions/workflows/build.yml/badge.svg)](https://github.com/ZeroIntensity/pyawaitable/actions/workflows/build.yml)
![Tests](https://github.com/ZeroIntensity/pyawaitable/actions/workflows/tests.yml/badge.svg)
![Memory Leak](https://github.com/ZeroIntensity/pyawaitable/actions/workflows/memory_leak.yml/badge.svg)

-   [Docs](https://awaitable.zintensity.dev)
-   [Source](https://github.com/ZeroIntensity/pyawaitable)
-   [PyPI](https://pypi.org/project/pyawaitable)

## What is it?

PyAwaitable is the *only* library to support writing and calling asynchronous Python functions from pure C code (with the exception of manually implementing an awaitable class from scratch, which is essentially what PyAwaitable does).

It was originally designed to be directly part of CPython - you can read the [scrapped PEP](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926) about it. Since this library only uses the public ABI, it's better fit outside of CPython, as a library.

## Installation

Add it to your project's build process:

```toml
# pyproject.toml example with setuptools
[build-system]
requires = ["setuptools", "pyawaitable"]
build-backend = "setuptools.build_meta"

[project]
# ...
dependencies = ["pyawaitable"]
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
#define PYAWAITABLE_PYAPI
#include <pyawaitable.h>

// Assuming that this is using METH_O
static PyObject *
hello(PyObject *self, PyObject *coro) {
    // Make our awaitable object
    PyObject *awaitable = PyAwaitable_New();

    if (awaitable == NULL) {
        return NULL;
    }

    // Mark the coroutine for being awaited
    if (PyAwaitable_AddAwait(awaitable, coro, NULL, NULL) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    // Return the awaitable object to yield to the event loop
    return awaitable;
}
```

```py
# Assuming top-level await
async def coro():
    await asyncio.sleep(1)
    print("awaited from C!")

# Use our C function to await it
await hello(coro())
```

## Copyright

`pyawaitable` is distributed under the terms of the [MIT](https://spdx.org/licenses/MIT.html) license.
