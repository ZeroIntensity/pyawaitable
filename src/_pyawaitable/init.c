#include <pyawaitable/init.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>

static struct PyModuleDef _pyawaitable =
{
    PyModuleDef_HEAD_INIT,
    "_pyawaitable",
    NULL,
    -1
};

_PyAwaitable_API(int)
PyAwaitable_Init(void)
{
    pyawaitableModule = PyModule_Create(&_pyawaitable);
    if (pyawaitableModule == NULL)
    {
        return -1;
    }

    if (PyModule_AddType(pyawaitableModule, &PyAwaitable_Type) < 0)
    {
        Py_DECREF(pyawaitableModule);
        return -1;
    }

    if (PyModule_AddType(pyawaitableModule, &_PyAwaitableGenWrapperType) < 0)
    {
        Py_DECREF(pyawaitableModule);
        return -1;
    }

    return 0;
}
