Reference
=========

General
-------

.. function:: pyawaitable.include()

   Return the include path of ``pyawaitable.h``.


.. envvar:: PYAWAITABLE_INCLUDE

   Result of :func:`pyawaitable.include`, stored as an environment variable
   through a ``.pth`` file (only available when Python isn't run through
   :py:option:`-S`).


.. c:function:: int PyAwaitable_Init(void)

   Initialize PyAwaitable. This should typically be done in the :c:data:`Py_mod_exec`
   slot of a module.

   This can safely be called multiple times.

   Return ``0`` on success, and ``-1`` with an exception set on failure.


.. c:function:: PyObject *PyAwaitable_New(void)

   Create a new empty PyAwaitable object.

   This returns a new :term:`strong reference` to a PyAwaitable object on
   success, and returns ``NULL`` with an exception set on failure.


.. c:function:: int PyAwaitable_SetResult(PyObject *awaitable, PyObject *result)

   Set *result* to the :ref:`result <return-values>` of the PyAwaitable object.
   The result will be returned to the :keyword:`await` expression on this
   PyAwaitable object.

   *result* will not be stolen; this function will create its own reference to
   *result* internally.

   Return ``0`` with the return value set on success, and ``-1`` with an
   exception set on failure.


Coroutines
----------


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


Value Storage
-------------

.. c:function:: int PyAwaitable_SaveValues(PyObject *awaitable, Py_ssize_t nargs, ...)

   Store *nargs* amount of :ref:`object values <object-values>` in the
   PyAwaitable object.

   The number of arguments passed to ``...`` must match *nargs*. The objects
   passed will be stored in the PyAwaitable object internally to be unpacked
   by :c:func:`PyAwaitable_UnpackValues` later.

   Return ``0`` with the values stored on success, and ``-1`` with an
   exception set on failure.


.. c:function:: int PyAwaitable_UnpackValues(PyObject *awaitable, ...)

   Unpack :ref:`object values <object-values>` stored in the PyAwaitable
   object.

   This function expects ``PyObject **`` pointers passed to the ``...``.
   These will then be set to :term:`borrowed reference <borrowed reference>`.
   The number of arguments passed to the ``...`` must match the sum of all
   *nargs* to prior :c:func:`PyAwaitable_SaveValues` calls. For example, if
   one call stored two values, and then another call stored three values, this
   function would expect five pointers to be passed.

   Pointers passed to the ``...`` may be ``NULL``, in which case the object at
   that position is skipped.

   Return ``0`` will all references set on success, and ``-1`` with an
   exception set on failure.


.. c:function:: int PyAwaitable_SetValue(PyObject *awaitable, Py_ssize_t index, PyObject *value)

   Replace a single :ref:`object value <object-values>` at the position *index*
   with *value*. The old reference to the object stored at the position *index*
   is released, so *value* must not be ``NULL``.

   If *index* is below zero or out of bounds for the number of stored object
   values, this function will fail. As such, this function cannot be used to
   append new object values -- use :c:func:`PyAwaitable_SaveValues` for that.

   Return ``0`` with the object replaced on success, and ``-1`` with an exception
   set on failure.


.. c:function:: PyObject *PyAwaitable_GetValue(PyObject *awaitable, Py_ssize_t index)

   Unpack a single :ref:`object value <object-values>` at the position *index*.

   If *index* is below zero or out of bounds for the number of stored object
   values, this function will sanely fail.

   This is a low-level routine meant for complete-ness; always prefer using
   :c:func:`PyAwaitable_UnpackValues` over this function.

   Return a :term:`borrowed reference` to the value on success, and ``NULL``
   with an exception set on failure.


.. c:function:: int PyAwaitable_SaveArbValues(PyObject *awaitable, Py_ssize_t nargs, ...)

   Similar to :c:func:`PyAwaitable_SaveValues`, but saves
   :ref:`arbitrary values <arbitrary-values>` (``void *`` pointers) instead
   of :c:type:`PyObject * <PyObject>` references.

   Arbitrary values are separate from object values, so the number of Python
   objects stored through :c:func:`PyAwaitable_SaveValues` has no effect
   on this function.

   Return ``0`` with all pointers stored on success, and ``-1`` with an
   exception set on failure.


.. c:function:: int PyAwaitable_UnpackArbValues(PyObject *awaitable, ...)

   Similar to :c:func:`PyAwaitable_UnpackValues`, but unpacks
   :ref:`arbitrary values <arbitrary-values>` (``void *`` pointers) instead
   of :c:type:`PyObject * <PyObject>` references.

   Arbitrary values are separate from object values, so the number of Python
   objects stored through :c:func:`PyAwaitable_SaveValues` has no effect
   on this function.
   
   This function expects ``void **`` pointers passed to the ``...``.
   The number of arguments passed to the ``...`` must match the sum of
   all *nargs* to prior :c:func:`PyAwaitable_SaveArbValues` calls. For
   example, if one call stored two values, and then another call stored
   three values, this function would expect five pointers to be passed.

   Return ``0`` with all pointers set on success, and ``-1`` with an
   exception set on failure.


.. c:function:: int PyAwaitable_SetArbValue(PyObject *awaitable, Py_ssize_t index, void *value)

   Similar to :c:func:`PyAwaitable_SetValue`, but replaces a single
   :ref:`arbitrary value <arbitrary-values>` instead.

   If *index* is below zero or out of bounds for the number of stored object
   values, this function will fail. As such, this function cannot be used to
   append new object values -- use :c:func:`PyAwaitable_SaveArbValues` for that.

   Return ``0`` with the object replaced on success, and ``-1`` with an exception
   set on failure.


.. c:function:: void *PyAwaitable_GetArbValue(PyObject *awaitable, Py_ssize_t index)

   Similar to :c:func:`PyAwaitable_GetValue`, but unpacks a single
   :ref:`arbitrary value <arbitrary-values>` at the position *index*.

   If *index* is below zero or out of bounds for the number of stored object
   values, this function will sanely fail.

   This is a low-level routine meant for complete-ness; always prefer using
   :c:func:`PyAwaitable_UnpackArbValues` over this function.

   Return the ``void *`` pointer stored at *index* on success, and ``NULL``
   with an exception set on failure. If ``NULL`` is a valid value for the
   arbitrary value, use :c:func:`PyErr_Occurred` to differentiate.
