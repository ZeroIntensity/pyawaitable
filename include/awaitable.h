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

typedef struct _awaitable_abi {
  Py_ssize_t size;
  PyObject *(*awaitable_new)(void);
  int (*awaitable_await)(PyObject *, PyObject *, awaitcallback,
                         awaitcallback_err);
  void (*awaitable_cancel)(PyObject *);
  int (*awaitable_set_result)(PyObject *, PyObject *);
  int (*awaitable_save)(PyObject *, Py_ssize_t, ...);
  int (*awaitable_save_arb)(PyObject *, Py_ssize_t, ...);
  int (*awaitable_unpack)(PyObject *, ...);
  int (*awaitable_unpack_arb)(PyObject *, ...);
  PyTypeObject *AwaitableType;
} AwaitableABI;

#ifndef AWAITABLE_ABI_EXTERN
#define AWAITABLE_ABI_EXTERN static
#else
#define AWAITABLE_ABI_EXTERN extern
#endif

#ifndef AWAITABLE_ABI_DECLARE
AWAITABLE_ABI_EXTERN AwaitableABI *awaitable_abi = NULL;
#else
AWAITABLE_ABI_EXTERN AwaitableABI *awaitable_abi;
#endif

#undef AWAITABLE_ABI_EXTERN  // so users of this header file cannot use this macro.

// PyObject *awaitable_new(void);
#define awaitable_new awaitable_abi->awaitable_new

// int awaitable_await(PyObject *aw, PyObject *coro, awaitcallback cb,
// awaitcallback_err err);
#define awaitable_await awaitable_abi->awaitable_await

// void awaitable_cancel(PyObject *aw);
#define awaitable_cancel awaitable_abi->awaitable_cancel

// int awaitable_set_result(PyObject *awaitable, PyObject *result);
#define awaitable_set_result awaitable_abi->awaitable_set_result

// int awaitable_save(PyObject *awaitable, Py_ssize_t nargs, ...);
#define awaitable_save awaitable_abi->awaitable_save

// int awaitable_save_arb(PyObject *awaitable, Py_ssize_t nargs, ...);
#define awaitable_save_arb awaitable_abi->awaitable_save_arb

// int awaitable_unpack(PyObject *awaitable, ...);
#define awaitable_unpack awaitable_abi->awaitable_unpack

// int awaitable_unpack_arb(PyObject *awaitable, ...);
#define awaitable_unpack_arb awaitable_abi->awaitable_unpack_arb

#define AwaitableType awaitable_abi->AwaitableType

static int awaitable_init() {
  if (awaitable_abi != NULL)
    return 0;

  AwaitableABI *capsule = PyCapsule_Import("pyawaitable.abi.v1", 0);
  if (capsule == NULL)
    return -1;

  awaitable_abi = capsule;
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
#define PyAwaitable_ABI awaitable_abi
#define PyAwaitable_Type AwaitableType
#endif

#endif
