#ifndef PYAWAITABLE_CORO_H
#define PYAWAITABLE_CORO_H

#include <Python.h>
#include <pyawaitable/dist.h>

#ifndef MS_WIN32
_PyAwaitable_INTERNAL_DATA(PyMethodDef) pyawaitable_methods[];
#endif
_PyAwaitable_INTERNAL_DATA(PyAsyncMethods) pyawaitable_async_methods;

#endif
