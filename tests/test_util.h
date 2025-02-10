#ifndef PYAWAITABLE_TEST_UTIL_H
#define PYAWAITABLE_TEST_UTIL_H

#include <Python.h>

#define TEST(name) {#name, name, METH_NOARGS, NULL}
#define TEST_UTIL(name) {#name, name, METH_O, NULL}
#define TEST_ASSERT(cond)                                  \
        do {                                               \
            if (!(cond)) {                                 \
                PyErr_SetString(                           \
    PyExc_AssertionError,                                  \
    "assertion failed: " #cond " at" __FILE__ ":" __LINE__ \
                );                                         \
                return NULL;                               \
            }                                              \
        } while (0)
#define TESTS(name) PyMethodDef _pyawaitable_test_ ## name []
#define SENTINEL() {NULL, 0, NULL, NULL}

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
