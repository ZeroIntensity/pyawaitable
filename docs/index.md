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

```toml
# pyproject.toml
[build-system]
requires = ["your_preferred_build_system", "pyawaitable>=2.0.0"]
build-backend = "your_preferred_build_system.build"
```

### `setuptools`

```py
from setuptools import setup, Extension
import pyawaitable

if __name__ == "__main__":
    setup(
        ext_modules=[
            Extension("_module", ["src/module.c"], include_dirs=[pyawaitable.include()])
        ]
    )
```

### `scikit-build-core`

```t
# CMakeLists.txt
cmake_minimum_required(VERSION 3.15...3.30)
project(${SKBUILD_PROJECT_NAME} LANGUAGES C)

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

Python_add_library(_module MODULE src/module.c WITH_SOABI)
target_include_directories(_module PRIVATE $ENV{PYAWAITABLE_INCLUDE})
install(TARGETS _module DESTINATION ${SKBUILD_PROJECT_NAME})
```

### `meson-python`

```lua
project('_module', 'c')

py = import('python').find_installation(pure: false)
pyawaitable_include = run_command('pyawaitable --include', check: true).stdout().strip()

py.extension_module(
    'src/module.c',
    install: true,
    include_directories: [pyawaitable_include],
)
```

### Simple Extension

```c
#include <Python.h>
#include <pyawaitable.h>

static int
module_exec(PyObject *mod)
{
    return PyAwaitable_Init();
}

static PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, module_exec},
    {0, NULL}
};

static PyModuleDef module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_size = 0,
    .m_slots = module_slots,
    .m_methods = module_methods
};

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

static PyMethodDef module_methods[] = {
    {"async_function", async_function, METH_O, NULL},
    {NULL, NULL, 0, NULL},
}

PyMODINIT_FUNC
PyInit__module()
{
    return PyModuleDef_Init(&module);
}
```

## Acknowledgements

Special thanks to:

-   [Petr Viktorin](https://github.com/encukou), for his feedback on the initial API design and PEP.
-   [Sean Hunt](https://github.com/AraHaan), for beta testing.
