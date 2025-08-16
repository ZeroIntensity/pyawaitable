# Making a C Function Asynchronous

Let's make a C function that replicates the following Python code:

```py
async def hello() -> None:
    print("Hello, PyAwaitable")
```

If you've tried to implement an asynchronous C function in the past, this is likely where you got stuck. How do we make a C function `async`?

## Breaking Down Awaitable Functions

In Python, you have to _call_ an `async def` function to use it with `await`. In our example above, the following would be invalid:

```py
>>> await hello
```

Of course, you need to do `await hello()` instead. `hello()` is returning a _coroutine_, and coroutine objects are usable with the `await` keyword. So, `hello` as a synchronous function would really be like:

```py
class _hello_coroutine:
    def __await__(self) -> collections.abc.Generator:
        print("Hello, PyAwaitable")
        yield

def hello() -> collections.abc.Coroutine:
    return _hello_coroutine()
```

If there were to be `await` expressions inside `hello`, the returned coroutine object would handle those by yielding inside of the `__await__` dunder method. We can do the same kind of thing in C.

## Creating PyAwaitable Objects

You can create a new PyAwaitable object with `PyAwaitable_New`. This returns a _strong reference_ to a PyAwaitable object, and `NULL` with an exception set on failure.

Think of a PyAwaitable object sort of like the `_hello_coroutine` example from above, but it's _generic_ instead of being special for `hello`. So, like our Python example, we need to return the coroutine to allow it to be used in `await` expressions:

```c
static PyObject *
hello(PyObject *self, PyObject *nothing) // METH_NOARGS
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    puts("Hello, PyAwaitable");
    return awaitable;
}
```

!!! note "There's a difference between native coroutines and implemented coroutines"

    "Coroutine" is a bit of an ambigious term in Python. There are two types of coroutines: native ones ([`types.CoroutineType`](https://docs.python.org/3/library/types.html#types.CoroutineType)), and objects that implement the *coroutine protocol* ([`collections.abc.Coroutine`](https://docs.python.org/3/library/types.html#types.CoroutineType)). Only the interpreter itself can create native coroutines, so a PyAwaitable object is an object that implements the coroutine protocol.

Yay! We can now use `hello` in `await` expressions:

```py
>>> from _yourmod import hello
>>> await hello()
Hello, PyAwaitable
```

## Changing the Return Value

Note that in all code-paths, we should return the PyAwaitable object, or `NULL` with an exception set to indicate a failure. But that means you can't simply `return` your own value; how can the `await` expression evaluate to something useful?

By default, the "return value" (_i.e._, what `await` will evaluate to) is `None`. That can be changed with `PyAwaitable_SetResult`, which takes a reference to the object you want to return.

For example, if you wanted to return the Python integer `42` from `hello`, you would simply pass that to `PyAwaitable_SetResult`:

```c
static PyObject *
hello(PyObject *self, PyObject *nothing) // METH_NOARGS
{
    PyObject *awaitable = PyAwaitable_New();
    if (awiatable == NULL) {
        return NULL;
    }

    PyObject *my_number = PyLong_FromLong(42);
    if (my_number == NULL) {
        Py_DECREF(awaitable);
        return NULL;
    }

    if (PyAwaitable_SetResult(awaitable, my_number) < 0) {
        Py_DECREF(awaitable);
        Py_DECREF(my_number);
        return NULL;
    }

    Py_DECREF(my_number);

    puts("Hello, PyAwaitable");
    return awaitable;
}
```

Now, the `await` expression evalutes to `42`:

```py
>>> from _yourmod import hello
>>> await hello()
Hello, PyAwaitable
42
```
