# Storing and Fetching Values

Every ``AwaitableObject*`` will contain an array of strong references to ``PyObject*``'s, as well as an array of ``void*`` (referred to as arbitrary values here). Both of these arrays are separate, and deallocated at the end of the object's lifetime. ``awaitable_save*`` functions are the public functions for saving values to a ``AwaitableObject*``. ``awaitable_save*`` functions append to the existing array if called multiple times. These functions are varadic, and are supplied a ``nargs`` parameter specifying the number of values. 

An example of saving and unpacking values is shown below:

```c
static int
spam_callback(PyObject *awaitable, PyObject *result)
{
    PyObject *value;
    if (awaitable_unpack(awaitable, &value) < 0)
        return -1;

    long a = PyLong_AsLong(result);
    long b = PyLong_AsLong(value);
    if (PyErr_Occurred())
        return -1;

    PyObject *ret = PyLong_FromLong(a + b);
    if (ret == NULL)
        return -1;

    if (awaitable_set_result(awaitable, ret) < 0)
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

    PyObject *awaitable = awaitable_new();
    if (awaitable == NULL)
        return NULL;

    if (awaitable_save(awaitable, 1, value) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }

    if (awaitable_await(awaitable, coro, spam_callback, NULL) < 0)
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
