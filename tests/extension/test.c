#define PYAWAITABLE_THIS_FILE_INIT
#include <Python.h>
#include <pyawaitable.h>
#include <stdio.h>

#define ADD_ADDRESS(ptr)                                          \
        do { PyObject *py_int = PyLong_FromVoidPtr((void *) ptr); \
             if (!py_int) {                                       \
                 Py_DECREF(m);                                    \
                 return NULL;                                     \
             }                                                    \
             if (PyModule_AddObject(m, #ptr, py_int) < 0) {       \
                 Py_DECREF(m);                                    \
                 Py_DECREF(py_int);                               \
                 return NULL;                                     \
             }                                                    \
        } while (0);

PyObject *
test2(PyObject *self, PyObject *args);

static int
callback(PyObject *awaitable, PyObject *result)
{
    return pyawaitable_set_result(awaitable, result);
}

static PyObject *
test(PyObject *self, PyObject *coro)
{
    PyObject *awaitable = pyawaitable_new();
    if (pyawaitable_await(awaitable, coro, callback, NULL) < 0)
    {
        Py_DECREF(awaitable);
        return NULL;
    }

    return awaitable;
}

Py_EXPORTED_SYMBOL int
raising_callback(PyObject *awaitable, PyObject *result)
{
    PyErr_SetString(PyExc_RuntimeError, "test");
    return -1;
}

Py_EXPORTED_SYMBOL int
raising_err_callback(PyObject *awaitable, PyObject *result)
{
    PyErr_SetString(PyExc_ZeroDivisionError, "test");
    return -2;
}

static PyMethodDef methods[] =
{
    {"test", test, METH_O, NULL},
    {"test2", test2, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef module =
{
    PyModuleDef_HEAD_INIT,
    "_pyawaitable_test",
    NULL,
    -1,
    .m_methods = methods
};

PyMODINIT_FUNC
PyInit__pyawaitable_test()
{
    if (pyawaitable_init() < 0)
        return NULL;

    PyObject *m = PyModule_Create(&module);
    if (!m)
        return NULL;

    ADD_ADDRESS(raising_callback);
    ADD_ADDRESS(raising_err_callback);
    return m;
}
