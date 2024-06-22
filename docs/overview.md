---
hide:
    - navigation
---

# Overview

## Initialization

After installing PyAwaitable, you need to initialize the ABI.

!!! tip

    You can check if the ABI is initialized via a `NULL` check on `pyawaitable_abi`

    ```c
    assert(pyawaitable_abi != NULL);
    ```

One file in your project should define `PYAWAITABLE_THIS_FILE_INIT`, and this file should call `pyawaitable_init`. Ideally, this will be in your module initialization function (`PyInit_*`):

```c
#define PYAWAITABLE_THIS_FILE_INIT
#include <pyawaitable.h>

PyMODINIT_FUNC
PyInit_foo() {
    if (pyawaitable_init() < 0)
        return NULL;

    return PyModule_Create(/* */);
}
```

This will then store the capsule pointer as a global on the module so then it can be accessed in every source file at once.

## API

!!! note

    For all functions returning ``int``, ``0`` is a successful result and ``-1`` is a failure, per the existing CPython ABI.

PyAwaitable provides a suite of API functions under the prefix of `pyawaitable_`, as well as a `PyAwaitableObject` structure along with a `PyAwaitableType` (known in Python as `pyawaitable.PyAwaitable`). This is an object that implements `collections.abc.Coroutine`.

!!! warning

    While a `PyAwaitableObject*` *implements* `collections.abc.Coroutine`, it is *not* a `types.CoroutineType`. This means that performing `inspect.iscoroutine(awaitable)` will return `False`. Instead, use `isinstance(awaitable, collections.abc.Coroutine)`.

The list of public functions is as follows:

-   `#!c PyObject *pyawaitable_new()`
-   `#!c void pyawaitable_cancel(PyObject *aw)`
-   `#!c int pyawaitable_await(PyObject *aw, PyObject *coro, awaitcallback cb, awaitcallback_err err)`
-   `#!c int pyawaitable_set_result(PyObject *awaitable, PyObject *result)`
-   `#!c int pyawaitable_save(PyObject *awaitable, Py_ssize_t nargs, ...)`
-   `#!c int pyawaitable_save_arb(PyObject *awaitable, Py_ssize_t nargs, ...)`
-   `#!c int pyawaitable_unpack(PyObject *awaitable, ...)`
-   `#!c int pyawaitable_unpack_arb(PyObject *awaitable, ...)`

!!! tip

    If you would like to use the `PyAwaitable_` prefix as if it was part of the CPython ABI, define the `PYAWAITABLE_PYAPI` macro before including `pyawaitable.h`:

    ```c
    #define PYAWAITABLE_PYAPI
    #include <awaitable.h> // e.g. pyawaitable_init can now be used via PyAwaitable_Init
    ```

PyAwaitable also comes with these typedefs:

```c
typedef int (*awaitcallback)(PyObject *, PyObject *);
typedef int (*awaitcallback_err)(PyObject *, PyObject *);
typedef struct _PyAwaitableObject PyAwaitableObject;
```

!!! info "Calling Conventions"

    PyAwaitable distributes it's functions via [capsules](https://docs.python.org/3/extending/extending.html#using-capsules), meaning that the PyAwaitable library file doesn't actually export the functions above. Instead, calling `pyawaitable_init` initializes the ABI in your file. This means that, for example, `pyawaitable_new` is *actually* defined as:
    ```c
    #define pyawaitable_new pyawaitable_abi->pyawaitable_new
    ```

    For information why this is the behavior, see [this discussion](https://discuss.python.org/t/linking-against-installed-extension-modules/51710).
