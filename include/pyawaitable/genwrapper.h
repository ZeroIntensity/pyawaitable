#ifndef PYAWAITABLE_GENWRAPPER_H
#define PYAWAITABLE_GENWRAPPER_H

#include <Python.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/dist.h>

PyTypeObject _PyAwaitable_MANGLE(_PyAwaitableGenWrapperType);

typedef struct _GenWrapperObject
{
    PyObject_HEAD
    PyAwaitableObject *gw_aw;
    PyObject *gw_current_await;
} _PyAwaitable_MANGLE(GenWrapperObject);

_PyAwaitable_INTERNAL(PyObject *)
genwrapper_next(PyObject * self);

_PyAwaitable_INTERNAL(int)
genwrapper_fire_err_callback(
    PyObject * self,
    awaitcallback_err err_callback
);

_PyAwaitable_INTERNAL(PyObject *)
genwrapper_new(PyAwaitableObject * aw);

#endif
