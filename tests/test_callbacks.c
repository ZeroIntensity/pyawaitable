#include <Python.h>
#include <pyawaitable.h>
#include "pyawaitable_test.h"
#include "pyerrors.h"

static int _Thread_local callback_called = 0;
static int _Thread_local error_callback_called = 0;

static int
simple_callback(PyObject *awaitable, PyObject *value)
{
    TEST_ASSERT_INT(awaitable != NULL);
    TEST_ASSERT_INT(value == Py_None);
    TEST_ASSERT_INT(Py_IS_TYPE(awaitable, PyAwaitable_GetType()));
    TEST_ASSERT_INT(callback_called == 0);
    callback_called = 1;
    return 0;
}

static int
aborting_callback(PyObject *awaitable, PyObject *value)
{
    _PyObject_Dump(value);
    Py_FatalError("Test case shouldn't have ever reached here!");
    return 0;
}

static int
error_callback(PyObject *awaitable, PyObject *err)
{
    TEST_ASSERT_INT(!PyErr_Occurred());
    TEST_ASSERT_INT(awaitable != NULL);
    TEST_ASSERT_INT(Py_IS_TYPE(awaitable, PyAwaitable_GetType()));
    TEST_ASSERT_INT(err != NULL);
    TEST_ASSERT_INT(
        Py_IS_TYPE(
            err,
            (PyTypeObject *)PyExc_ZeroDivisionError
        )
    );
    TEST_ASSERT_INT(error_callback_called == 0);
    error_callback_called = 1;
    return 0;
}

static int
repropagating_error_callback(PyObject *awaitable, PyObject *err)
{
    TEST_ASSERT_INT(!PyErr_Occurred());
    TEST_ASSERT_INT(awaitable != NULL);
    TEST_ASSERT_INT(Py_IS_TYPE(awaitable, PyAwaitable_GetType()));
    TEST_ASSERT_INT(err != NULL);
    TEST_ASSERT_INT(
        Py_IS_TYPE(
            err,
            (PyTypeObject *)PyExc_ZeroDivisionError
        )
    );
    TEST_ASSERT_INT(error_callback_called == 0);
    error_callback_called = 1;
    return -1;
}

static int
overwriting_error_callback(PyObject *awaitable, PyObject *err)
{
    TEST_ASSERT_INT(!PyErr_Occurred());
    TEST_ASSERT_INT(awaitable != NULL);
    TEST_ASSERT_INT(Py_IS_TYPE(awaitable, PyAwaitable_GetType()));
    TEST_ASSERT_INT(err != NULL);
    TEST_ASSERT_INT(
        Py_IS_TYPE(
            err,
            (PyTypeObject *)PyExc_ZeroDivisionError
        )
    );
    TEST_ASSERT_INT(error_callback_called == 0);
    error_callback_called = 1;
    // Some random exception that probably won't occur elsewhere
    PyErr_SetNone(PyExc_FileExistsError);
    return -2;
}

static PyObject *
test_callback_is_called(PyObject *self, PyObject *coro)
{
    callback_called = 0;
    PyObject *awaitable = Test_NewAwaitableWithCoro(
        coro,
        simple_callback,
        NULL
    );
    if (awaitable == NULL) {
        return NULL;
    }
    PyObject *res = Test_RunAwaitable(awaitable);
    if (res == NULL) {
        return NULL;
    }
    Py_DECREF(res);
    TEST_ASSERT(callback_called == 1);
    Py_RETURN_NONE;
}

static PyObject *
test_callback_not_invoked_when_exception(PyObject *self, PyObject *coro)
{
    PyObject *awaitable = Test_NewAwaitableWithCoro(
        coro,
        aborting_callback,
        NULL
    );
    if (awaitable == NULL) {
        return NULL;
    }
    PyObject *res = Test_RunAwaitable(awaitable);
    EXPECT_ERROR(PyExc_ZeroDivisionError);
    Py_RETURN_NONE;
}

static PyObject *
test_error_callback_not_invoked_when_ok(PyObject *self, PyObject *coro)
{
    callback_called = 0;
    PyObject *awaitable = Test_NewAwaitableWithCoro(
        coro,
        simple_callback,
        aborting_callback
    );
    if (awaitable == NULL) {
        return NULL;
    }
    PyObject *res = Test_RunAwaitable(awaitable);
    if (res == NULL) {
        return NULL;
    }
    Py_DECREF(res);
    TEST_ASSERT(callback_called == 1);
    Py_RETURN_NONE;
}

static PyObject *
test_error_callback_gets_exception_from_coro(PyObject *self, PyObject *coro)
{
    error_callback_called = 0;
    PyObject *awaitable = Test_NewAwaitableWithCoro(
        coro,
        aborting_callback,
        error_callback
    );
    PyObject *res = Test_RunAwaitable(awaitable);
    if (res == NULL) {
        return NULL;
    }
    Py_DECREF(res);
    TEST_ASSERT(error_callback_called == 1);
    Py_RETURN_NONE;
}

static PyObject *
test_failing_error_callback_repropagates_exception(
    PyObject *self,
    PyObject *coro
)
{
    error_callback_called = 0;
    PyObject *awaitable = Test_NewAwaitableWithCoro(
        coro,
        aborting_callback,
        repropagating_error_callback
    );
    PyObject *res = Test_RunAwaitable(awaitable);
    EXPECT_ERROR(PyExc_ZeroDivisionError);
    TEST_ASSERT(res == NULL);
    TEST_ASSERT(error_callback_called == 1);
    Py_RETURN_NONE;
}

static PyObject *
test_error_callback_can_overwrite_exception(
    PyObject *self,
    PyObject *coro
)
{
    error_callback_called = 0;
    PyObject *awaitable = Test_NewAwaitableWithCoro(
        coro,
        aborting_callback,
        overwriting_error_callback
    );
    PyObject *res = Test_RunAwaitable(awaitable);
    EXPECT_ERROR(PyExc_FileExistsError);
    TEST_ASSERT(res == NULL);
    TEST_ASSERT(error_callback_called == 1);
    Py_RETURN_NONE;
}

TESTS(callbacks) = {
    TEST_CORO(test_callback_is_called),
    TEST_RAISING_CORO(test_callback_not_invoked_when_exception),
    TEST_CORO(test_error_callback_not_invoked_when_ok),
    TEST_RAISING_CORO(test_error_callback_gets_exception_from_coro),
    TEST_RAISING_CORO(test_failing_error_callback_repropagates_exception),
    TEST_RAISING_CORO(test_error_callback_can_overwrite_exception),
    {NULL}
};
