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
test_add_await(PyObject *self, PyObject *coro)
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    TEST_ASSERT(PyAwaitable_AddAwait(awaitable, self, NULL, NULL) < 0);
    EXPECT_ERROR(PyExc_TypeError);

    TEST_ASSERT(PyAwaitable_AddAwait(awaitable, NULL, NULL, NULL) < 0);
    EXPECT_ERROR(PyExc_ValueError);

    TEST_ASSERT(PyAwaitable_AddAwait(awaitable, awaitable, NULL, NULL) < 0);
    EXPECT_ERROR(PyExc_ValueError);

    if (PyAwaitable_AddAwait(awaitable, coro, NULL, NULL) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    return Test_RunAwaitable(awaitable);
}

static PyObject *
test_add_await_no_memory(PyObject *self, PyObject *coro)
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    // Exhaust any preallocated buffers
    for (int i = 0; i < 16; ++i) {
        PyObject *dummy = PyAwaitable_New();
        if (dummy == NULL) {
            return NULL;
        }

        if (PyAwaitable_AddAwait(awaitable, dummy, NULL, NULL) < 0) {
            Py_DECREF(awaitable);
            Py_DECREF(dummy);
            return NULL;
        }

        TEST_ASSERT(Py_REFCNT(dummy) > 1);
        Py_DECREF(dummy);
    }


    int res;
    Test_SetNoMemory();
    res = PyAwaitable_AddAwait(awaitable, NULL, NULL, NULL);
    Test_UnSetNoMemory();
    EXPECT_ERROR(PyExc_MemoryError);
    TEST_ASSERT(res < 0);

    Test_SetNoMemory();
    res = PyAwaitable_AddAwait(awaitable, awaitable, NULL, NULL);
    Test_UnSetNoMemory();
    EXPECT_ERROR(PyExc_MemoryError);
    TEST_ASSERT(res < 0);

    Test_SetNoMemory();
    res = PyAwaitable_AddAwait(awaitable, coro, NULL, NULL);
    Test_UnSetNoMemory();
    EXPECT_ERROR(PyExc_MemoryError);
    TEST_ASSERT(res < 0);

    // Actually await it to prevent the warning
    if (PyAwaitable_AddAwait(awaitable, coro, NULL, NULL) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    return Test_RunAwaitable(awaitable);
}

TESTS(awaitable) = {
    TEST(test_awaitable_new),
    TEST(test_set_result),
    TEST_CORO(test_add_await),
    TEST_CORO(test_add_await_no_memory),
    {NULL}
};
