Executing Asynchronous Calls from C
===================================

Let's say we wanted to replicate the following in C:

.. code-block:: py

    async def trampoline(coro: collections.abc.Coroutine) -> int:
        return await coro

This is simply a function that we pass a coroutine to, and it will await it for
us. It's not particularly useful, but it's just for learning purposes.

We already know that ``trampoline()`` will evaluate to a magic coroutine object
that supports :py:keyword:`await`, via the :py:attr:`~object.__await__` dunder,
and needs to :py:ref:`yield from <yieldexpr>` its coroutines. So, if we wanted
to break ``trampoline()`` down into a synchronous Python function, it would look
something like this:

.. code-block:: py

    class _trampoline_coroutine:
        def __init__(self, coro: collections.abc.Coroutine) -> None:
            self.coro = coro

        def __await__(self) -> collections.abc.Generator:
            yield
            yield from self.coro.__await__()

    def trampoline(coro: collections.abc.Coroutine) -> collections.abc.Coroutine:
        return _trampoline_coroutine(coro)

But, this is using ``yield from``; there's no ``yield from`` in C, so how do we
actually await things, or more importantly, use their return value? This is
where things get tricky.

Adding Awaits to a PyAwaitable Object
-------------------------------------

There's one big function for "adding" coroutines to a PyAwaitable object:
:c:func:`PyAwaitable_AddAwait`. By "add", we mean that the asynchronous
call won't happen right then and there. Instead, the PyAwaitable will
store it, and then when something comes to call the :py:attr:`~object.__await__`
on the PyAwaitable object, it will mimick a :py:ref:`yield from <yieldexpr>`
on that coroutine.

:c:func:`PyAwaitable_AddAwait` takes four arguments:

-   The PyAwaitable object.
-   The coroutine to store. (Not an `async def` function, but the result of
    calling one without :py:keyword:`await`.)
-   A callback.
-   An error callback.

Let's focus on the first two for now, and just pass ``NULL`` for the other
two in the meantime. We can implement ``trampoline()`` from our earlier
example pretty easily:

.. code-block:: c

    static PyObject *
    trampoline(PyObject *self, PyObject *coro) // METH_O
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

To your eyes, the ``yield from`` and all of that mess is completely
hidden; you give PyAwaitable your coroutine, and it handles the rest!
``trampoline()`` now acts like our pure-Python function from earlier:

.. code-block:: pycon

    >>> from _yourmod import trampoline
    >>> import asyncio
    >>> await trampoline(asyncio.sleep(2))  # Sleeps for 2 seconds

Yay! We called an asynchronous function from C!

Simpler ``PyAwaitable_AddAwait`` Calls
--------------------------------------

But, what if we wanted to call the ``async def`` function from the C API?
With our current knowledge, that would look like this:

.. code-block:: c

    static PyObject *
    trampoline(PyObject *self, PyObject *func) // METH_O
    {
        PyObject *awaitable = PyAwaitable_New();
        if (awaitable == NULL) {
            return NULL;
        }

        PyObject *coro = PyObject_CallNoArgs(func);
        if (coro == NULL) {
            Py_DECREF(awaitable);
            return NULL;
        }

        if (PyAwaitable_AddAwait(awaitable, coro, NULL, NULL) < 0) {
            Py_DECREF(awaitable);
            Py_DECREF(coro);
            return NULL;
        }

        Py_DECREF(coro);
        return awaitable;
    }

Ouch, that's a lot of boilerplate. Luckily, PyAwaitable provides a convenience
function for this case: :c:func:`PyAwaitable_AddExpr`. This function is very
similar to :c:func:`PyAwaitable_AddAwait`, but it has two additional semantics
for the passed coroutine:

-   If the coroutine is ``NULL``, it returns ``-1`` without setting an
    exception.
-   If the coroutine is non-``NULL``, it passes it to
    :c:func:`PyAwaitable_AddAwait` and then decrements its reference count
    ("stealing a reference").

These properties make it possible to directly use the result of a C API
function without extra boilerplate, because errors will be propagated when
it fails (when the coroutine is ``NULL``) and the reference count will be
decremented, preventing leaks.

So, with that in mind, we can rewrite our example as the following:

.. code-block:: c

    static PyObject *
    trampoline(PyObject *self, PyObject *func) // METH_O
    {
        PyObject *awaitable = PyAwaitable_New();
        if (awaitable == NULL) {
            return NULL;
        }

        if (PyAwaitable_AddExpr(awaitable, PyObject_CallNoArgs(func), NULL, NULL) < 0) {
            Py_DECREF(awaitable);
            return NULL;
        }

        return awaitable;
    }

.. _return-value-callbacks:

Using ``async with`` from C
---------------------------
Unlike the :c:func:`PyAwaitable_AddAwait` and the :c:func:`PyAwaitable_AddExpr`
functions there exists :c:func:`PyAwaitable_AsyncWith` that will call the
`__aenter__` and `__aexit__` members of a class if it implements the Async Context
Manager.

Using this on a type that is designed with `async with` in mind that automatically
cleans up resources when the scope leaves the context manager can be much easier
than manually making those calls to clean them up.

:c:func:`PyAwaitable_AsyncWith` takes four arguments:

-   The PyAwaitable object.
-   An instance of the class that implements the async context manager.
-   A callback.
-   An error callback.

.. note::

    This example uses the `asqlite` library created by Rapptz to allow using the
    `sqlite3` module from within Asynchronous code safely. It also uses a helper header
    file called `awaitfunc.h` from the https://github.com/AraHaan/awaitfunc github
    repository.

.. code-block:: c

    static int
    add_or_delete_items_cursor_cb(PyObject *awaitable, PyObject *cursor) {
        if (cursor != NULL && !Py_IsNone(cursor)) {
            PyObject *args;
            PyObject *connection;
            if (PyAwaitable_UnpackValues(awaitable, &args, &connection) < 0) {
                return -1;
            }

            if (PyAwaitable_AwaitFunction(awaitable, PyObject_GetCallableMethodString(cursor, "executemany"), "OO", NULL, NULL, PySequence_GetItem(args, 0), PySequence_GetItem(args, 1)) < 0) {
                return -1;
            }

            if (PyAwaitable_AwaitFunctionNoArgs(awaitable, PyObject_GetCallableMethodString(connection, "commit"), NULL, NULL) < 0) {
                return -1;
            }

            // No need to call "close" here for both cursor and connection because "PyAwaitable_AsyncWith" did it for us.

            return 0;
        }

        return -1;
    }

    static int
    add_or_delete_items_connect_cb(PyObject *awaitable, PyObject *connection) {
        if (connection != NULL && !Py_IsNone(connection)) {
            PyObject *args;
            if (PyAwaitable_UnpackValues(awaitable, &args) < 0) {
                return -1;
            }

            if (PyAwaitable_SaveValues(awaitable, 1, connection) < 0) {
                return -1;
            }

            if (PyAwaitable_AsyncWithFunctionNoArgs(awaitable, PyObject_GetCallableMethodString(connection, "cursor"), add_or_delete_items_cursor_cb, NULL) < 0) {
                return -1;
            }

            return 0;
        }

        return -1;
    }

    static PyObject *
    _DiscordBot___add_or_delete_items(PyObject *mod, PyObject *args) {
        PyObject *awaitable = PyAwaitable_New();
        if (!awaitable) {
            return NULL;
        }

        if (PyAwaitable_SaveValues(awaitable, 1, args) < 0) {
            Py_XDECREF(awaitable);
            return NULL;
        }

        PyObject *dbString = PyUnicode_FromString("Bot.db");
        if (!dbString) {
            Py_XDECREF(awaitable);
            return NULL;
        }

        DiscordBot_State *state = get_DiscordBot_state(mod);
        if (PyAwaitable_AsyncWithFunction(awaitable, PyObject_GetCallableMethodString(state->asqliteModule, "connect"), "N", add_or_delete_items_connect_cb, NULL, dbString) < 0) {
            Py_XDECREF(awaitable);
            return NULL;
        }

        return awaitable;
    }

The above is functionally equivalent to the following in Pure Python Code:

.. code-block:: py

    async def __add_or_delete_items(query: str, values: list):
        """Internal API. DO NOT USE."""
        async with asqlite.connect('Bot.db') as connection:
            async with connection.cursor() as cursor:
                await cursor.executemany(query, values)
                await connection.commit()

Getting the Return Value in a Callback
--------------------------------------

In many cases, it's desirable to use the return value of a coroutine.
For example, let's say we wanted to get the result of the following
asynchronous function:

.. code-block:: py

    async def silly() -> int:
        await asyncio.sleep(2)  # Simulate doing some I/O work
        return 42

The details of how coroutines return values aren't relevant, but we do know
that a coroutine isn't actually "awaited" until after we've already returned
our PyAwaitable object from C. That means we have to use a callback to get the
return value of the coroutine.

Specifically, we can pass a function pointer to the third parameter of
:c:func:`PyAwaitable_AddAwait`. A callback function takes two
:c:type:`PyObject * <PyObject>` parameters:

-   A reference to the PyAwaitable object that called it.
-   A reference to the return value of the coroutine.

A callback must return ``0`` to indicate success, or ``-1`` with an exception set to indicate failure.

Now, we can use the result of ``silly()`` in C:

.. code-block:: c

    static int
    callback(PyObject *awaitable, PyObject *value)
    {
        if (PyAwaitable_SetResult(awaitable, value) < 0) {
            return -1;
        }

        return 0;
    }

    static PyObject *
    call_silly(PyObject *self, PyObject *silly)
    {
        PyObject *awaitable = PyAwaitable_New();
        if (awaitable == NULL) {
            return NULL;
        }

        if (PyAwaitable_AddExpr(awaitable, PyObject_CallNoArgs(silly), callback, NULL) < 0) {
            Py_DECREF(awaitable);
            Py_DECREF(coro);
            return NULL;
        }

        Py_DECREF(coro);
        return awaitable;
    }

This can be used from Python as such:

.. code-block:: pycon

    >>> from _yourmod import call_silly
    >>> await call_silly(silly)  # Sleeps for 2 seconds
    silly() returned: 42


.. _error-callbacks:

Handling Errors with Callbacks
------------------------------

Coroutines can raise exceptions, during execution. For example, imagine we
wanted to use a function that makes a network request:

.. code-block:: py

    import asyncio


    async def make_request() -> str:
        async with asyncio.timeout(5):
            await asyncio.sleep(10)  # Simulate some I/O
            return "..."

The above will raise :py:exc:`TimeoutError`, but not on simply calling
``make_request()``; it will only raise once it's actually started executing
in an :py:keyword:`await`, and as we already know that coroutines don't execute
at the :c:func:`PyAwaitable_AddAwait` callsite, we can't simply check for
errors there. So, similar to return value callbacks, PyAwaitable provides error
callbacks, which--you guessed it--is the fourth argument to ``PyAwaitable_AddAwait``.

An error callback has the same signature as a return value callback, but instead
of taking a reference to a return value, it takes a reference to an exception
object that was caught and raised by either the coroutine or the coroutine's
callback.

.. note::

    Error callbacks are *not* called with an exception "set"
    (so :c:func:`PyErr_Occurred()` returns ``NULL``), so it's safe to call
    most of Python's C API without worrying about those kinds of failures.

An error callback's return value can do a number of different things to the
state of the PyAwaitable object's exception. Namely:

-   Returning ``0`` will consider the error successfully caught, so
    the PyAwaitable object will clear the exception and continue
    executing the rest of its coroutine.
-   Returning ``-1`` indicates that the error should be repropagated.
    The PyAwaitable object will officially "set" the Python exception
    (via :c:func:`PyErr_SetRaisedException`), raise the error to the
    event loop and stop itself from executing any future coroutines.
-   Returning ``-2`` indicates that a new error occurred while handling
    the other one; the original exception is _not_ restored, and an exception
    set by the error callback is used instead and propagated to the event loop.

.. note::

    Return value callbacks are not called if an exception occurred while executing the coroutine.

To try and give a real-world example of all three of these, let's implement
the following function in C:

.. code-block:: py

    async def is_api_reachable(make_request: Callable[[], collections.abc.Coroutine]) -> bool:
        try:
            await make_request()
            return True
        except TimeoutError:
            return False

.. note::

    :py:exc:`asyncio.TimeoutError` is an alias of the built-in
    :py:exc:`TimeoutError` exception.

We have to do several things here:

-   Call ``make_request()`` to get the coroutine object to :py:keyword:`await`.
-   Add an error callback for that coroutine.
-   In a return value callback, set the return value to ``True`` (or really,
    :c:data:`Py_True`), because that means the operation didn't time out.
-   In an error callback, check if the exception is an instance of
    :py:exc:`TimeoutError`, and set the return value to ``False`` if it is.
-   If it's something other than ``TimeoutError``, let it propagate.

In C, all that would be implemented like this:

.. code-block:: c

    static int
    return_true(PyObject *awaitable, PyObject *unused)
    {
        return PyAwaitable_SetResult(awaitable, Py_True);
    }

    static int
    return_false(PyObject *awaitable, PyObject *exc)
    {
        if (PyErr_GivenExceptionMatches(exc, PyExc_TimeoutError)) {
            if (PyAwaitable_SetResult(exc, Py_False) < 0) {
                // New exception occurred; give it to the event loop.
                return -2;
            }

            return 0;
        } else {
            // This isn't a TimeoutError!
            return -1;
        }
    }

    static PyObject *
    is_api_reachable(PyObject *self, PyObject *make_request)
    {
        PyObject *awaitable = PyAwaitable_New();
        if (awaitable == NULL) {
            return NULL;
        }

        if (PyAwaitable_AddExpr(awaitable, PyObject_CallNoArgs(coro), return_true, return_false)) {
            Py_DECREF(awaitable);
            return NULL;
        }

        return awaitable;
    }

Propagation of Errors in Return Value Callbacks
-----------------------------------------------

By default, returning ``-1`` from a return value callback will implicitly
call the error callback if one is set. But, this isn't always desirable;
sometimes, you want to let errors in callbacks bubble up instead of
getting handled by some default error handling mechanism you've installed.

You can force the PyAwaitable object to propagate the exception by returning
``-2`` from a return value callback. If ``-2`` is returned, the exception set
by the callback will always be raised back to whoever awaited the PyAwaitable
object.

For example, if we installed some global exception logger inside of the error
callback, but don't want that to grab things like a :py:exc:`MemoryError` inside
of the return callback, we would return ``-2``:

.. code-block:: c

    static int
    error_handler(PyObject *awaitable, PyObject *error)
    {
        // Simply print the error and continue execution
        PyErr_SetRaisedException(Py_NewRef(error));
        PyErr_Print();
        return 0;
    }

    static int
    handle_value(PyObject *awaitable, PyObject *something)
    {
        PyObject *message = PyUnicode_FromString("LOG: Got value");
        if (message == NULL) {
            // Skip the error callback
            return -2;
        }

        if (magically_log_value(message, something) < 0) {
            // Skip the error callback
            return -2;
        }

        return 0;
    }
