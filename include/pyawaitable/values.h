#ifndef PYAWAITABLE_VALUES_H
#define PYAWAITABLE_VALUES_H

#include <Python.h> // PyObject, Py_ssize_t
#include <pyawaitable/dist.h>

/* Object values */

_PyAwaitable_API(int)
PyAwaitable_SaveValues(
    PyObject * awaitable,
    Py_ssize_t nargs,
    ...
);

_PyAwaitable_API(int)
PyAwaitable_UnpackValues(PyObject * awaitable, ...);

_PyAwaitable_API(int)
PyAwaitable_SetValue(
    PyObject * awaitable,
    Py_ssize_t index,
    PyObject * new_value
);
_PyAwaitable_API(PyObject *)
PyAwaitable_GetValue(
    PyObject * awaitable,
    Py_ssize_t index
);

/* Arbitrary values */

_PyAwaitable_API(int)
PyAwaitable_SaveArbValues(
    PyObject * awaitable,
    Py_ssize_t nargs,
    ...
);

_PyAwaitable_API(int)
PyAwaitable_UnpackArbValues(PyObject * awaitable, ...);

_PyAwaitable_API(int)
PyAwaitable_SetArbValue(
    PyObject * awaitable,
    Py_ssize_t index,
    void *new_value
);

_PyAwaitable_API(void *)
PyAwaitable_GetArbValue(
    PyObject * awaitable,
    Py_ssize_t index
);

#endif
