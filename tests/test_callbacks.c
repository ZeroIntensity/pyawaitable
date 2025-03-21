#include <Python.h>
#include <pyawaitable.h>
#include "pyawaitable_test.h"

static int
simple_callback(PyObject *awaitable, PyObject *value)
{
    TEST_ASSERT_INT(value == Py_None);
    return 0;
}

static PyObject *
test_simple_callback(PyObject *self, PyObject *coro)
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    if (PyAwaitable_AddAwait(awaitable, coro, simple_callback, NULL) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    return Test_RunAwaitable(awaitable);
}

TESTS(callbacks) = {
    TEST_CORO(test_simple_callback),
    {NULL}
};
