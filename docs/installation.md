# Installation

## Build Dependency

PyAwaitable needs to be installed as a build dependency, not a runtime dependency. For the `project.dependencies` portion of your `pyproject.toml`, you can completely omit PyAwaitable as a dependency!

For example, in a `setuptools` based project, your `build-system` section would look like:

```toml
# pyproject.toml
[build-system]
requires = ["setuptools~=78.1", "pyawaitable>=2.0.0"]
build-backend = "setuptools.build_meta"
```

## Including the `pyawaitable.h` File

PyAwaitable provides a number of APIs to access the include path for a number of different build systems. Namely:

-   `pyawaitable.include()` is available in Python code (typically for `setup.py`-based extensions).
-   `PYAWAITABLE_INCLUDE` is accessible as an environment variable, but only if Python has been started without `-S` (this is useful for `scikit-build-core` projects).
-   `pyawaitable --include` returns the path of the include directory (useful for everything else, such as `meson-python`).

!!! note

    PyAwaitable uses a nifty trick for building itself into your project. Python's packaging ecosystem isn't exactly great at distributing C libraries, so the `pyawaitable.h` actually contains the entire PyAwaitable source code (but with mangled names to prevent collisions with your own project).

## Initializing PyAwaitable in Your Extension

PyAwaitable has to do a one-time initialization to get its types and other state initialized in the Python process. This is done with `PyAwaitable_Init`, which can be called basically anywhere, as long as its called before any other PyAwaitable functions are used.

Typically, you'll want to call this in your extension's `PyInit_` function, or in the module-exec function in multi-phase extensions. For example:

```c
// Single-phase
PyMODINIT_FUNC
PyInit_mymodule()
{
    if (PyAwaitable_Init() < 0) {
        return NULL;
    }

    return PyModule_Create(/* ... */);
}
```

```c
// Multi-phase
static int
module_exec(PyObject *mod)
{
    return PyAwaitable_Init();
}
```

!!! warning "No Limited API Support"

    Unfortunately, PyAwaitable cannot be used with the [limited C API](https://docs.python.org/3/c-api/stable.html#limited-c-api). This is due to PyAwaitable needing [am_send](https://docs.python.org/3/c-api/typeobj.html#c.PyAsyncMethods.am_send) to implement the coroutine protocol on 3.10+, but the corresponding heap-type slot `Py_am_send` was not added until 3.11. Therefore, PyAwaitable cannot support the limited API without dropping support for <3.11.

## Examples

### `setuptools`

```py
# setup.py
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

```py
# meson.build
project('_module', 'c')

py = import('python').find_installation(pure: false)
pyawaitable_include = run_command('pyawaitable --include', check: true).stdout().strip()

py.extension_module(
    'src/module.c',
    install: true,
    include_directories: [pyawaitable_include],
)
```

## Simple Extension Example

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
