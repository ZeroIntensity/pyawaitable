import ctypes
from ctypes import pythonapi

import pyawaitable

get_pointer = pythonapi.PyCapsule_GetPointer
get_pointer.argtypes = (ctypes.py_object, ctypes.c_void_p)
get_pointer.restype = ctypes.c_void_p

awaitcallback = ctypes.CFUNCTYPE(ctypes.py_object, ctypes.py_object)
awaitcallback_err = awaitcallback

# Initialize API array
ptr = get_pointer(pyawaitable._api, None)
api = (ctypes.c_void_p * 10).from_address(ptr)

# Types
AwaitableType = ctypes.cast(api[0], ctypes.py_object).value
AwaitableGenWrapperType = ctypes.cast(api[1], ctypes.py_object).value

# API Functions
awaitable_new = ctypes.cast(api[2], ctypes.CFUNCTYPE(ctypes.py_object))
awaitable_await = ctypes.cast(
    api[3],
    ctypes.CFUNCTYPE(
        ctypes.c_int,
        ctypes.py_object,
        ctypes.py_object,
        awaitcallback,
        awaitcallback_err,
    ),
)

pythonapi.PyGILState_Ensure.restype = ctypes.c_long
pythonapi.PyGILState_Release.argtypes = (ctypes.c_long,)

state = pythonapi.PyGILState_Ensure()
awaitable_new()
