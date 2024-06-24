#ifndef PYAWAITABLE_VENDOR_H
#define PYAWAITABLE_VENDOR_H

#include <Python.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>

/*
 * vendor.h is only for use by the vendor build tool, don't use it manually!
 * (If you're seeing this message from a vendored copy, you're fine)
 */

#define PYAWAITABLE_ADD_TYPE(m, tp)                                 \
        do                                                          \
        {                                                           \
            Py_INCREF(&tp);                                         \
            if (PyType_Ready(&tp) < 0) {                            \
                Py_DECREF(&tp);                                     \
                return -1;                                          \
            }                                                       \
            if (PyModule_AddObject(m, #tp, (PyObject *) &tp) < 0) { \
                Py_DECREF(&tp);                                     \
                return -1;                                          \
            }                                                       \
        } while (0)

#define PYAWAITABLE_MAJOR_VERSION 1
#define PYAWAITABLE_MINOR_VERSION 0
#define PYAWAITABLE_MICRO_VERSION 0
#define PYAWAITABLE_RELEASE_LEVEL 0xF

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
#define PyAwaitable_VendorInit pyawaitable_vendor_init
#endif

static int
pyawaitable_init()
{
    PyErr_SetString(
        PyExc_SystemError,
        "cannot use pyawaitable_init from a vendored copy, use pyawaitable_vendor_init instead!"
    );
    return -1;
}

static void
close_pool(PyObject *Py_UNUSED(capsule))
{
    dealloc_awaitable_pool();
}

static int
pyawaitable_vendor_init(PyObject *mod)
{
    PYAWAITABLE_ADD_TYPE(mod, _PyAwaitableType);
    PYAWAITABLE_ADD_TYPE(mod, _PyAwaitableGenWrapperType);

    PyObject *capsule = PyCapsule_New(
        pyawaitable_vendor_init, // Any pointer, except NULL
        "_pyawaitable.__do_not_touch",
        close_pool
    );

    if (!capsule)
    {
        return -1;
    }

    if (PyModule_AddObject(mod, "__do_not_touch", capsule) < 0)
    {
        Py_DECREF(capsule);
        return -1;
    }

    if (alloc_awaitable_pool() < 0)
    {
        return -1;
    }

    return 0;
}

#endif
