#ifndef PYAWAITABLE_GENWRAPPER_H
#define PYAWAITABLE_GENWRAPPER_H

#include <Python.h>

extern PyTypeObject _AwaitableGenWrapperType;

typedef struct {
    PyObject_HEAD
    PyObject *gw_result;
    AwaitableObject *gw_aw;
    PyObject *gw_current_await;
} GenWrapperObject;

PyObject *gen_next(PyObject *self);

#endif
