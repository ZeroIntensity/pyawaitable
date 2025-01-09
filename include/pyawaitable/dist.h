#ifndef PYAWAITABLE_DIST_H
#define PYAWAITABLE_DIST_H

#ifdef Py_LIMITED_API
#error "the limited API cannot be used with pyawaitable"
#endif

#ifdef _PYAWAITABLE_VENDOR
#define _PyAwaitable_API(ret) static ret
#define _PyAwaitable_INTERNAL(ret) static ret
#else
#define _PyAwaitable_API(ret) ret
#define _PyAwaitable_INTERNAL(ret) ret
#endif

#define _PyAwaitable_MANGLE(name) name
#define _PyAwaitable_NO_MANGLE(name) name

#define PyAwaitable_MAJOR_VERSION 2
#define PyAwaitable_MINOR_VERSION 0
#define PyAwaitable_MICRO_VERSION 0
#define PyAwaitable_PATCH_VERSION PyAwaitable_MINOR_VERSION

#endif
