#ifndef PYAWAITABLE_GENWRAPPER_H
#define PYAWAITABLE_GENWRAPPER_H

#include <Python.h>
#include <pyawaitable/awaitableobject.h>

extern PyTypeObject _PyAwaitableGenWrapperType;

typedef struct _GenWrapperObject
{
    PyObject_HEAD
    PyAwaitableObject *gw_aw;
    PyObject *gw_current_await;
} GenWrapperObject;

PyObject *
genwrapper_next(PyObject *self);

int
genwrapper_fire_err_callback(
    PyObject *self,
    PyObject *await,
    pyawaitable_callback *cb
);

PyObject *
genwrapper_new(PyAwaitableObject *aw);

#endif
