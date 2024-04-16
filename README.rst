PyAwaitable
===========

**NOTE:** This README is a scrapped PEP. For the original text, see `<https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926> here`

Motivation
==========

CPython currently has no existing C interface for writing asynchronous functions or doing any sort of ``await`` operations, other than defining extension types and manually implementing methods like ``__await__`` from scratch. This lack of an API can be seen in some Python-to-C transpilers (such as ``mypyc``) having limited support for asynchronous code.

In the current C API, developers are forced to do one of three things when it comes to asynchronous code:

- Manually implementing coroutines using extension types.
- Use an external tool to compile their asynchronous code to C.
- Defer their asynchronous logic to a synchronous Python function, and then call that natively.

Rationale
=========

This API aims to provide a *generic* interface for working with asynchronous primitives only, as there are other event loop implementations.

For this reason, ``PyAwaitable`` does not provide any interface for executing C blocking I/O, as that would likely require leveraging something in the event loop implementation (in ``asyncio``, it would be something like ``asyncio.to_thread``).


Specification
=============

Additions to the C API
----------------------

**NOTE:** For all functions returning ``int`` in this PEP, ``0`` is a successful result and ``-1`` is a failure.

This PEP will create a new suite of C API functions under the prefix of ``PyAwaitable_``, as well as a new ``PyAwaitableObject`` structure along with a ``PyAwaitable_Type`` (known in Python as ``awaitable``). This is an object that must implement ``collections.abc.Coroutine`` [3]_. Note that the standard generator ``send`` attribute should be implemented under a method *and* under ``tp_as_async.am_send``, due to ``PyIter_Send`` defaulting to ``tp_iternext``, which is likely already defined [6]_.

This PEP adds these new functions to the C API:

- ``PyObject *PyAwaitable_New()``
- ``void PyAwaitable_Cancel(PyObject *aw)``
- ``int PyAwaitable_AddAwait(PyObject *aw, PyObject *coro, awaitcallback cb, awaitcallback_err err)``
- ``int PyAwaitable_SetResult(PyObject *awaitable, PyObject *result)``
- ``int PyAwaitable_SaveValues(PyObject *awaitable, Py_ssize_t nitems, PyObject **values)``
- ``int PyAwaitable_SaveArbValues(PyObject *awaitable, Py_ssize_t nitems, void **values)``
- ``int PyAwaitable_UnpackValues(PyObject *awaitable, PyObject **values)``
- ``int PyAwaitable_UnpackArbValues(PyObject *awaitable, void **values)``
- ``int PyAwaitable_UnpackArbValuesVa(PyObject *awaitable, ...)``
- ``int PyAwaitable_UnpackValuesVa(PyObject *awaitable, ...)``

This PEP also adds these new typedefs:

::

    typedef int (*awaitcallback)(PyObject *, PyObject *);
    typedef int (*awaitcallback_err)(PyObject *, PyObject *);
    typedef struct _PyAwaitableObject PyAwaitableObject;

And one macro:

::

    #define PyAwaitable_AWAIT(aw, coro) PyAwaitable_AddAwait(aw, coro, NULL, NULL)

Changes to ``inspect`` documentation
------------------------------------

A note should be added to ``inspect.iscoroutine`` mentioning that ``isinstance(obj, collections.abc.Coroutine)`` should be used instead for most cases to support detecting ``PyAwaitableObject*``, *except* when you are truly trying to detect a native coroutine (such as inside of a debugger). 

Overview
--------

A ``PyAwaitableObject*`` will store an array of strong references to coroutines, which are then yielded to the event loop by an iterator returned by the ``PyAwaitableObject*``'s ``__await__``. In the reference implementation, this is done with an extra type, called ``_GenWrapper`` (in the API defined as ``_PyAwaitable_GenWrapper_Type``), which will defer the result to the ``__next__`` of the coroutine iterator currently being executed.

The lifecycle of this process is as follows:

- ``__await__`` on the ``PyAwaitableObject*`` is called, returns an iterator (``GenWrapperObject*`` in the reference implementation [4]_). Throughout this PEP, any iterator returned by ``__await__`` is referred to as a coroutine iterator.
- This iterator must somehow contain the current coroutine being executed (this is the state/index), as well as the array of coroutines added by the user. 
- Upon ``__next__`` being called on this iterator, if no coroutine is being executed, the ``__await__`` is called on the coroutine at the current index (the state), and the state is incremented. ``__next__`` does not return after this step.
- Inside of ``__next__``, the ``__next__`` is called on the current coroutine iterator (the result of ``coro.__await__()``, once again). If the object has no ``__await__`` attribute, a ``TypeError`` is raised.
- When the coroutine iterator raises ``StopIteration``, the callback for the current coroutine (if it exists) is called with the value and sets the current iterator to a ``NULL``-like value (any value denoting "no coroutine iterator is being executed". This is ``NULL`` in the reference implementation).
- This process is repeated inside ``__next__`` calls until the end of the coroutine array is reached. Note that coroutine callbacks may add extra coroutines to be awaited during their execution.
- Finally, once the final coroutine is done, the iterator must raise ``StopIteration`` with the return value upon the next call to ``__next__``. After a ``StopIteration`` has been raised, then the awaitable object is marked as done. At this point, a ``RuntimeError`` should be raised upon trying to call ``__next__``, ``__await__``, or any ``PyAwaitable*`` functions.
- If at any point during this process, an exception is raised (including by coroutine result callbacks), the error callback of the coroutine being executed is called with the raised exception.

Semantics
---------

Returning a ``PyAwaitableObject*`` from a C function will mimic a Python function defined with ``async def``. This means it supports using ``await`` on the result of the function, as well as functions relating to coroutine operations, such as ``asyncio.run``, regardless of whether it has any coroutines stored. The public interface for creating a ``PyAwaitableObject*`` is ``PyAwaitable_New``, which like other constructor functions in the C API, may return a ``PyObject*`` or ``NULL``.

An example of basic usage would look like:

::

    static PyObject *
    spam(PyObject *self, PyObject *args)
    {
        PyObject *awaitable = PyAwaitable_New();
        return awaitable;
    }

::

    # Assuming top-level await for simplicity
    await spam()


Adding Coroutines
-----------------

The public interface for adding a coroutine to be executed by the event loop is ``PyAwaitable_AddAwait``, which takes four parameters:

- ``aw`` is the ``PyAwaitableObject*``.
- ``coro`` is the coroutine (or again, any object supporting ``__await__``). This is not checked by this function, but is checked later inside the ``__next__`` of the ``PyAwaitableObject*``'s coroutine iterator). This is not a function defined with ``async def``, but instead the return value of one (called without ``await``). This value is stored for the lifetime of the object, or until ``PyAwaitable_Cancel`` is called.
- ``cb`` is the callback that will be run with the result of ``coro``. This may be ``NULL``, in which case the result will be discarded.
- ``err`` is a callback in the event that an exception occurs during the execution of ``coro``. This may be ``NULL``, in which case the error is simply raised.

The ``awaitable`` is guaranteed to yield (or ``await``) each coroutine in the order they were added to the awaitable. For example, if ``foo`` was added, then ``bar``, then ``baz``, first ``foo`` would be awaited (with its respective callbacks), then ``bar``, and finally ``baz``.

This PEP also introduces a new macro, which is a shortcut for adding a coroutine with no callbacks. It is simply defined as:

::

    #define PyAwaitable_AWAIT(obj, coro) PyAwaitable_AddAwait(obj, coro, NULL, NULL)


An example of ``PyAwaitable_AddAwait`` without callbacks is as follows:

::

    static PyObject *
    spam(PyObject *self, PyObject *args)
    {
        PyObject *foo;
        PyObject *bar;
        // In this example, these are both coroutines, not asynchronous functions
        
        if (!PyArg_ParseTuple(args, "OOO", &foo, &bar))
            return NULL;

        PyObject *awaitable = PyAwaitable_New();

        if (awaitable == NULL)
            return NULL;

        if (PyAwaitable_AWAIT(awaitable, foo, NULL, NULL) < 0)
        {
            Py_DECREF(awaitable);
            return NULL;
        }
        
        if (PyAwaitable_AWAIT(awaitable, bar, NULL, NULL) < 0)
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


Callbacks
---------

The first argument in an ``awaitcallback`` is the ``PyAwaitableObject*`` (casted to a ``PyObject*``), and the second argument is the result of the coroutine. Both of these are borrowed references, and should not be ``Py_DECREF``'d by the user. The return value of this function must be an integer. Any value below ``0`` denotes an error occurred, but there are two different ways to handle it:

- If the function returned ``-1``, it expects the error to be deferred to the error callback if it exists.
- If the function returned anything less than ``-1``, the error callback is ignored, and the error is deferred to the event loop (*i.e.*, ``__next__`` on the object's coroutine returns ``NULL``).

In an ``awaitcallback_err``, there are once again two arguments, both of which are again, borrowed references. The first argument is a ``PyAwaitableObject*``casted to a ``PyObject*``, and the second argument is the current exception (via ``PyErr_GetRaisedException``). Likewise, this function can also return an error, which is once again denoted by a value less than ``0``. This function also has two ways to handle exceptions:

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

        PyObject *awaitable = PyAwaitable_New();

        if (PyAwaitable_AddAwait(awaitable, coro, spam_callback, NULL) < 0)
        {
            Py_DECREF(awaitable);
            return NULL;
        }

        return awaitable;
    }

Setting Results
---------------

``PyAwaitable_SetResult`` is the API function for setting the return value of a ``PyAwaitableObject*``. If ``PyAwaitable_SetResult`` is never called, the default return value is ``None``. This function may be called multiple times, in which case the previous return value is replaced. The ``PyAwaitableObject*`` must store a strong reference to the result, and is only decremented upon deallocation (or upon setting a new result).

Cancelling
----------

The function for cancelling a ``PyAwaitableObject*`` is ``PyAwaitable_Cancel``. This function must decrement any references to coroutines added. This function should only be used in callbacks and should raise a ``SystemError`` if called without any coroutines added. Note that coroutines may be added after this function is called, but is only possible to do in the same callback (as execution will stop when no coroutines are left). An example of usage is below:

::

    static int
    spam_callback(PyObject *awaitable, PyObject *result)
    {
        if (PyAwaitable_Cancel(awaitable) < 0)
            return -1;

        // Assume result is a coroutine
        if (PyAwaitable_AWAIT(awaitable, result) < 0)
            return -1;

        return 0;
    }

Storing and Fetching Values
---------------------------

Every ``PyAwaitableObject*`` must contain an array of strong references to ``PyObject*``, as well as an array of ``void*`` (referred to as arbitrary values in this PEP). Both of these may be stored in any way (such as in a ``list``), but in the reference implementation they are simply allocated via ``PyMem_Calloc`` [4]_. Both of these arrays must be separate, and deallocated at the end of the object's lifetime. ``PyAwaitable_Save*`` functions are the public functions for saving values to a ``PyAwaitableObject*``. ``PyAwaitable_Save*`` functions append to the existing array if called multiple times.

``PyAwaitable_Save*Va`` functions take a variadic number of arguments via an ellipsis, while the other functions take an array. In the case of a varadic function, the ``nargs`` parameter should match the number of passed arguments.

Saved values must be unpacked to a callback via passing pointers to local variables (*i.e.*, ``void**`` or ``PyObject**``) to ``PyAwaitable_UnpackValues`` or ``PyAwaitable_UnpackArbValues``. These functions do not take a ``nargs`` parameter, and instead, expect a number of arguments equal to that passed in the ``PyAwaitable_Save*`` function. Parameters that are not needed should be passed as ``NULL`` instead of a pointer.

An example of saving and unpacking values is shown below:

::

    static int
    spam_callback(PyObject *awaitable, PyObject *result)
    {
        PyObject *value;
        if (PyAwaitable_UnpackValuesVa(awaitable, &value) < 0)
            return -1;

        long a = PyLong_AsLong(result);
        long b = PyLong_AsLong(value);
        if (PyErr_Occurred())
            return -1;

        PyObject *ret = PyLong_FromLong(a + b);
        if (ret == NULL)
            return -1;

        if (PyAwaitable_SetResult(awaitable, ret) < 0)
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

        PyObject *awaitable = PyAwaitable_New();
        if (awaitable == NULL)
            return NULL;

        if (PyAwaitable_SaveValuesVa(awaitable, 1, value) < 0)
        {
            Py_DECREF(awaitable);
            return NULL;
        }

        if (PyAwaitable_AddAwait(awaitable, coro, spam_callback, NULL) < 0)
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

Copyright
=========

This document is placed in the public domain or under the
CC0-1.0-Universal license, whichever is more permissive.
