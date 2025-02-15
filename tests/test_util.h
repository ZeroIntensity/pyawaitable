#ifndef PYAWAITABLE_TEST_UTIL_H
#define PYAWAITABLE_TEST_UTIL_H

#include <Python.h>

#define TEST(name) {#name, name, METH_NOARGS, NULL}
#define TEST_UTIL(name) {#name, name, METH_O, NULL}
#define TEST_ERROR(msg)           \
        PyErr_Format(             \
    PyExc_AssertionError,         \
    "%s (" __FILE__ ":%d): " msg, \
    __func__,                     \
    __LINE__                      \
        );
#define TEST_ASSERT(cond)                               \
        do {                                            \
            if (!(cond)) {                              \
                PyErr_Format(                           \
    PyExc_AssertionError,                               \
    "assertion failed in %s (" __FILE__ ":%d): " #cond, \
    __func__,                                           \
    __LINE__                                            \
                );                                      \
                return NULL;                            \
            }                                           \
        } while (0)
#define TESTS(name) PyMethodDef _pyawaitable_test_ ## name []
#define EXPECT_ERROR(tp)                                    \
        do {                                                \
            if (!PyErr_Occurred()) {                        \
                TEST_ERROR(                                 \
    "expected " #tp " to be raised, but nothing happened"   \
                );                                          \
                return NULL;                                \
            }                                               \
            if (!PyErr_ExceptionMatches((PyObject *)tp)) {  \
                /* Let the unexpected error fall through */ \
                return NULL;                                \
            }                                               \
            PyErr_Clear();                                  \
        } while (0)

void Test_SetNoMemory(void);
void Test_UnSetNoMemory(void);
PyObject *Test_RunAwaitable(PyObject *awaitable);

PyObject *
_Test_RunAndCheck(
    PyObject *awaitable,
    PyObject *expected,
    const char *func,
    const char *file,
    int line
);

#define Test_RunAndCheck(aw, ex) \
        _Test_RunAndCheck(       \
    aw,                          \
    ex,                          \
    __func__,                    \
    __FILE__,                    \
    __LINE__                     \
        );

#endif
