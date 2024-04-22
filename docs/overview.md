# Overview

## API

!!! note

    For all functions returning ``int``, ``0`` is a successful result and ``-1`` is a failure, per the existing CPython ABI.

PyAwaitable provides a suite of API functions under the prefix of ``awaitable_``, as well as a ``AwaitableObject`` structure along with a ``AwaitableType`` (known in Python as ``_awaitable``). This is an object that implements ``collections.abc.Coroutine``.

!!! warning

    While an `AwaitableObject*` *implements* `collections.abc.Coroutine`, it is *not* a `type.CoroutineType`. This means that performing `inspect.iscoroutine(awaitable)` will return `False`. Instead, use `isinstance(awaitable, collections.abc.Coroutine)`.

The list of public functions is as follows:

- ``#!c PyObject *awaitable_new()``
- ``#!c void awaitable_cancel(PyObject *aw)``
- ``#!c int awaitable_await(PyObject *aw, PyObject *coro, awaitcallback cb, awaitcallback_err err)``
- ``#!c int awaitable_set_result(PyObject *awaitable, PyObject *result)``
- ``#!c int awaitable_save(PyObject *awaitable, Py_ssize_t nargs, ...)``
- ``#!c int awaitable_save_arb(PyObject *awaitable, Py_ssize_t nargs, ...)``
- ``#!c int awaitable_unpack(PyObject *awaitable, ...)``
- ``#!c int awaitable_unpack_arb(PyObject *awaitable, ...)``

!!! tip

    If you would like to use the `PyAwaitable_` prefix as if it was part of the CPython ABI, define the `PYAWAITABLE_PYAPI` macro before including `awaitable.h`:

    ```c
    #define PYAWAITABLE_PYAPI
    #include <awaitable.h> // e.g. awaitable_init can now be used via PyAwaitable_Init
    ```

PyAwaitable also comes with these typedefs:

```c
typedef int (*awaitcallback)(PyObject *, PyObject *);
typedef int (*awaitcallback_err)(PyObject *, PyObject *);
typedef struct _AwaitableObject AwaitableObject;
```

## Lifecycle

!!! note

    This section is not critical to knowing how to use PyAwaitable. It simply explains how it works.

An ``AwaitableObject*`` stores an array of strong references to coroutines, which are then yielded to the event loop by an iterator returned by the ``AwaitableObject*``'s ``__await__``. This is done with an extra type, called ``GenWrapper`` (in the API defined as ``AwaitableGenWrapperType``), which will defer the result to the ``__next__`` of the coroutine iterator currently being executed.

The lifecycle of this process is as follows:

- ``__await__`` on the ``AwaitableObject*`` is called, returns an iterator (``GenWrapperObject*``).
- This iterator must somehow contain the current coroutine being executed (this is the state/index), as well as the array of coroutines added by the user. 
- Upon ``__next__`` being called on this iterator, if no coroutine is being executed, the ``__await__`` is called on the coroutine at the current index (the state), and the state is incremented. ``__next__`` does not return after this step.
- Inside of ``__next__``, the ``__next__`` is called on the current coroutine iterator (the result of ``coro.__await__()``, once again). If the object has no ``__await__`` attribute, a ``TypeError`` is raised.
- When the coroutine iterator raises ``StopIteration``, the callback for the current coroutine (if it exists) is called with the value and sets the current iterator to ``NULL``.
- This process is repeated inside ``__next__`` calls until the end of the coroutine array is reached. Note that coroutine callbacks may add extra coroutines to be awaited during their execution.
- Finally, once the final coroutine is done, the iterator must raise ``StopIteration`` with the return value upon the next call to ``__next__``. After a ``StopIteration`` has been raised, then the awaitable object is marked as done. At this point, a ``RuntimeError`` should be raised upon trying to call ``__next__``, ``__await__``, or any ``awaitable*`` functions.
- If at any point during this process, an exception is raised (including by coroutine result callbacks), the error callback of the coroutine being executed is called with the raised exception. If no error callback is set, then `__next__` throws the error to the event loop.
