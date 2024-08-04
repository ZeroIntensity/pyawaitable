#include <pyawaitable.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/coro.h>
#include <pyawaitable/values.h>
#include <pyawaitable/genwrapper.h>
#define ADD_TYPE(tp)                                               \
        do                                                         \
        {                                                          \
            Py_INCREF(&tp);                                        \
            if (PyType_Ready(&tp) < 0) {                           \
                Py_DECREF(&tp);                                    \
                Py_DECREF(m);                                      \
                return NULL;                                       \
            }                                                      \
            if (PyModule_AddObject(m, #tp, (PyObject *)&tp) < 0) { \
                Py_DECREF(&tp);                                    \
                Py_DECREF(m);                                      \
                return NULL;                                       \
            }                                                      \
        } while (0)

static PyModuleDef awaitable_module =
{
    PyModuleDef_HEAD_INIT,
    "_pyawaitable",
    NULL,
    -1
};

/*
 * This is the ABI definition.
 *
 * You can only append to this structure.
 * Never ever remove, move, or change the size of an existing field.
 */
static PyAwaitableABI _abi_interface =
{
    sizeof(PyAwaitableABI),
    pyawaitable_new_impl,
    pyawaitable_await_impl,
    pyawaitable_cancel_impl,
    pyawaitable_set_result_impl,
    pyawaitable_save_impl,
    pyawaitable_save_arb_impl,
    pyawaitable_unpack_impl,
    pyawaitable_unpack_arb_impl,
    &_PyAwaitableType,
    pyawaitable_await_function_impl,
    pyawaitable_save_int_impl,
    pyawaitable_unpack_int_impl
};

static void
close_pool(PyObject *Py_UNUSED(capsule))
{
    dealloc_awaitable_pool();
}

PyMODINIT_FUNC
PyInit__pyawaitable(void)
{
    PyObject *m = PyModule_Create(&awaitable_module);
    ADD_TYPE(_PyAwaitableType);
    ADD_TYPE(_PyAwaitableGenWrapperType);
    PyObject *capsule = PyCapsule_New(
        &_abi_interface,
        "_pyawaitable.abi_v1",
        close_pool
    );

    if (!capsule)
    {
        Py_DECREF(m);
        return NULL;
    }

    if (PyModule_AddObject(m, "abi_v1", capsule) < 0)
    {
        Py_DECREF(m);
        Py_DECREF(capsule);
        return NULL;
    }

    if (alloc_awaitable_pool() < 0)
    {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
