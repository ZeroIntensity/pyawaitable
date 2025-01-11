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

#endif
