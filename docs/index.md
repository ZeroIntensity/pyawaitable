# PyAwaitable

!!! note

  This project originates from a scrapped PEP. For the original text, see [here](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926).

## Motivation

CPython currently has no existing C interface for writing asynchronous functions or doing any sort of ``await`` operations, other than defining extension types and manually implementing methods like ``__await__`` from scratch. This lack of an API can be seen in some Python-to-C transpilers (such as ``mypyc``) having limited support for asynchronous code.

In the C API, developers are forced to do one of three things when it comes to asynchronous code:

- Manually implementing coroutines using extension types.
- Use an external tool to compile their asynchronous code to C.
- Defer their asynchronous logic to a synchronous Python function, and then call that natively.

This API aims to provide a *generic* interface for working with asynchronous primitives only, as there are other event loop implementations.

For this reason, ``PyAwaitable`` does not provide any interface for executing C blocking I/O, as that would likely require leveraging something in the event loop implementation (in ``asyncio``, it would be something like ``asyncio.to_thread``).

## Installation

```console
$ pip install pyawaitable
```

You can then use it in your extension module as such:

```py
from setuptools import setup, Extension
import pyawaitable

if __name__ == "__main__":
    setup(
        ...,
        ext_modules=[
            Extension(
                ...,
                include_dirs=[pyawaitable.include()],
                library_dirs=[pyawaitable.lib()]
            )
        ]
    )
```
