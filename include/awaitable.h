/* *INDENT-OFF* */
#ifndef PYAWAITABLE_H
#define PYAWAITABLE_H
#include <Python.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && \
    !defined(__CYGWIN__)
#define PYAWAITABLE_PLATFORM_WIN
#endif

#ifdef PYAWAITABLE_PLATFORM_WIN
    #define IMPORT __declspec(dllimport)
    #define EXPORT __declspec(dllexport)
    #define LOCAL
#else
    #ifndef __has_attribute
    #define __has_attribute(x) 0  // Compatibility with non-clang compilers.
    #endif

    #if (defined(__GNUC__) && (__GNUC__ >= 4)) || \
    (defined(__clang__) && __has_attribute(visibility))
        #define IMPORT __attribute__ ((visibility ("default")))
        #define EXPORT __attribute__ ((visibility ("default")))
        #define LOCAL __attribute__ ((visibility ("hidden")))
    #else
        #define IMPORT
        #define EXPORT
        #define LOCAL
    #endif
#endif

#ifdef PYAWAITABLE_IS_COMPILING
#define API EXPORT
#else
#define API IMPORT
#endif

typedef int (*awaitcallback)(PyObject *, PyObject *);
typedef int (*awaitcallback_err)(PyObject *, PyObject *, PyObject *, PyObject *);

typedef struct _AwaitableObject AwaitableObject;

extern API PyTypeObject AwaitableType;
extern API PyTypeObject AwaitableGenWrapperType;

API PyObject *awaitable_new(void);

API int awaitable_await(PyObject *aw, PyObject *coro, awaitcallback cb, awaitcallback_err err);
API void awaitable_cancel(PyObject *aw);

API int awaitable_set_result(PyObject *awaitable, PyObject *result);

API int awaitable_save(PyObject *awaitable, Py_ssize_t nargs, ...);
API int awaitable_save_arb(PyObject *awaitable, Py_ssize_t nargs, ...);

API int awaitable_unpack(PyObject *awaitable, ...);
API int awaitable_unpack_arb(PyObject *awaitable, ...);

API int awaitable_init(void);

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
#endif

#endif
