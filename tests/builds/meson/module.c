#include <Python.h>
#include <pyawaitable.h>

static int
module_exec(PyObject *mod)
{
    return PyAwaitable_Init();
}

/*
 Equivalent to the following Python function:

 async def async_function(coro: collections.abc.Awaitable) -> None:
    await coro

 */
static PyObject *
async_function(PyObject *self, PyObject *coro)
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    if (PyAwaitable_AddAwait(awaitable, coro, NULL, NULL) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    return awaitable;
}

static PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, module_exec},
    {0, NULL}
};

static PyMethodDef module_methods[] = {
    {"async_function", async_function, METH_O, NULL},
    {NULL, NULL, 0, NULL},
};

static PyModuleDef module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_size = 0,
    .m_slots = module_slots,
    .m_methods = module_methods
};

PyMODINIT_FUNC
PyInit__meson_module()
{
    return PyModuleDef_Init(&module);
}