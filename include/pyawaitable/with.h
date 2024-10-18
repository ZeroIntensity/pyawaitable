#ifndef PYAWAITABLE_WITH_H
#define PYAWAITABLE_WITH_H

#include <Python.h> // PyObject
#include <pyawaitable//awaitableobject.h> // awaitcallback, awaitcallback_err

int
pyawaitable_async_with_impl(
    PyObject *aw,
    PyObject *ctx,
    awaitcallback cb,
    awaitcallback_err err
);

#endif
