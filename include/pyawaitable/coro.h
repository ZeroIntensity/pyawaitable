#ifndef PYAWAITABLE_CORO_H
#define PYAWAITABLE_CORO_H

#include <Python.h>
#include <pyawaitable/dist.h>

_PyAwaitable_INTERNAL_DATA(PyMethodDef) pyawaitable_methods[];
_PyAwaitable_INTERNAL_DATA(PyAsyncMethods) pyawaitable_async_methods;

#endif
