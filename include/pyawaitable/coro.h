#ifndef PYAWAITABLE_CORO_H
#define PYAWAITABLE_CORO_H

#include <Python.h>
#include <pyawaitable/dist.h>

extern PyMethodDef _PyAwaitable_MANGLE(pyawaitable_methods)[];
extern PyAsyncMethods _PyAwaitable_MANGLE(pyawaitable_async_methods);

#endif
