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
    Py_DECREF(awaitable);
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

static PyObject *
test_store_and_load_arbitrary_values(PyObject *self, PyObject *nothing)
{
    PyObject *awaitable = PyAwaitable_New();
    if (awaitable == NULL) {
        return NULL;
    }

    int *ptr = malloc(sizeof(int));
    if (ptr == NULL) {
        Py_DECREF(awaitable);
        PyErr_NoMemory();
        return NULL;
    }

    *ptr = 42;
    if (PyAwaitable_SaveArbValues(awaitable, 2, ptr, NULL) < 0) {
        Py_DECREF(awaitable);
        free(ptr);
        return NULL;
    }

    int *ptr_unpacked;
    void *null_unpacked;
    if (
        PyAwaitable_UnpackArbValues(
            awaitable,
            &ptr_unpacked,
            &null_unpacked
        ) < 0
    ) {
        Py_DECREF(awaitable);
        free(ptr);
        return NULL;
    }
    TEST_ASSERT(ptr_unpacked == ptr);
    TEST_ASSERT((*ptr_unpacked) == 42);
    TEST_ASSERT(null_unpacked == NULL);
    free(ptr);
    PyAwaitable_Cancel(awaitable);
    Py_DECREF(awaitable);
    Py_RETURN_NONE;
}

static PyObject *
test_load_fails_when_no_values(PyObject *self, PyObject *nothing)
{
    PyObject *awaitable = PyAwaitable_New();
    PyAwaitable_Cancel(awaitable);

    int fail = PyAwaitable_UnpackValues(awaitable);
    EXPECT_ERROR(PyExc_RuntimeError);
    TEST_ASSERT(fail == -1);

    int fail_arb = PyAwaitable_UnpackArbValues(awaitable);
    EXPECT_ERROR(PyExc_RuntimeError);
    TEST_ASSERT(fail_arb == -1);

    Py_DECREF(awaitable);
    Py_RETURN_NONE;
}

static PyObject *
test_load_null_pointer(PyObject *self, PyObject *nothing)
{
    PyObject *awaitable = PyAwaitable_New();
    PyAwaitable_Cancel(awaitable);

    // It doesn't matter what this is, it just needs to be unique
    int sentinel;
    void *dummy = &sentinel;

    if (PyAwaitable_SaveArbValues(awaitable, 3, dummy, dummy, dummy) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    void *only_unpack;
    if (PyAwaitable_UnpackArbValues(awaitable, NULL, &only_unpack, NULL) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    TEST_ASSERT(only_unpack == dummy);
    Py_DECREF(awaitable);
    Py_RETURN_NONE;
}

static PyObject *
test_get_and_set_values(PyObject *self, PyObject *nothing)
{
    PyObject *awaitable = PyAwaitable_New();
    PyAwaitable_Cancel(awaitable);

    int sentinel;
    void *dummy = &sentinel;

    if (PyAwaitable_SaveArbValues(awaitable, 3, dummy, NULL, dummy) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    TEST_ASSERT(PyAwaitable_GetArbValue(awaitable, 0) == dummy);
    TEST_ASSERT(PyAwaitable_GetArbValue(awaitable, 1) == NULL);
    TEST_ASSERT(PyAwaitable_GetArbValue(awaitable, 2) == dummy);

    if (PyAwaitable_SetArbValue(awaitable, 1, dummy) < 0) {
        Py_DECREF(awaitable);
        return NULL;
    }

    TEST_ASSERT(PyAwaitable_GetArbValue(awaitable, 1) == dummy);

    void *fail = PyAwaitable_GetArbValue(awaitable, 4);
    EXPECT_ERROR(PyExc_IndexError);
    TEST_ASSERT(fail == NULL);

    int other_fail = PyAwaitable_SetArbValue(awaitable, 5, dummy);
    EXPECT_ERROR(PyExc_IndexError);
    TEST_ASSERT(other_fail < 0);

    Py_DECREF(awaitable);
    Py_RETURN_NONE;
}

TESTS(values) = {
    TEST(test_store_and_load_object_values),
    TEST(test_object_values_can_outlive_awaitable),
    TEST(test_store_and_load_arbitrary_values),
    TEST(test_load_fails_when_no_values),
    TEST(test_load_null_pointer),
    {NULL}
};
