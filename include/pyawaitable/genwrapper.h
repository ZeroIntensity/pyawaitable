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

PyObject *
genwrapper_next(PyObject *self);

void
awaitable_genwrapper_set_result(PyObject *gen, PyObject *result);

int
genwrapper_fire_err_callback(PyObject *self, PyObject *await, awaitable_callback *cb);

PyObject *
genwrapper_new(AwaitableObject *aw);

#endif
