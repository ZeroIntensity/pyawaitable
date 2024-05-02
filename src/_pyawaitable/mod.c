/* *INDENT-OFF* */
// This code follows PEP 7 and CPython ABI conventions

#include <awaitable.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/coro.h>
#include <pyawaitable/values.h>
#include <pyawaitable/genwrapper.h>
#include <pyawaitable/util.h>

static PyModuleDef awaitable_module = {
    PyModuleDef_HEAD_INIT,
    "_pyawaitable",
    NULL,
    -1
};

static AwaitableABI _abi_interface = {
    sizeof(AwaitableABI),
    awaitable_new_impl,
    awaitable_await_impl,
    awaitable_cancel_impl,
    awaitable_set_result_impl,
    awaitable_save_impl,
    awaitable_save_arb_impl,
    awaitable_unpack_impl,
    awaitable_unpack_arb_impl,
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
