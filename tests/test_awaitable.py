from ward import test
import pyawaitable
import ctypes as ct
from ctypes import pythonapi

get_pointer = pythonapi.PyCapsule_GetPointer
get_pointer.argtypes = (ct.py_object, ct.c_void_p)
get_pointer.restype = ct.c_void_p

@test("awaitable creation")
async def _():
  ptr = get_pointer(pyawaitable._api)
