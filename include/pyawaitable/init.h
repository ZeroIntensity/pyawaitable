#ifndef PYAWAITABLE_INIT_H
#define PYAWAITABLE_INIT_H

#include <Python.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/dist.h>
#include <pyawaitable/genwrapper.h>

_PyAwaitable_API(int)
PyAwaitable_Init(PyObject * mod)
{
    if (PyModule_AddType(mod, &PyAwaitable_Type) < 0)
    {
        return -1;
    }

    if (PyModule_AddType(mod, &_PyAwaitableGenWrapperType) < 0)
    {
        return -1;
    }

    return 0;
}

#endif
