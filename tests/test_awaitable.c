#include <Python.h>
#include <pyawaitable.h>
#include "pyawaitable_test.h"

static PyObject *
test_awaitable_new(PyObject *self, PyObject *nothing)
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    TEST_ASSERT(Py_IS_TYPE(awaitable, PyAwaitable_GetType()));
    Py_DECREF(awaitable);

    Test_SetNoMemory();
    PyObject *fail_alloc = PyAwaitable_New();
    Test_UnSetNoMemory();

    TEST_ASSERT(fail_alloc == NULL);
    Py_RETURN_NONE;
}

static PyObject *
test_awaitable_c_result_without_callback(PyObject *self, PyObject *nothing)
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    PyObject *value = PyLong_FromLong(42);
    if (value == NULL) {
        Py_DECREF(awaitable);
        return NULL;
    }

    if (PyAwaitable_SetResult(awaitable, value) < 0) {
        Py_DECREF(value);
        Py_DECREF(awaitable);
        return NULL;
    }

    TEST_ASSERT(Py_REFCNT(value) > 1);
    Py_DECREF(value);
    return Test_RunAndCheck(awaitable, value);
}

TESTS(awaitable) = {
    TEST(test_awaitable_new),
    TEST(test_awaitable_c_result_without_callback),
    {NULL}
};
