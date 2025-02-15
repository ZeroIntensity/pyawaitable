#ifndef PYAWAITABLE_GENWRAPPER_H
#define PYAWAITABLE_GENWRAPPER_H

#include <Python.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/dist.h>

_PyAwaitable_INTERNAL_DATA(PyTypeObject) _PyAwaitableGenWrapperType;

typedef struct _GenWrapperObject {
    PyObject_HEAD
    PyAwaitableObject *gw_aw;
    PyObject *gw_current_await;
} _PyAwaitable_MANGLE(GenWrapperObject);

_PyAwaitable_INTERNAL(PyObject *)
_PyAwaitableGenWrapper_Next(PyObject * self);

_PyAwaitable_INTERNAL(int)
_PyAwaitableGenWrapper_FireErrCallback(
    PyObject * self,
    PyAwaitable_Error err_callback
);

_PyAwaitable_INTERNAL(PyObject *)
genwrapper_new(PyAwaitableObject * aw);

#endif
