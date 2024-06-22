---
hide:
    - navigation
---

# Storing and Fetching Values

## Basics

Every `PyAwaitableObject*` will contain an array of strong references to `PyObject*`'s, as well as an array of `void*` pointers. Both of these arrays are separate, and deallocated or `Py_DECREF`'d at the end of the object's lifetime.

The two public interfaces for saving values are `pyawaitable_save` and `pyawaitable_save_arb`. These functions are variadic, and are supplied a `nargs` parameter specifying the number of values.

## Arbitrary vs Normal Values

"Normal value" refers to a `PyObject*`, while "arbitrary value" refers to a `void*`. The main difference when using them is that the awaitable will increment the reference count of a normal value, while an arbitrary value has all memory management deferred to the user.

-   `pyawaitable_save` and `pyawaitable_unpack` are both about normal values, so they will increment and decrement reference counts accordingly.
-   `pyawaitable_save_arb` and `pyawaitable_unpack_arb` are both about _arbitrary_ values, so they will not try to `Py_DECREF` nor `free` the pointer.

## Example

```c
static int
spam_callback(PyObject *awaitable, PyObject *result)
{
    PyObject *value;
    if (pyawaitable_unpack(awaitable, &value) < 0)
        return -1;

    long a = PyLong_AsLong(result);
    long b = PyLong_AsLong(value);
    if (PyErr_Occurred())
        return -1;

    PyObject *ret = PyLong_FromLong(a + b);
    if (ret == NULL)
        return -1;

    if (pyawaitable_set_result(awaitable, ret) < 0)
    {
        Py_DECREF(ret);
        return -1;
    }
    Py_DECREF(ret);

    return 0;
}

static PyObject *
spam(PyObject *awaitable, PyObject *args)
{
    PyObject *value;
    PyObject *coro;

    if (!PyArg_ParseTuple(args, "OO", &value, &coro))
        return NULL;

    PyObject *awaitable = pyawaitable_new();
    if (awaitable == NULL)
        return NULL;

    if (pyawaitable_save(awaitable, 1, value) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }

    if (pyawaitable_await(awaitable, coro, spam_callback, NULL) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }

    return awaitable;
}
```

```py
# Assuming top-level await
async def foo():
    await ...  # Pretend to do some blocking I/O
    return 39

await spam(3, foo())  # 42
```
