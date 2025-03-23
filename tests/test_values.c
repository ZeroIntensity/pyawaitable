#include <Python.h>
#include <pyawaitable.h>
#include "pyawaitable_test.h"

static PyObject *
test_store_and_load_object_values(PyObject *self, PyObject *nothing)
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    PyObject *num = PyLong_FromLong(999);
    if (num == NULL) {
        Py_DECREF(awaitable);
        return NULL;
    }

    PyObject *str = PyUnicode_FromString("hello");
    if (str == NULL) {
        Py_DECREF(awaitable);
        Py_DECREF(num);
        return NULL;
    }

    int fail = PyAwaitable_UnpackValues(awaitable);
    EXPECT_ERROR(PyExc_RuntimeError);
    TEST_ASSERT(fail == -1);

    if (PyAwaitable_SaveValues(awaitable, 2, num, str) < 0) {
        Py_DECREF(awaitable);
        Py_DECREF(num);
        Py_DECREF(str);
        return NULL;
    }
    Py_DECREF(num);
    Py_DECREF(str);
    TEST_ASSERT(Py_REFCNT(num) >= 1);
    TEST_ASSERT(Py_REFCNT(str) >= 1);

    PyObject *num_unpacked;
    PyObject *str_unpacked;

    if (
        PyAwaitable_UnpackValues(
            awaitable,
            &num_unpacked,
            &str_unpacked
        ) < 0
    ) {
        Py_DECREF(awaitable);
        return NULL;
    }

    TEST_ASSERT(num_unpacked == num);
    TEST_ASSERT(str_unpacked == str);
    PyAwaitable_Cancel(awaitable);
    Py_RETURN_NONE;
}

static PyObject *
test_object_values_can_outlive_awaitable(PyObject *self, PyObject *nothing)
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    PyObject *dummy =
        PyUnicode_FromString("nobody expects the spanish inquisition");
    if (dummy == NULL) {
        Py_DECREF(awaitable);
        return NULL;
    }

    if (PyAwaitable_SaveValues(awaitable, 1, dummy) < 0) {
        Py_DECREF(awaitable);
        Py_DECREF(dummy);
        return NULL;
    }

    PyAwaitable_Cancel(awaitable);
    Py_DECREF(awaitable);
    assert(
        PyUnicode_CompareWithASCIIString(
            dummy,
            "nobody expects the spanish inquisition"
        )
    );
    Py_DECREF(dummy);
    Py_RETURN_NONE;
}

TESTS(values) = {
    TEST(test_store_and_load_object_values),
    TEST(test_object_values_can_outlive_awaitable),
    {NULL}
};
