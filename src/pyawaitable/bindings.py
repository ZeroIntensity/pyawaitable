import ctypes
from ctypes import pythonapi
from typing import Any

from typing_extensions import Self

from . import abi

__all__ = "abi", "add_await", "awaitcallback", "awaitcallback_err"

get_pointer = pythonapi.PyCapsule_GetPointer
get_pointer.argtypes = (ctypes.py_object, ctypes.c_void_p)
get_pointer.restype = ctypes.c_void_p

get_name = pythonapi.PyCapsule_GetName
get_name.argtypes = (ctypes.py_object,)
get_name.restype = ctypes.c_char_p


class PyABI(ctypes.Structure):
    @classmethod
    def from_capsule(cls, capsule: Any) -> Self:
        # Assume that argtypes and restype have been properly set
        capsule_name = get_name(capsule)
        abi = ctypes.cast(
            get_pointer(capsule, capsule_name), ctypes.POINTER(cls)
        )

        return abi.contents

    def __init_subclass__(cls, **kwargs) -> None:
        super().__init_subclass__(**kwargs)
        # Assume that _fields_ is a list
        cls._fields_.insert(0, ("size", ctypes.c_ssize_t))  # type: ignore

    def __getattribute__(self, name: str) -> Any:
        size = super().__getattribute__("size")
        offset = getattr(type(self), name).offset

        if size <= offset:
            raise ValueError(f"{name!r} is not available on this ABI version")

        attr = super().__getattribute__(name)
        return attr


awaitcallback = ctypes.PYFUNCTYPE(
    ctypes.c_int, ctypes.py_object, ctypes.py_object
)
awaitcallback_err = awaitcallback


class AwaitableABI(PyABI):
    _fields_ = [
        ("new", ctypes.PYFUNCTYPE(ctypes.py_object)),
        (
            "await",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.py_object,
                awaitcallback,
                awaitcallback_err,
            ),
        ),
        ("cancel", ctypes.PYFUNCTYPE(None, ctypes.py_object)),
        (
            "set_result",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.py_object,
            ),
        ),
        (
            "save",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.c_ssize_t,
            ),
        ),
        (
            "save_arb",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.c_ssize_t,
            ),
        ),
        ("unpack", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object)),
        ("unpack_arb", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object)),
        ("PyAwaitableType", ctypes.py_object),
        (
            "await_function",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.py_object,
                ctypes.c_char_p,
                awaitcallback,
                awaitcallback_err,
            ),
        ),
        (
            "save_int",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.c_ssize_t,
            ),
        ),
        ("unpack_int", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object)),
        (
            "set",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.c_ssize_t,
                ctypes.py_object,
            ),
        ),
        (
            "set_arb",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.c_ssize_t,
                ctypes.c_void_p,
            ),
        ),
        (
            "set_int",
            ctypes.PYFUNCTYPE(
                ctypes.c_int, ctypes.py_object, ctypes.c_ssize_t, ctypes.c_long
            ),
        ),
        (
            "get",
            ctypes.PYFUNCTYPE(
                ctypes.py_object, ctypes.py_object, ctypes.c_ssize_t
            ),
        ),
        (
            "get_arb",
            ctypes.PYFUNCTYPE(
                ctypes.c_void_p, ctypes.py_object, ctypes.c_ssize_t
            ),
        ),
        (
            "get_int",
            ctypes.PYFUNCTYPE(
                ctypes.c_long, ctypes.py_object, ctypes.c_ssize_t
            ),
        ),
        (
            "async_with",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.py_object,
                awaitcallback,
                awaitcallback_err,
            ),
        ),
    ]


abi = AwaitableABI.from_capsule(abi.v1)
add_await = getattr(abi, "await")
