/* *INDENT-OFF* */
#ifndef PYAWAITABLE_H
#define PYAWAITABLE_H
#include <Python.h>

#define PYAWAITABLE_MAJOR_VERSION 1
#define PYAWAITABLE_MINOR_VERSION 0
#define PYAWAITABLE_MICRO_VERSION 0

typedef int (*awaitcallback)(PyObject *, PyObject *);
typedef int (*awaitcallback_err)(PyObject *, PyObject *);

typedef struct _AwaitableObject AwaitableObject;
#define PYAWAITABLE_API_SIZE 10

void *awaitable_api[PYAWAITABLE_API_SIZE];

PyTypeObject AwaitableType;
PyTypeObject AwaitableGenWrapperType;

// PyObject *awaitable_new(void);
typedef PyObject *(*_awaitable_new_type)(void);
#define awaitable_new ((_awaitable_new_type) awaitable_api[2])

// int awaitable_await(PyObject *aw, PyObject *coro, awaitcallback cb, awaitcallback_err err);
typedef int (*_awaitable_await_type)(PyObject *, PyObject *, awaitcallback, awaitcallback_err);
#define awaitable_await ((_awaitable_await_type) awaitable_api[3])

// void awaitable_cancel(PyObject *aw);
typedef void (*_awaitable_cancel_type)(PyObject *);
#define awaitable_cancel ((_awaitable_cancel_type) awaitable_api[4])

// int awaitable_set_result(PyObject *awaitable, PyObject *result);
typedef int (*_awaitable_set_result_type)(PyObject *, PyObject *);
#define awaitable_set_result ((_awaitable_set_result_type) awaitable_api[5])

// int awaitable_save(PyObject *awaitable, Py_ssize_t nargs, ...);
typedef int (*_awaitable_save_type)(PyObject *, Py_ssize_t, ...);
#define awaitable_save ((_awaitable_save_type) awaitable_api[6])

// int awaitable_save_arb(PyObject *awaitable, Py_ssize_t nargs, ...);
#define awaitable_save_arb ((_awaitable_save_type) awaitable_api[7])

// int awaitable_unpack(PyObject *awaitable, ...);
typedef int (*_awaitable_unpack_type)(PyObject *, ...);
#define awaitable_unpack ((_awaitable_unpack_type) awaitable_api[8])

// int awaitable_unpack_arb(PyObject *awaitable, ...);
#define awaitable_unpack_arb ((_awaitable_unpack_type) awaitable_api[9])

static int
awaitable_init()
{
    PyObject *pyawaitable = PyImport_ImportModule("pyawaitable");
    if (pyawaitable == NULL)
        return -1;

    PyObject *c_api = PyObject_GetAttrString(pyawaitable, "_api");
    Py_DECREF(pyawaitable);

    if (c_api == NULL)
        return -1;

    if (!PyCapsule_CheckExact(c_api)) {
        PyErr_SetString(PyExc_TypeError, "pyawaitable._api is not a capsule");
        Py_DECREF(c_api);
        return -1;
    }

    void** api = PyCapsule_GetPointer(c_api, NULL);
    AwaitableType = *((PyTypeObject*) api[0]);
    AwaitableGenWrapperType = *((PyTypeObject*) api[1]);

    for (int i = 2; i < PYAWAITABLE_API_SIZE; ++i)
        awaitable_api[i] = api[i];

    return 0;
}

#ifdef PYAWAITABLE_PYAPI
#define PyAwaitable_New awaitable_new
#define PyAwaitable_AddAwait awaitable_await
#define PyAwaitable_Cancel awaitable_cancel
#define PyAwaitable_SetResult awaitable_set_result
#define PyAwaitable_SaveValues awaitable_save
#define PyAwaitable_SaveArbValues awaitable_save_arb
#define PyAwaitable_UnpackValues awaitable_unpack
#define PyAwaitable_UnpackArbValues awaitable_unpack_arb
#define PyAwaitable_Init awaitable_init
#define PyAwaitable_API awaitable_api
#endif

#endif
