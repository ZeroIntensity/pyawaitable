/* *INDENT-OFF* */
// This code follows PEP 7 and CPython ABI conventions
// Stdlib headers
#include <stdarg.h>
#include <stdbool.h>

// Python headers
#include <Python.h>
#include "pyerrors.h"

// PyAwaitable headers
#include <awaitable.h>
#include <pyawaitable/util.h>
#include <pyawaitable/backport.h>

static PyModuleDef awaitable_module = {
    PyModuleDef_HEAD_INIT,
    "_pyawaitable",
    NULL,
    -1
};

static AwaitableABI _abi_interface = {
    sizeof(AwaitableABI),
    _awaitable_new,
    _awaitable_await,
    _awaitable_cancel,
    _awaitable_set_result,
    _awaitable_save,
    _awaitable_save_arb,
    _awaitable_unpack,
    _awaitable_unpack_arb,
    &_AwaitableType
};

PyMODINIT_FUNC PyInit__pyawaitable()
{
    PY_TYPE_IS_READY_OR_RETURN_NULL(_AwaitableType);
    PY_TYPE_IS_READY_OR_RETURN_NULL(_AwaitableGenWrapperType);
    PY_CREATE_MODULE(awaitable_module);
    PY_TYPE_ADD_TO_MODULE_OR_RETURN_NULL(_awaitable, _AwaitableType);
    PY_TYPE_ADD_TO_MODULE_OR_RETURN_NULL(_genwrapper, _AwaitableGenWrapperType);

    PY_ADD_CAPSULE_TO_MODULE_OR_RETURN_NULL(abiv1, &_abi_interface, "pyawaitable.abi.v1");
    return m;
}
