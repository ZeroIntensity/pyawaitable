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
    PyAwaitable_Cancel(awaitable); // Prevent warning
    Py_DECREF(awaitable);

    Test_SetNoMemory();
    PyObject *fail_alloc = PyAwaitable_New();
    Test_UnSetNoMemory();
    EXPECT_ERROR(PyExc_MemoryError);
    TEST_ASSERT(fail_alloc == NULL);
    Py_RETURN_NONE;
}

static PyObject *
test_set_result(PyObject *self, PyObject *nothing)
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

static PyObject *
add_await(PyObject *self, PyObject *coro)
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

TESTS(awaitable) = {
    TEST(test_awaitable_new),
    TEST(test_set_result),
    TEST_UTIL(add_await),
    {NULL}
};
