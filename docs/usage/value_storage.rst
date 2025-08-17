Managing State Between Callbacks
================================

So far, all of our examples haven't needed any transfer of state between the
:c:func:`PyAwaitable_AddAwait` callsite and the time the coroutine is executed
(in a callback). You might have noticed that the callbacks don't take a
``void *arg`` parameter to make up for C's lack of closures, so how do we
manage state?

First, let's get an example of what we might want state for. Our goal is to
implement a function that takes a paremeter and does something with it against
the result of a coroutine. For example:

.. code-block:: python

    async def multiply_async(
        number: int,
        get_number_io: Callable[[], collections.abc.Awaitable[int]]
    ) -> str:
        value = await get_number_io()
        return value * number

Introducing Value Storage
-------------------------

Instead of ``void *arg`` parameters, PyAwaitable provides APIs for storing
state directly on the PyAwaitable object. There are two types of value
storage:

-   Object value storage; :c:type:`PyObject * <PyObject>` pointers that
    PyAwaitable keeps references to.
-   Arbitrary value storage; ``void *`` pointers that PyAwaitable never
    dereferences -- it's your job to manage it.

Value storage is generally a lot more convenient than something like a
``void *arg``, because you don't have to define any ``struct`` or make
extra allocations. It's especially convenient in the ``PyObject *`` case,
because you don't have to worry about dealing with their reference counts
or traversing reference cycles. And, even if a single state ``struct`` is
more convenient for your case, it's easy to implement it through the arbitrary
values API.

There are four parts to the value APIs, each with a variant for object and
arbitrary values:

-   Saving values: :c:func:`PyAwaitable_SaveValues` and :c:func:`PyAwaitable_SaveArbValues`.
-   Unpacking values: :c:func:`PyAwaitable_UnpackValues` and :c:func:`PyAwaitable_UnpackArbValues`.
-   Getting values: :c:func:`PyAwaitable_GetValue` and :c:func:`PyAwaitable_GetArbValue`.
-   Setting values: :c:func:`PyAwaitable_SetValue` and :c:func:`PyAwaitable_SetArbValue`.

.. _object-values:

Object Value Storage
--------------------

In most cases, you want to store Python objects on your PyAwaitable object.
This can be anything you want, such as arguments passed into your function.
The two main APIs you want when using value storage are
:c:func:`PyAwaitable_SaveValues` and :c:func:`PyAwaitable_UnpackValues`.

These are variadic C functions; for ``Save``, pass the PyAwaitable object
and the number of objects you want to store, and then pass ``PyObject *``
pointers matching that number. These references will not be stolen by
PyAwaitable.

``Unpack``, on the other hand, does not require you to pass the number of
objects that you want -- it remembers how many you stored in ``Save``.
In ``Unpack``, you just pass the PyAwaitable object and pointers to local
``PyObject *`` variables, which will then be unpacked by the PyAwaitable
object (these may be ``NULL``, in which case the value is skipped).

.. note::

    Both :c:func:`PyAwaitable_SaveValues` and
    :c:func:`PyAwaitable_UnpackValues` can fail. They return ``-1`` with an
    exception set on failure, and ``0`` on success.

For example, if you called ``PyAwaitable_SaveValues(awaitable, 3, /* ... */)``,
you must pass three non-``NULL`` ``PyObject *`` references, and then pass
three pointers-to-pointers to ``PyAwaitable_UnpackValues`` (but these may be
``NULL``).

So, with all that in mind, we can implement ``multiply_async()`` from above
as such:

.. code-block:: c

    static int
    multiply_callback(PyObject *awaitable, PyObject *value)
    {
        PyObject *number;
        if (PyAwaitable_UnpackValues(awaitable, &number) < 0) {
            return -1;
        }

        PyObject *result = PyNumber_Multiply(number, value);
        if (result == NULL) {
            return -1;
        }

        if (PyAwaitable_SetResult(awaitable, result) < 0) {
            Py_DECREF(result);
            return -1;
        }

        Py_DECREF(result);
        return 0;
    }

    static PyObject *
    multiply_async(PyObject *self, PyObject *args) // METH_VARARGS
    {
        PyObject *number;
        PyObject *get_number_io;

        if (!PyArg_ParseTuple(args, "OO", &number, &get_number_io)) {
            return NULL;
        }

        PyObject *awaitable = PyAwaitable_New();
        if (awaitable == NULL) {
            return NULL;
        }

        if (PyAwaitable_SaveValues(awaitable, 1, number) < 0) {
            Py_DECREF(awaitable);
            return NULL;
        }

        if (PyAwaitable_AddExpr(awaitable, PyObject_CallNoArgs(get_number_io),
                                multiply_callback, NULL) < 0) {
            Py_DECREF(awaitable);
            return NULL;
        }

        return awaitable;
    }

Getting and Setting Values
--------------------------

In rare cases, it might be desirable to get or set a specific value at an
index. :c:func:`PyAwaitable_SetValue` (or :c:func:`PyAwaitable_SetArbValue`)
is useful if you intend to completely overwrite an object at a value index,
but :c:func:`PyAwaitable_GetValue` should basically never be preferred over
:c:func:`PyAwaitable_UnpackValues`; it's only there for complete-ness.

.. _arbitrary-values:

Arbitrary Value Storage
-----------------------

Arbitrary value storage works exactly the same as
:ref:`object value storage <object-values>`, with the exception of taking
``void *`` pointers instead of ``PyObject *`` pointers. PyAwaitable will
never attempt to read or write the pointers that you pass, so managing their
lifetime is up to you. In most cases, if your PyAwaitable object is supposed
to own the state of the arbitrary value, you deallocate it in the last
callback.
