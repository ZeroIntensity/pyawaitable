#ifndef PYAWAITABLE_WITH_H
#define PYAWAITABLE_WITH_H

#include <Python.h> // PyObject
#include <pyawaitable//awaitableobject.h> // awaitcallback, awaitcallback_err

_PyAwaitable_API(int)
pyawaitable_async_with(
    PyObject *aw,
    PyObject *ctx,
    awaitcallback cb,
    awaitcallback_err err
);

#endif
