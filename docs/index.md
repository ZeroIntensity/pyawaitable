---
hide:
    - navigation
---

# PyAwaitable

!!! note

    This project originates from a scrapped PEP. For the original text, see [here](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926).

CPython currently has no existing C interface for writing asynchronous functions or doing any sort of `await` operations, other than defining extension types and manually implementing methods like `__await__` from scratch. This lack of an API can be seen in some Python-to-C transpilers (such as `mypyc`) having limited support for asynchronous code.

In the C API, developers are forced to do one of three things when it comes to asynchronous code:

-   Manually implementing coroutines using extension types.
-   Use an external tool to compile their asynchronous code to C.
-   Defer their asynchronous logic to a synchronous Python function, and then call that natively.

PyAwaitable aims to provide a _generic_ interface for working with asynchronous primitives only, as there are other event loop implementations.

For this reason, `PyAwaitable` does not provide any interface for executing C blocking I/O, as that would likely require leveraging something in the event loop implementation (in `asyncio`, it would be something like `asyncio.to_thread`).

## Installation

```console
$ pip install pyawaitable
```

You can then use it in your extension module (example with `setuptools`):

```py
from setuptools import setup, Extension
import pyawaitable

if __name__ == "__main__":
    setup(
        ...,
        ext_modules=[
            Extension(
                ...,
                include_dirs=[pyawaitable.include()]
            )
        ]
    )
```

However, if you're distributing a library or something similar, then you want to specify PyAwaitable as a [build dependency](https://peps.python.org/pep-0517/#build-requirements):

```toml
# pyproject.toml example with setuptools
[build-system]
requires = ["setuptools", "pyawaitable"]
build-backend = "setuptools.build_meta"
```

!!! note

    PyAwaitable needs to be installed as both a runtime dependency and build time dependency.

## Vendored Copies

PyAwaitable ships a vendorable version of each release, containing both a `pyawaitable.c` and `pyawaitable.h` file. For many user, it's much easier to vendor PyAwaitable than use it off PyPI.

Initialization is slightly different on a vendored version - instead of using `pyawaitable_init`, you are to use `pyawaitable_vendor_init`, which takes a module object. For example:

```c
#include "pyawaitable.h"
/* ... */

PyMODINIT_FUNC
PyInit_foo(void)
{
    PyObject *m = PyModule_Create(/* ... */);
    if (!m)
        return NULL;

    if (pyawaitable_init_vendor(m) < 0)
    {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
```

## Acknowledgements

Special thanks to:

-   [Petr Viktorin](https://github.com/encukou), for his feedback on the initial API design.
-   [Sean Hunt](https://github.com/AraHaan), for beta testing.
