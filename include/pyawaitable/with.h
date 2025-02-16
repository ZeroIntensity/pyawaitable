#ifndef PYAWAITABLE_WITH_H
#define PYAWAITABLE_WITH_H

#include <Python.h> // PyObject
#include <pyawaitable/awaitableobject.h> // PyAwaitable_Callback, PyAwaitable_Error

_PyAwaitable_API(int)
PyAwaitable_AsyncWith(
    PyObject * aw,
    PyObject * ctx,
    PyAwaitable_Callback cb,
    PyAwaitable_Error err
);

#endif
