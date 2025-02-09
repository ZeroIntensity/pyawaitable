#ifndef PYAWAITABLE_DIST_H
#define PYAWAITABLE_DIST_H

#if PY_MINOR_VERSION < 9
#error \
    "Python 3.8 and older are no longer supported, please use Python 3.9 or newer."
#endif

#ifdef _PYAWAITABLE_VENDOR
#define _PyAwaitable_API(ret) static ret
#define _PyAwaitable_INTERNAL(ret) static ret
#define _PyAwaitable_INTERNAL_DATA(tp) static tp
#define _PyAwaitable_INTERNAL_DATA_DEF(tp) static tp
#else
/* These are for IDEs */
#define _PyAwaitable_API(ret) ret
#define _PyAwaitable_INTERNAL(ret) ret
#define _PyAwaitable_INTERNAL_DATA(tp) extern tp
#define _PyAwaitable_INTERNAL_DATA_DEF(tp) tp
#define PyAwaitable_MAGIC_NUMBER 0
#endif

#define _PyAwaitable_MANGLE(name) name
#define _PyAwaitable_NO_MANGLE(name) name

#endif
