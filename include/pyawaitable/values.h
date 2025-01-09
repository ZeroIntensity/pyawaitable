#ifndef PYAWAITABLE_VALUES_H
#define PYAWAITABLE_VALUES_H

#include <Python.h> // PyObject, Py_ssize_t

#define SAVE(name) int name(PyObject * awaitable, Py_ssize_t nargs, ...)
#define UNPACK(name) int name(PyObject * awaitable, ...)
#define SET(name, tp)     \
        int name(         \
    PyObject * awaitable, \
    Py_ssize_t index,     \
    tp new_value          \
        )
#define GET(name, tp)     \
        tp name(          \
    PyObject * awaitable, \
    Py_ssize_t index      \
        )

// Normal values

SAVE(pyawaitable_save);
UNPACK(pyawaitable_unpack);
SET(pyawaitable_set, PyObject *);
GET(pyawaitable_get, PyObject *);

// Arbitrary values

SAVE(pyawaitable_save_arb);
UNPACK(pyawaitable_unpack_arb);
SET(pyawaitable_set_arb, void *);
GET(pyawaitable_get_arb, void *);

// Integer values

SAVE(pyawaitable_save_int);
UNPACK(pyawaitable_unpack_int);
SET(pyawaitable_set_int, long);
GET(pyawaitable_get_int, long);

#undef SAVE
#undef UNPACK
#undef GET
#undef SET

#endif
