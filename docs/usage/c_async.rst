Making a C Function Asynchronous
--------------------------------

Let's make a C function that replicates the following Python code:

.. code-block::

    async def hello() -> None:
        print("Hello, PyAwaitable")

If you've tried to implement an asynchronous C function in the past, this is
likely where you got stuck. How do we make a C function ``async def``?

Breaking Down Awaitable Functions
---------------------------------

In Python, you have to call an ``async def`` function to use it with
:py:keyword:`await`. In our example above, the following would be invalid:

.. code-block:: pycon

    >>> await hello

Of course, you need to do ``await hello()`` instead. Why?

``hello()`` is returning a :term:`coroutine`, and coroutine objects are
usable with the :py:keyword:`await` keyword (but not ``async def`` functions
themselves). So, ``hello()`` as a synchronous function would really be like:

.. code-block:: python

    class _hello_coroutine:
        def __await__(self) -> collections.abc.Generator:
            print("Hello, PyAwaitable")
            yield

    def hello() -> collections.abc.Coroutine:
        return _hello_coroutine()

If there were to be :keyword:`await` expressions inside ``hello()``, the
returned coroutine object would handle those by yielding inside of
the :py:attr:`~object.__await__` dunder method. We can do the same kind of
thing in C.

Creating PyAwaitable Objects
----------------------------

You can create a new PyAwaitable object with :c:func:`PyAwaitable_New`.
This returns a :term:`strong reference` to a PyAwaitable object, and
``NULL`` with an exception set on failure.

Think of a PyAwaitable object sort of like the ``_hello_coroutine`` example
from above, but it's generic instead of being specially built for the
``hello()`` function. So, like our Python example, we need to return the
coroutine to allow it to be used in :keyword:`await` expressions:

.. code-block:: c

    static PyObject *
    hello(PyObject *self, PyObject *unused) // METH_NOARGS
    {
        PyObject *awaitable = PyAwaitable_New();
        if (awaitable == NULL) {
            return NULL;
        }

        puts("Hello, PyAwaitable");
        return awaitable;
    }

.. note::

    "Coroutine" is a bit of an ambigious term in Python. There are two types
    of coroutines: native ones (instances of :py:class:`types.CoroutineType`),
    and objects that implement the coroutine protocol
    (:py:class:`collections.abc.Coroutine`). Only the interpreter itself can
    create native coroutines, so a PyAwaitable object is an object that
    implements the coroutine protocol.

Yay! We can now use ``hello()`` in :keyword:`await` expressions:

.. code-block:: pycon

    >>> from _yourmod import hello
    >>> await hello()
    Hello, PyAwaitable

.. _return-values:

Changing the Return Value
-------------------------

Note that in all code-paths, we should return the PyAwaitable object, or
``NULL`` with an exception set to indicate a failure. But that means you can't
simply ``return`` your own :c:type:`PyObject * <PyObject>`; how can the
:keyword:`await` expression of our C coroutine evaluate to something useful?

By default, the "return value" (_i.e._, what :keyword:`await` will evaluate to)
is :const:`None` (or really, :c:data:`Py_None` in C). That can be changed with
:c:func:`PyAwaitable_SetResult`, which takes a reference to the object you
want to return. PyAwaitable will store a :term:`strong reference` to this
object internally.

For example, if you wanted to return the Python integer ``42`` from ``hello()``,
you would simply pass a :c:type:`PyObject * <PyObject>` for ``42`` to
:c:func:`PyAwaitable_SetResult`:

.. code-block:: c

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

Now, the :keyword:`await` expression evalutes to ``42``:

.. code-block:: pycon

    >>> from _yourmod import hello
    >>> await hello()
    Hello, PyAwaitable
    42
