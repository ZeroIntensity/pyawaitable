---
hide:
    - navigation
---

# Installation

## Quickstart

```toml
# pyproject.toml
[build-system]
requires = ["pyawaitable", "setuptools"]
build-backend = "setuptools.build_meta"
```

```py
# setup.py
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

```c
// your_file.c
#include <Python.h>

#define PYAWAITABLE_THIS_FILE_INIT
#include <pyawaitable.h>
/* ... */

PyMODINIT_FUNC
PyInit_foo(void)
{
    if (pyawaitable_init() < 0)
    {
        return NULL;
    }
    return PyModule_Create(/* ... */);
}
```

## Tutorial

_This section is, more or less, an explanation of the quickstart above._

In pretty much all cases, PyAwaitable needs to be installed at both build time and run time. We can do this pretty easily with `pyproject.toml`.

At build time, we need PyAwaitable's header files in our C code, so let's start there:

```toml
# pyproject.toml
[build-system]
requires = ["pyawaitable", "..."]
build-backend = "..."
```

OK, but PyAwaitable is not a build system! We need something to actually build our extensions! Typically, this is done through [setuptools](https://setuptools.pypa.io/en/latest/), but there are many alternatives nowadays. For simplicity, let's just use `setuptools` for now:

```
# pyproject.toml
[build-system]
requires = ["pyawaitable", "setuptools"]
build-backend = "setuptools.build_meta"
```

Perfect! Now we need to setup the extension module for our project. At build time, we don't need any actual library dependencies (we handle this via [capsule magic](https://docs.python.org/3/extending/extending.html#using-capsules)), but you do need the include directory. We can fetch the directory that contains our `pyawaitable.h` via the `include()` function:

```py
from setuptools import setup, Extension
import pyawaitable

if __name__ == "__main__":
    setup(
        name="test",
        ext_modules=[
            Extension(
                "test",
                ["./test.c"]
                include_dirs=[pyawaitable.include()]
            )
        ]
    )
```

Great! PyAwaitable is ready for use. However, before we can do anything with it, we need to initialize it in our extension, which is done pretty easily through `pyawaitable_init`.

```c
// test.c
#include <Python.h>
#include <pyawaitable.h>

/* module boilerplate */

PyMODINIT_FUNC
PyInit_test(void)
{
    if (pyawaitable_init() < 0)
    {
        return NULL;
    }
    return PyModule_Create(/* ... */);
}
```

However, the above would throw an error at runtime (either a linker "undefined symbol" error, or a `SystemError`, depending on whether there are multiple C files in your extension that include `pyawaitable.h`.)

This is due to a small quirk with the PyAwaitable loading process, which is that we need to define `PYAWAITABLE_THIS_FILE_INIT` in the file that calls `pyawaitable_init`:

```c
// test.c
#include <Python.h>

#define PYAWAITABLE_THIS_FILE_INIT
#include <pyawaitable.h>

/* module boilerplate */

PyMODINIT_FUNC
PyInit_test(void)
{
    if (pyawaitable_init() < 0)
    {
        return NULL;
    }
    return PyModule_Create(/* ... */);
}
```

In technical terms, this is due to the fact that the header file relies on an `extern` to initalize the ABI. By adding `PYAWAITABLE_THIS_FILE_INIT`, the extern is removed and ABI symbol is properly exported to the rest of your project.

## Python Prefixes

It may feel a little awkward to call into `pyawaitable_` functions after looking at `Py*` functions for a while. If you want to use PyAwaitable as if it were part of CPython, you can define `PYAWAITABLE_PYAPI` to enable their style of function names.

For example, with our previous `test.c` file:

```c
// test.c
#include <Python.h>

#define PYAWAITABLE_PYAPI
#define PYAWAITABLE_THIS_FILE_INIT
#include <pyawaitable.h>

/* module boilerplate */

PyMODINIT_FUNC
PyInit_test(void)
{
    if (PyAwaitable_Init() < 0)
    {
        return NULL;
    }
    return PyModule_Create(/* ... */);
}
```

The complete mapping of names to their Python API counterparts are:

| Name                         | Python prefix name            |
| ---------------------------- | ----------------------------- |
| `pyawaitable_new`            | `PyAwaitable_New`             |
| `pyawaitable_await`          | `PyAwaitable_AddAwait`        |
| `pyawaitable_await_function` | `PyAwaitable_AwaitFunction`   |
| `pyawaitable_cancel`         | `PyAwaitable_Cancel`          |
| `pyawaitable_set_result`     | `PyAwaitable_SetResult`       |
| `pyawaitable_save`           | `PyAwaitable_SaveValues`      |
| `pyawaitable_save_arb`       | `PyAwaitable_SaveArbValues`   |
| `pyawaitable_save_int`       | `PyAwaitable_SaveIntValues`   |
| `pyawaitable_unpack`         | `PyAwaitable_UnpackValues`    |
| `pyawaitable_unpack_arb`     | `PyAwaitable_UnpackArbValues` |
| `pyawaitable_unpack_int`     | `PyAwaitable_UnpackIntValues` |
| `pyawaitable_set`            | `PyAwaitable_SetValue`        |
| `pyawaitable_set_arb`        | `PyAwaitable_SetArbValue`     |
| `pyawaitable_set_int`        | `PyAwaitable_SetIntValue`     |
| `pyawaitable_get`            | `PyAwaitable_GetValue`        |
| `pyawaitable_get_arb`        | `PyAwaitable_GetArbValue`     |
| `pyawaitable_get_int`        | `PyAwaitable_GetIntValue`     |
| `pyawaitable_init`           | `PyAwaitable_Init`            |
| `pyawaitable_abi`            | `PyAwaitable_ABI`             |
| `PyAwaitableType`            | `PyAwaitable_Type`            |

## Using Precompiled Headers with PyAwaitable

Using precompiled headers in a large C extension project is supported, just replace `PYAWAITABLE_THIS_FILE_INIT` with `PYAWAITABLE_USE_PCH` **BEFORE** including your precompiled header file in every source file.

After that inside of the precompiled header file right after including `pyawaitable.h` add this line:

```c
DECLARE_PYAWAITABLE_ABI;
```

In the source file with the module initialization code:

```c
PYAWAITABLE_INIT_DEF;
```

And then finally after the include of `pch.h` inside of `pch.c` (`pch` is placeholder name for the precompiled header):

```c
DECLARE_PYAWAITABLE_ABI = NULL;
```

After that, all logic from the example code in `test.c` still applies.

## Vendored Copies

PyAwaitable ships a vendorable version of each release, containing both a `pyawaitable.c` and `pyawaitable.h` file. For many users, it's much easier to vendor PyAwaitable than use it off PyPI. You can download the zip file containing a vendorable version [from the releases page](https://github.com/ZeroIntensity/pyawaitable/releases).

Initialization is slightly different on a vendored version. Instead of using `pyawaitable_init`, you are to use `pyawaitable_vendor_init`, which takes a module object. For example:

```c
#include "pyawaitable.h"
/* ... */

PyMODINIT_FUNC
PyInit_foo(void)
{
    PyObject *m = PyModule_Create(/* ... */);
    if (!m)
        return NULL;

    if (pyawaitable_vendor_init(m) < 0)
    {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
```

This is because with a PyPI version, PyAwaitable has it's own module that it can hook, but it needs to hook onto yours when vendoring.

## Version Rules

### Major Versions (Y.x.x)

Between major versions, there may be any changes made to the ABI. However, the legacy ABIs will still be supported for a period of time. In fact, the new ABI will be opt-in for a while (as in, you would have to define a macro to use the new ABI instead of the legacy one).

### Minior Versions (x.Y.x)

Between minor versions, anything may be added to the ABI, but once something is added, it's signature is frozen. Fields will never change type, move around, or be removed _ever_. If one of these changes occur, it would be in a totally new ABI with the major version bumped.

### Micro Versions (x.x.Y)

Nothing will be added to the ABI between micro versions, and will only add security patches to function implementations. To the user, `x.x.0` will feel the same as `x.x.1` (with the exception of bug fixes).
