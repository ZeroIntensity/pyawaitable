# PyAwaitable

## CPython API for asynchronous functions

-   [Docs](https://awaitable.zintensity.dev)
-   [Scrapped PEP](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926)

This project originates from a scrapped PEP. For the original text, see [here](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926).

## Installation

### Console

```console
$ pip install pyawaitable
```

### Add it to your project

```toml
# pyproject.toml example with setuptools
[build-system]
requires = ["setuptools", "pyawaitable"]
build-backend = "setuptools.build_meta"
```

## Example

```c
#include <awaitable.h>

// Assuming that this is using METH_O
static PyObject *
hello(PyObject *self, PyObject *coro) {
    // Make our awaitable object
    PyObject *awaitable = awaitable_new();

    if (!awaitable)
        return NULL;

    // Mark the coroutine for being awaited
    if (awaitable_await(awaitable, coro, NULL, NULL) < 0) {
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

# Use our C function to await coro
await hello(coro())
```

## Copyright

`pyawaitable` is distributed under the terms of the [MIT](https://spdx.org/licenses/MIT.html) license.
