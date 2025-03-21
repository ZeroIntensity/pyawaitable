#include <Python.h>
#include <pyawaitable.h>
#include "pyawaitable_test.h"

static int
_pyawaitable_test_exec(PyObject *mod)
{
#define ADD_TESTS(name)                                                       \
        do {                                                                  \
            if (PyModule_AddFunctions(mod, _pyawaitable_test_ ## name) < 0) { \
                return -1;                                                    \
            }                                                                 \
        } while (0)

    ADD_TESTS(awaitable);
    ADD_TESTS(callbacks);
#undef ADD_TESTS
    return PyAwaitable_Init();
}

static PyModuleDef_Slot _pyawaitable_test_slots[] = {
    {Py_mod_exec, _pyawaitable_test_exec},
    {0, NULL}
};

static PyModuleDef _pyawaitable_test_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_size = 0,
    .m_slots = _pyawaitable_test_slots,
};

PyMODINIT_FUNC
PyInit__pyawaitable_test()
{
    return PyModuleDef_Init(&_pyawaitable_test_module);
}
