#ifndef PYAWAITABLE_INIT_H
#define PYAWAITABLE_INIT_H

#include <Python.h>
#include <pyawaitable/dist.h>

_PyAwaitable_INTERNAL(PyObject *)
_PyAwaitable_GetState(void);

_PyAwaitable_API(PyTypeObject *)
PyAwaitable_GetType(void);

_PyAwaitable_INTERNAL(PyTypeObject *)
_PyAwaitable_GetGenWrapperType(void);

_PyAwaitable_API(int)
PyAwaitable_Init(void);

#endif
