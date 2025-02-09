#include <pyawaitable.h>

static int
mod_exec(PyObject *mod)
{
    return PyAwaitable_Init();
}

PyModuleDef_Slot slots[] =
{
    {Py_mod_exec, mod_exec},
    {0, NULL}
};

static int
test_callback(PyObject *awaitable, PyObject *result)
{
    if (PyAwaitable_SetResult(awaitable, result) < 0)
    {
        return -1;
    }

    return 0;
}

static PyObject *
test(PyObject *self, PyObject *coro)
{
    if (!PyCoro_CheckExact(coro))
    {
        PyErr_SetString(PyExc_TypeError, "not a coroutine");
        return NULL;
    }

    /* Make sure that the structure is exposed correctly */
    PyAwaitableObject *awaitable = (PyAwaitableObject *)PyAwaitable_New();
    if (awaitable == NULL)
    {
        return NULL;
    }

    if (
        PyAwaitable_AddAwait(
            (PyObject *)awaitable,
            coro,
            test_callback,
            NULL
        ) < 0
    )
    {
        Py_DECREF(awaitable);
        return NULL;
    }

    return (PyObject *)awaitable;
}

PyMethodDef methods[] =
{
    {"test", test, METH_O, NULL},
    {NULL, NULL, 0, NULL}
};

PyModuleDef def =
{
    PyModuleDef_HEAD_INIT,
    .m_name = "_pyawaitable_test",
    .m_slots = slots,
    .m_methods = methods
};

PyMODINIT_FUNC
PyInit__pyawaitable_test()
{
    return PyModuleDef_Init(&def);
}
