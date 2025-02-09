#ifndef PYAWAITABLE_OPTIMIZE_H
#define PYAWAITABLE_OPTIMIZE_H

#if (defined(__GNUC__) && __GNUC__ >= 15) || defined(__clang__) && __clang__ >= 13
#define PyAwaitable_MUSTTAIL [[clang::musttail]]
#else
#define PyAwaitable_MUSTTAIL
#endif

#if defined(__GNUC__) || defined(__clang__)
/* Called often */
#define PyAwaitable_HOT __attribute__((hot))
/* Depends only on input and memory state (i.e. makes no memory allocations */
#define PyAwaitable_PURE __attribute__((pure))
/* Depends only on inputs */
#define PyAwaitable_CONST __attribute__((const))
/* Called rarely */
#define PyAwaitable_COLD __attribute__((cold))
#else
#define PyAwaitable_HOT
#define PyAwaitable_PURE
#define PyAwaitable_CONST
#define PyAwaitable_COLD
#endif

#if defined(__GNUC__) || defined(__clang__)
#define PyAwaitable_UNLIKELY(x)     (__builtin_expect(!!(x),false))
#define PyAwaitable_LIKELY(x)       (__builtin_expect(!!(x),true))
#elif (defined(__cplusplus) && (__cplusplus >= 202002L)) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#define PyAwaitable_UNLIKELY(x)     (x) [[unlikely]]
#define PyAwaitable_LIKELY(x)       (x) [[likely]]
#else
#define PyAwaitable_UNLIKELY(x)     (x)
#define PyAwaitable_LIKELY(x)       (x)
#endif

#endif
