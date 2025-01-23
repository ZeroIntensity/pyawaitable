#ifndef PYAWAITABLE_INIT_H
#define PYAWAITABLE_INIT_H

#include <Python.h>
#include <pyawaitable/dist.h>

_PyAwaitable_INTERNAL_DATA(PyObject *) pyawaitableModule;

_PyAwaitable_API(int)
PyAwaitable_Init(void);

#endif
