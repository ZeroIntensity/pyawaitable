# PyAwaitable

**NOTE:** This README is a scrapped PEP. For the original text, see [here](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926).

## Motivation

CPython currently has no existing C interface for writing asynchronous functions or doing any sort of ``await`` operations, other than defining extension types and manually implementing methods like ``__await__`` from scratch. This lack of an API can be seen in some Python-to-C transpilers (such as ``mypyc``) having limited support for asynchronous code.

In the C API, developers are forced to do one of three things when it comes to asynchronous code:

- Manually implementing coroutines using extension types.
- Use an external tool to compile their asynchronous code to C.
- Defer their asynchronous logic to a synchronous Python function, and then call that natively.

``PyAwaitable`` provides a proper interface for interacting with asynchronous Python code from C.

## Rationale

This API aims to provide a *generic* interface for working with asynchronous primitives only, as there are other event loop implementations.

For this reason, ``PyAwaitable`` does not provide any interface for executing C blocking I/O, as that would likely require leveraging something in the event loop implementation (in ``asyncio``, it would be something like ``asyncio.to_thread``).

## C API

**NOTE:** For all functions returning ``int``, ``0`` is a successful result and ``-1`` is a failure, per the existing CPython ABI.

PyAwaitable adds a suite of API functions under the prefix of ``awaitable_``, as well as a new ``AwaitableObject`` structure along with a ``AwaitableType`` (known in Python as ``awaitable``). This is an object that implements ``collections.abc.Coroutine``. The list of functions is as follows:

- ``PyObject *awaitable_new()``
- ``void awaitable_cancel(PyObject *aw)``
- ``int awaitable_await(PyObject *aw, PyObject *coro, awaitcallback cb, awaitcallback_err err)``
- ``int awaitable_set_result(PyObject *awaitable, PyObject *result)``
- ``int awaitable_save(PyObject *awaitable, Py_ssize_t nargs, ...)``
- ``int awaitable_save_arb(PyObject *awaitable, Py_ssize_t nargs, ...)``
- ``int awaitable_unpack(PyObject *awaitable, ...)``
- ``int awaitable_unpack_arb(PyObject *awaitable, ...)``

PyAwaitable also adds these typedefs:

::

    typedef int (*awaitcallback)(PyObject *, PyObject *);
    typedef int (*awaitcallback_err)(PyObject *, PyObject *);
    typedef struct _AwaitableObject AwaitableObject;

## Overview

An ``AwaitableObject*`` stores an array of strong references to coroutines, which are then yielded to the event loop by an iterator returned by the ``AwaitableObject*``'s ``__await__``. This is done with an extra type, called ``_GenWrapper`` (in the API defined as ``_Awaitable_GenWrapper_Type``), which will defer the result to the ``__next__`` of the coroutine iterator currently being executed.

The lifecycle of this process is as follows:

- ``__await__`` on the ``AwaitableObject*`` is called, returns an iterator (``GenWrapperObject*``).
- This iterator must somehow contain the current coroutine being executed (this is the state/index), as well as the array of coroutines added by the user. 
- Upon ``__next__`` being called on this iterator, if no coroutine is being executed, the ``__await__`` is called on the coroutine at the current index (the state), and the state is incremented. ``__next__`` does not return after this step.
- Inside of ``__next__``, the ``__next__`` is called on the current coroutine iterator (the result of ``coro.__await__()``, once again). If the object has no ``__await__`` attribute, a ``TypeError`` is raised.
- When the coroutine iterator raises ``StopIteration``, the callback for the current coroutine (if it exists) is called with the value and sets the current iterator to a ``NULL``-like value (any value denoting "no coroutine iterator is being executed". This is ``NULL`` in the reference implementation).
- This process is repeated inside ``__next__`` calls until the end of the coroutine array is reached. Note that coroutine callbacks may add extra coroutines to be awaited during their execution.
- Finally, once the final coroutine is done, the iterator must raise ``StopIteration`` with the return value upon the next call to ``__next__``. After a ``StopIteration`` has been raised, then the awaitable object is marked as done. At this point, a ``RuntimeError`` should be raised upon trying to call ``__next__``, ``__await__``, or any ``PyAwaitable*`` functions.
- If at any point during this process, an exception is raised (including by coroutine result callbacks), the error callback of the coroutine being executed is called with the raised exception.

## Semantics

Returning an ``AwaitableObject*`` from a C function will mimic a Python function defined with ``async def``. This means it supports using ``await`` on the result of the function, as well as functions relating to coroutine operations, such as ``asyncio.run``, regardless of whether it has any coroutines stored. The public interface for creating an ``AwaitableObject*`` is ``awaitalbe_new``, which may return a ``PyObject*`` or ``NULL``.

An example of basic usage would look like:

::

    static PyObject *
    spam(PyObject *self, PyObject *args)
    {
        PyObject *awaitable = awaitable_new();
        return awaitable;
    }

::

    # Assuming top-level await for simplicity
    await spam()


## Adding Coroutines

The public interface for adding a coroutine to be executed by the event loop is ``awaitable_await``, which takes four parameters:

- ``aw`` is the ``AwaitableObject*``.
- ``coro`` is the coroutine (or again, any object supporting ``__await__``). This is not checked by this function, but is checked later inside the ``__next__`` of the ``AwaitableObject*``'s coroutine iterator). This is not a function defined with ``async def``, but instead the return value of one (called without ``await``). This value is stored for the lifetime of the object, or until ``awaitable_cancel`` is called.
- ``cb`` is the callback that will be run with the result of ``coro``. This may be ``NULL``, in which case the result will be discarded.
- ``err`` is a callback in the event that an exception occurs during the execution of ``coro``. This may be ``NULL``, in which case the error is simply raised.

The awaitable is guaranteed to yield (or ``await``) each coroutine in the order they were added to the awaitable. For example, if ``foo`` was added, then ``bar``, then ``baz``, first ``foo`` would be awaited (with its respective callbacks), then ``bar``, and finally ``baz``.

An example of ``awaitable_await`` (without callbacks) is as follows:

::

    static PyObject *
    spam(PyObject *self, PyObject *args)
    {
        PyObject *foo;
        PyObject *bar;
        // In this example, these are both coroutines, not asynchronous functions
        
        if (!PyArg_ParseTuple(args, "OOO", &foo, &bar))
            return NULL;

        PyObject *awaitable = awaitable_new();

        if (awaitable == NULL)
            return NULL;

        if (awaitable_await(awaitable, foo, NULL, NULL) < 0)
        {
            Py_DECREF(awaitable);
            return NULL;
        }
        
        if (awaitable_await(awaitable, bar, NULL, NULL) < 0)
        {
            Py_DECREF(awaitable);
            return NULL;
        }
        
        return awaitable;
    }

::
    
    import asyncio

    async def foo():
        print("foo!")

    async def bar():
        print("bar!")

    asyncio.run(spam(foo(), bar()))
    # foo! is printed, then bar!


## Callbacks

The first argument in an ``awaitcallback`` is the ``AwaitableObject*`` (casted to a ``PyObject*``, once again), and the second argument is the result of the coroutine. Both of these are borrowed references, and should not be ``Py_DECREF``'d by the user. The return value of this function must be an integer. Any value below ``0`` denotes an error occurred, but there are two different ways to handle it:

- If the function returned ``-1``, it expects the error to be deferred to the error callback if it exists.
- If the function returned anything less than ``-1``, the error callback is ignored, and the error is deferred to the event loop (*i.e.*, ``__next__`` on the object's coroutine returns ``NULL``).

In an ``awaitcallback_err``, there are once again two arguments, both of which are again, borrowed references. The first argument is a ``AwaitableObject*``casted to a ``PyObject*``, and the second argument is the current exception (via ``PyErr_GetRaisedException``). Likewise, this function can also return an error, which is once again denoted by a value less than ``0``. This function also has two ways to handle exceptions:

- ``-1`` denotes that the original error should be restored via ``PyErr_SetRaisedException``.
- ``-2`` or lower says to not restore the error, and instead use the current error set by the callback. If no error is set, a ``SystemError`` is raised.

If either of these callbacks return an error value without an exception set, a ``SystemError`` is raised.

An example of using callbacks is shown below:

::
    static int
    spam_callback(PyObject *awaitable, PyObject *result)
    {
        printf("coro returned result: ");
        PyObject_Print(result, stdout, Py_PRINT_RAW);
        putc('\n');

        return 0;
    }


    static PyObject *
    spam(PyObject *self, PyObject *args)
    {
        PyObject *coro;
        if (!PyArg_ParseTuple(args, "O", &coro))
            return NULL;

        PyObject *awaitable = awaitable_new();

        if (awaitable_await(awaitable, coro, spam_callback, NULL) < 0)
        {
            Py_DECREF(awaitable);
            return NULL;
        }

        return awaitable;
    }

## Setting Results

``awaitable_res_result`` is the API function for setting the return value of an ``AwaitableObject*``. If ``awaitable_set_result`` is never called, the default return value is ``None``. This function may be called multiple times, in which case the previous return value is replaced. The ``AwaitableObject*`` will store a strong reference to the result, and is only decremented upon deallocation (or upon setting a new result).

## Cancelling

The function for cancelling an ``AwaitableObject*`` is ``awaitable_cancel``. This function will decrement any references to coroutines added. This function should only be used in callbacks and will raise a ``SystemError`` if called without any coroutines added. Note that coroutines may be added after this function is called, but is only possible to do in the same callback (as execution will stop when no coroutines are left). An example of usage is below:

::

    static int
    spam_callback(PyObject *awaitable, PyObject *result)
    {
        if (awaitable_cancel(awaitable) < 0)
            return -1;

        // Assume result is a coroutine
        if (awaitable_await(awaitable, result, NULL, NULL) < 0)
            return -1;

        return 0;
    }

## Storing and Fetching Values

Every ``AwaitableObject*`` will contain an array of strong references to ``PyObject*``'s, as well as an array of ``void*`` (referred to as arbitrary values here). Both of these arrays are separate, and deallocated at the end of the object's lifetime. ``awaitable_save*`` functions are the public functions for saving values to a ``AwaitableObject*``. ``awaitable_save*`` functions append to the existing array if called multiple times. These functions are varadic, and are supplied a ``nargs`` parameter specifying the number of values. 


An example of saving and unpacking values is shown below:

::

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

::

    # Assuming top-level await
    async def foo():
        await ...  # Pretend to do some blocking I/O
        return 39

    await spam(3, foo())  # 42

## Copyright

`pyawaitable` is distributed under the terms of the [MIT](https://spdx.org/licenses/MIT.html>) license.
