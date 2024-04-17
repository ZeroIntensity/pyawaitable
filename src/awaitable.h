/* *INDENT-OFF* */
#ifndef PYAWAITABLE_H
#define PYAWAITABLE_H

#include <Python.h>

typedef int (*awaitcallback)(PyObject *, PyObject *);
typedef int (*awaitcallback_err)(PyObject *, PyObject *, PyObject *, PyObject *);

typedef struct _AwaitableObject AwaitableObject;

extern PyTypeObject AwaitableType;
extern PyTypeObject AwaitableGenWrapperType;

PyObject *awaitable_new();

int awaitable_await(PyObject *aw, PyObject *coro, awaitcallback cb, awaitcallback_err err);
void awaitable_cancel(PyObject *aw);

int awaitable_set_result(PyObject *awaitable, PyObject *result);

int awaitable_save(PyObject *awaitable, Py_ssize_t nargs, ...);
int awaitable_save_arb(PyObject *awaitable, ...);

int awaitable_unpack(PyObject *awaitable, ...);
int awaitable_unpack_arb(PyObject *awaitable, ...);

#ifdef PYAWAITABLE_PYAPI
#define PyAwaitable_New awaitable_new
#define PyAwaitable_AddAwait awaitable_await
#define PyAwaitable_Cancel awaitable_cancel
#define PyAwaitable_SetResult awaitable_set_result
#define PyAwaitable_SaveValues awaitable_save
#define PyAwaitable_SaveArbValues awaitable_save_arb
#define PyAwaitable_UnpackValues awaitable_unpack
#define PyAwaitable_UnpackArbValues awaitable_unpack_arb
#endif

#endif
