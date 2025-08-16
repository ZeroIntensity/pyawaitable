Reference
=========

.. c:type:: int (*PyAwaitable_Callback)(PyObject *awaitable, PyObject *result)

   The type of a result callback, as submitted in :c:func:`PyAwaitable_AddAwait`.

   This takes two :c:type:`PyObject * <PyObject>` references: an instance of
   a PyAwaitable object, and the return value of the awaited coroutine.

   There are three possible return values:

   1. ``0``: OK.
   2. ``-1``: Error, fall to the error callback registered alongside this one,
      if it exists. There must be an exception set.
   3. ``-2``: Error, but skip the error callback if provided. There must be an
      exception set.


.. c:type:: int (*PyAwaitable_Error)(PyObject *awaitable, PyObject *result)

   The type of an error callback.


.. c:type:: int (*PyAwaitable_Defer)(PyObject *awaitable)

   The type of a "defer" callback.


.. c:function:: int PyAwaitable_AddAwait(PyObject *awaitable, PyObject *coroutine, PyAwaitable_Callback result_callback, PyAwaitable_Error error_callback)

    Mark an :term:`awaitable` object, *coroutine*, for execution by the event
    loop when the PyAwaitable object is awaited.

    *result_callback* is a :ref:`return value callback <return-value-callbacks>`.
    It will be called with the result of *coroutine* when it has finished
    execution. This may be ``NULL``, in which case the return value is simply
    discarded.

    *error_callback* is an :ref:`error callback <error-callbacks>`. It is
    called if *coroutine* raises an exception during execution, or when
    *result_callback* returns ``-1``. This may be ``NULL``, which will cause
    any exceptions to be propagated to the caller (the one who awaited the
    PyAwaitable object).

    This function will return ``0`` on success, and ``-1`` with an exception
    set on failure.
