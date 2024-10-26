#ifndef PYAWAITABLE_H
#define PYAWAITABLE_H
#include <Python.h>

#define PYAWAITABLE_MAJOR_VERSION 1
#define PYAWAITABLE_MINOR_VERSION 3
#define PYAWAITABLE_MICRO_VERSION 0
/* Per CPython Conventions: 0xA for alpha, 0xB for beta, 0xC for release candidate or 0xF for final. */
#define PYAWAITABLE_RELEASE_LEVEL 0xF

typedef int (*awaitcallback)(PyObject *, PyObject *);
typedef int (*awaitcallback_err)(PyObject *, PyObject *);

typedef struct _PyAwaitableObject PyAwaitableObject;

typedef struct _pyawaitable_abi
{
    Py_ssize_t size;
    PyObject *(*new)(void);
    int (*await)(
        PyObject *,
        PyObject *,
        awaitcallback,
        awaitcallback_err
    );
    void (*cancel)(PyObject *);
    int (*set_result)(PyObject *, PyObject *);
    int (*save)(PyObject *, Py_ssize_t, ...);
    int (*save_arb)(PyObject *, Py_ssize_t, ...);
    int (*unpack)(PyObject *, ...);
    int (*unpack_arb)(PyObject *, ...);
    PyTypeObject *PyAwaitableType;
    int (*await_function)(
        PyObject *,
        PyObject *,
        const char *fmt,
        awaitcallback,
        awaitcallback_err,
        ...
    );
    int (*save_int)(PyObject *, Py_ssize_t nargs, ...);
    int (*unpack_int)(PyObject *, ...);
    int (*set)(PyObject *, Py_ssize_t, PyObject *);
    int (*set_arb)(PyObject *, Py_ssize_t, void *);
    int (*set_int)(PyObject *, Py_ssize_t, long);
    PyObject *(*get)(PyObject *, Py_ssize_t);
    void *(*get_arb)(PyObject *, Py_ssize_t);
    long (*get_int)(PyObject *, Py_ssize_t);
    int (*async_with)(
        PyObject *aw,
        PyObject *ctx,
        awaitcallback cb,
        awaitcallback_err err
    );
} PyAwaitableABI;

#ifdef PYAWAITABLE_THIS_FILE_INIT
PyAwaitableABI *pyawaitable_abi = NULL;
#else
extern PyAwaitableABI *pyawaitable_abi;
#endif

#define pyawaitable_new pyawaitable_abi->new
#define pyawaitable_cancel pyawaitable_abi->cancel
#define pyawaitable_set_result pyawaitable_abi->set_result

#define pyawaitable_await pyawaitable_abi->await
#define pyawaitable_await_function pyawaitable_abi->await_function
#define pyawaitable_async_with pyawaitable_abi->async_with

#define pyawaitable_save pyawaitable_abi->save
#define pyawaitable_save_arb pyawaitable_abi->save_arb
#define pyawaitable_save_int pyawaitable_abi->save_int

#define pyawaitable_unpack pyawaitable_abi->unpack
#define pyawaitable_unpack_arb pyawaitable_abi->unpack_arb
#define pyawaitable_unpack_int pyawaitable_abi->unpack_int

#define pyawaitable_set pyawaitable_abi->set
#define pyawaitable_set_arb pyawaitable_abi->set_arb
#define pyawaitable_set_int pyawaitable_abi->set_int

#define pyawaitable_get pyawaitable_abi->get
#define pyawaitable_get_arb pyawaitable_abi->get_arb
#define pyawaitable_get_int pyawaitable_abi->get_int

#define PyAwaitableType pyawaitable_abi->PyAwaitableType


#ifdef PYAWAITABLE_THIS_FILE_INIT
static int
pyawaitable_init()
{
    if (pyawaitable_abi != NULL)
        return 0;

    PyAwaitableABI *capsule = PyCapsule_Import("_pyawaitable.abi_v1", 0);
    if (capsule == NULL)
        return -1;

    pyawaitable_abi = capsule;
    return 0;
}

#else
static int
pyawaitable_init()
{
    PyErr_SetString(
        PyExc_RuntimeError,
        "pyawaitable_init() can only be called in a file with a PYAWAITABLE_THIS_FILE_INIT #define"
    );
    return -1;
}

#endif

#ifdef PYAWAITABLE_PYAPI
#define PyAwaitable_Init pyawaitable_init
#define PyAwaitable_ABI pyawaitable_abi
#define PyAwaitable_Type PyAwaitableType

#define PyAwaitable_New pyawaitable_new
#define PyAwaitable_Cancel pyawaitable_cancel
#define PyAwaitable_SetResult pyawaitable_set_result

#define PyAwaitable_AddAwait pyawaitable_await
#define PyAwaitable_AwaitFunction pyawaitable_await_function
#define PyAwaitable_AsyncWith pyawaitable_async_with

#define PyAwaitable_SaveValues pyawaitable_save
#define PyAwaitable_SaveArbValues pyawaitable_save_arb
#define PyAwaitable_SaveIntValues pyawaitable_save_int

#define PyAwaitable_UnpackValues pyawaitable_unpack
#define PyAwaitable_UnpackArbValues pyawaitable_unpack_arb
#define PyAwaitable_UnpackIntValues pyawaitable_unpack_int

#define PyAwaitable_SetValue pyawaitable_set
#define PyAwaitable_SetArbValue pyawaitable_set_arb
#define PyAwaitable_SetIntValue pyawaitable_set_int

#define PyAwaitable_GetValue pyawaitable_set
#define PyAwaitable_GetArbValue pyawaitable_get_arb
#define PyAwaitable_GetIntValue pyawaitable_get_int
#endif

static int
pyawaitable_vendor_init(PyObject *mod)
{
    PyErr_SetString(
        PyExc_SystemError,
        "cannot use pyawaitable_vendor_init from an installed version, use pyawaitable_init instead!"
    );
    return -1;
}

#endif
