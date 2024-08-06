---
hide:
    - navigation
---

# Storing and Fetching Values

## Introduction

Generally, many people get around this with something like a `void *arg` parameter, but PyAwaitable has a better solution. We call it "value storage."

Value storage in PyAwaitable is simply packing and unpacking of an array.
Specifically, we have three different types of values that we can store.

## Saving

Let's start with the very basics, with what we call "normal values," which are Python objects (or more technically, `PyObject` pointers). We load Python objects into PyAwaitable by calling `pyawaitable_save` with the number of arguments, followed by variadic parameters.

For tutorial purposes, let's come back to our `test` method. This time, it will be `METH_VARARGS`, so we can take multiple arguments:

```c
static PyObject *
test(PyObject *self, PyObject *args)
{
    return pyawaitable_new(); // To make it async, for now.
}

static PyMethodDef methods[] = {
    {"test", test, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL} // sentinel
}
```

OK, now let's add a call to `pyawaitable_save`:

```c
static PyObject *
test(PyObject *self, PyObject *args)
{
    PyObject *coro;
    PyObject *something_else;

    if (!PyArg_ParseTuple(args, "OO", &coro, &something_else))
        return NULL;

    PyObject *aw = pyawaitable_new();

    // We'll need the callback here in a moment.
    // For now, just focus on the save() call.
    if (pyawaitable_await(aw, coro, callback, NULL) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    if (pyawaitable_save(aw, 1, something_else) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    return aw;
}
```

Perfect! We've saved `something_else` into our PyAwaitable object (and in turn, incremented it's reference count).

!!! warning "Size Limit"

    For performance reasons, value arrays are preallocated, with a size of `32` (*i.e.*, you cannot store more than `32` values). If you try to overflow the size, a `ValueError` is raised by `pyawaitable_save`. If you would like to increase this limit, download a vendorable copy of PyAwaitable, and change the value of the `CALLBACK_ARRAY_SIZE` in the source code.

We can call `pyawaitable_save` multiple times to append to the internal value array.

```c
if (pyawaitable_save(aw, 1, something_else) < 0)
{
    /* ... */
    return NULL;
}

if (pyawaitable_save(aw, 2, foo, bar) < 0)
{
    /* ... */
    return NULL
}

if (pyawaitable_save(aw, 1, baz) < 0)
{
    /* ... */
    return NULL;
}
```

In the above, our value array would be of size `4`. However, the above would be considered bad practice! You should always try to only call `pyawaitable_save` once per function, and just pass as many arguments as you need in the one call. Our example above could be refactored to:

```c
if (pyawaitable_save(aw, 4, something_else, foo, bar, baz) < 0)
{
    /* ... */
    return NULL;
}
```

## Reference Counting

Contrary to what your instincts might be telling you, `pyawaitable_save` _does not steal references_. In our previous example, we didn't needed to `Py_DECREF` anything because they are borrowed references. If we have a strong reference, then not `Py_DECREF`'ing an argument would be a reference leak! For example, the following would **not** be valid:

```c
PyObject *str = PyUnicode_FromString("abc");
if (pyawaitable_save(aw, 1, str) < 0)
{
    Py_DECREF(aw);
    return NULL;
    // Wrong! We need to Py_DECREF(str) here!
}

// We need a Py_DECREF(str) here as well, if we're done using it.
```

Again, this is only in the case of a _strong_ reference. Object pointers obtained by something like `PyArg_ParseTuple` are borrowed, and hence do not need to be `Py_DECREF`'d.

!!! note

    Note that most functions in the C API return strong references! If you're unsure about what a function returns, check the [Python documentation](https://docs.python.org/3/c-api/index.html).

## Unpacking

Now how do we do anything with it? We can unpack it in a callback via `pyawaitable_unpack`, by passing it pointers to other (typically, local) `PyObject *` pointers. This number has to be the same as the number we passed to `pyawaitable_save` originally. For example, if we take our example from earlier where we stored `something_else`, we could unpack it in `callback` as such:

```c
static int
callback(PyObject *aw, PyObject *result)
{
    PyObject *something_else;
    if (pyawaitable_unpack(aw, &something_else) < 0)
        return -1;

    // something_else is now a borrowed reference to what
    // we stored in our original function!

    /* ... */

    return 0;
}
```

Per the comment above, `pyawaitable_unpack` unpacks each pointer into _borrowed_ references. PyAwaitable deals with decrementing them when _it_ is deallocated.

You may have also noticed that we don't pass the number of arguments to unpack. Both you (the developer) and PyAwaitable already know at this point how many values you saved! If you saved four values, you need to pass four pointers to unpack four values.

However, if you pass a `NULL` pointer to `pyawaitable_unpack`, we skip that value. For example, in our large example from earlier, if we only wanted to unpack `foo` and `baz`, it would look like:

```c
PyObject *foo;
PyObject *baz;

if (pyawaitable_unpack(aw, NULL, &foo, NULL, &baz) < 0)
    return -1;
```

## Arbitrary Values

So far, we've only talked about values that are first-class Python objects, that can be `Py_INCREF`'d and `Py_DECREF`'d accordingly. But what about if you have a value is just, in general, something else? PyAwaitable refers to this as _arbitrary_ value storage.

Arbitrary value storage is nearly identical to object storage, except that PyAwaitable _does not_ manage that memory for you. You're responsible for dealing with the state of that memory!

As an example, let's try and save a `malloc`'ed string:

```c
static PyObject *
test(PyObject *self, PyObject *coro) // We're back to METH_O!
{
    PyObject *aw = pyawaitable_new();

    if (pyawaitable_await(aw, coro, callback, NULL) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    char *str = PyMem_Malloc(16); // PyMalloc for this example
    if (str == NULL) {
        Py_DECREF(aw);
        return PyErr_NoMemory();
    }

    // Notice that this is pyawaitable_save_arb!
    if (pyawaitable_save_arb(aw, 1, str) < 0)
    {
        free(str);
        Py_DECREF(aw);
        return NULL;
    }

    return aw;
}
```

Great! As you can see above, we use `pyawaitable_save_arb` to save generic `void *` pointers, **not** `pyawaitable_save`.

It's also worth noting that the arbitrary values and "normal" values are totally unconnected (as in, they are in different arrays). You could save 10 arbitrary values, then 4 normal values, and you wouldn't have to change anything. Use `pyawaitable_unpack_arb` to unpack arbitrary values, and `pyawaitable_unpack` to unpack object values. As an example:

```c
static int
callback(PyObject *aw, PyObject *result)
{
    char *str;
    // Notice the use of pyawaitable_unpack_arb, instead of
    // the usual pyawaitable_unpack when using object values.
    if (pyawaitable_unpack_arb(aw, &str) < 0)
        return -1;

    // Do something with str

    PyObject *foo;
    if (pyawaitable_unpack(aw, &foo, NULL, NULL, NULL) < 0)
        return -1;

    return 0;
}
```

However, we forgot something! `str` was allocated by `malloc`, so this is leaked memory. We need to `free()` it. But won't that cause a use-after-free error? Nope, since PyAwaitable never actually tries to touch a `void *` that you give it. In fact, if you freed `str`, you could unpack it just fine!

In fact, this is the workflow that you should follow for your PyAwaitable callbacks. _You_ are the only person using that callback, you know what will be called next. It's your job to get that logic together and figure out where to free your memory.

## Integer Values

Finally, we have integer values. These are pretty self explanatory: instead of storing a `PyObject *`, or a `void *`, we can store general integers (these are stored as `long`s internally).

Once again, integer values are seperate from object and arbitrary values, and stored in seperate arrays. For example, if you store 3 arbitrary values, it won't affect how you save and unpack 4 other integer values.

As a quick example, we can store some flags:

```c
static PyObject *
test(PyObject *self, PyObject *coro) // We're back to METH_O!
{
    PyObject *aw = pyawaitable_new();

    if (pyawaitable_await(aw, coro, callback, NULL) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    int val = MY_FLAG | SOME_OTHER_FLAG;

    // Notice that this is pyawaitable_save_int!
    if (pyawaitable_save_int(aw, 1, val) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    return aw;
}
```

Which, of course, is unpacked like so, via `pyawaitable_unpack_int`:

```c
static int
callback(PyObject *aw, PyObject *result)
{
    int flags;
    if (pyawaitable_unpack_int(aw, &flags) < 0)
        return -1;

    /* ... */

    return 0;
}
```

### Bypassing the integer limit

As stated previously, this is stored as a `long`, meaning you'll have some overflow problems if you try to use larger integer types. To get around this, you can just allocate it as heap memory, and save it as an arbitrary values (do note that this has to be freed, of course):

```c
static PyObject *
test(PyObject *self, PyObject *coro) // We're back to METH_O!
{
    PyObject *aw = pyawaitable_new();

    if (pyawaitable_await(aw, coro, callback, NULL) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    long long val = /* ... */;
    long long *mem = malloc(sizeof(long long));
    *mem = val;

    if (pyawaitable_save_arb(aw, 1, mem) < 0)
    {
        Py_DECREF(aw);
        free(mem);
        return NULL;
    }

    return aw;
}
```

## Getting and Setting

In some cases, you might want to overwrite existing saved values (e.g. in a recursive callback). For example, we could have some state information, and want to change that between calls.

Let's start in a callback that uses itself recursively:

```c
static int
callback(PyObject *aw, PyObject *result)
{
    if (!PyCoro_CheckExact(result)) {
        return 0;
    }

    if (pyawaitable_await(aw, result, callback, NULL) < 0)
        return -1;

    return 0;
}

static PyObject *
test(PyObject *self, PyObject *coro) // We're back to METH_O!
{
    PyObject *aw = pyawaitable_new();

    if (pyawaitable_await(aw, coro, callback, NULL) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    return aw;
}
```

Theoretically, `callback` could be called an infinite number of times for the same PyAwaitable object, so we don't know what the state of the call is!

OK, let's start by saving an integer value:

```c
static PyObject *
test(PyObject *self, PyObject *coro) // We're back to METH_O!
{
    PyObject *aw = pyawaitable_new();

    if (pyawaitable_await(aw, coro, callback, NULL) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    if (pyawaitable_save_int(aw, 1, 1) < 0)
    {
        Py_DECREF(aw);
        return NULL;
    }

    return aw;
}
```

But, so far, we've only learned about _appending_ to the values array, not mutating in place. So, how do we do that? Each of the value arrays have their own get and set functions.

In this case, we want `pyawaitable_set_int`:

```c
static int
callback(PyObject *aw, PyObject *result)
{
    long value = pyawaitable_get_int(aw, 0);
    if (value == -1 && PyErr_Occurred())
    {
        return -1;
    }

    if (pyawaitable_set_int(aw, 0, value + 1) < 0)
    {
        return -1;
    }

    if (!PyCoro_CheckExact(result))
    {
        return 0;
    }

    if (pyawaitable_await(aw, result, callback, NULL) < 0)
        return -1;

    return 0;
}
```

Great! We increment our state for each call!

!!! warning "Prefer `unpack` over `get`!"

    In the above, `pyawaitable_get_int` was used for teaching purposes. `pyawaitable_get_*` functions exist for very niche cases, they shouldn't be preferred over `pyawaitable_unpack_*`!

## Next Steps

Congratuilations, you now know how to use PyAwaitable! If you're interested in reading about the internals, be sure to take a look at the [scrapped PEP draft](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926), where this was originally designed to be part of CPython.

Moreover, this project was conceived due to being needed in [view.py](https://github.com/ZeroIntensity/view.py). If you would like to see some very complex examples of PyAwaitable usage, take a look at their [C ASGI implementation](https://github.com/ZeroIntensity/view.py/blob/master/src/_view/app.c#L273), which is powered by PyAwaitable.

```

```
