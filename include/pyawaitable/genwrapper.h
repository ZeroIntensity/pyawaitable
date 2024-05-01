#ifndef PYAWAITABLE_GENWRAPPER_H
#define PYAWAITABLE_GENWRAPPER_H

#include <Python.h>

extern PyTypeObject _AwaitableGenWrapperType;

PyObject *gen_next(PyObject *self);

#endif
