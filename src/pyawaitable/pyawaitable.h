#ifndef PYAWAITABLE_H
#define PYAWAITABLE_H
#include <Python.h>

#define PYAWAITABLE_MAJOR_VERSION 1
#define PYAWAITABLE_MINOR_VERSION 0
#define PYAWAITABLE_MICRO_VERSION 0
/* Per CPython Conventions: 0xA for alpha, 0xB for beta, 0xC for release candidate or 0xF for final. */
#define PYAWAITABLE_RELEASE_LEVEL 0xC

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
} PyAwaitableABI;

#ifdef PYAWAITABLE_THIS_FILE_INIT
PyAwaitableABI *pyawaitable_abi = NULL;
#else
extern PyAwaitableABI *pyawaitable_abi;
#endif

#define pyawaitable_new pyawaitable_abi->new

#define pyawaitable_await pyawaitable_abi->await

#define pyawaitable_cancel pyawaitable_abi->cancel

#define pyawaitable_set_result pyawaitable_abi->set_result

#define pyawaitable_save pyawaitable_abi->save

#define pyawaitable_save_arb pyawaitable_abi->save_arb

#define pyawaitable_unpack pyawaitable_abi->unpack

#define pyawaitable_unpack_arb pyawaitable_abi->unpack_arb

#define PyAwaitableType pyawaitable_abi->PyAwaitableType

#define pyawaitable_await_function pyawaitable_abi->await_function

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
#define PyAwaitable_New pyawaitable_new
#define PyAwaitable_AddAwait pyawaitable_await
#define PyAwaitable_Cancel pyawaitable_cancel
#define PyAwaitable_SetResult pyawaitable_set_result
#define PyAwaitable_SaveValues pyawaitable_save
#define PyAwaitable_SaveArbValues pyawaitable_save_arb
#define PyAwaitable_UnpackValues pyawaitable_unpack
#define PyAwaitable_UnpackArbValues pyawaitable_unpack_arb
#define PyAwaitable_Init pyawaitable_init
#define PyAwaitable_ABI pyawaitable_abi
#define PyAwaitable_Type PyAwaitableType
#define PyAwaitable_AwaitFunction pyawaitable_await_function
#endif

#endif
