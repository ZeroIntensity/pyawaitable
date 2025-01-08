#ifndef PYAWAITABLE_VALUES_H
#define PYAWAITABLE_VALUES_H

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

SAVE(pyawaitable_save_impl);
UNPACK(pyawaitable_unpack_impl);
SET(pyawaitable_set_impl, PyObject *);
GET(pyawaitable_get_impl, PyObject *);

// Arbitrary values

SAVE(pyawaitable_save_arb_impl);
UNPACK(pyawaitable_unpack_arb_impl);
SET(pyawaitable_set_arb_impl, void *);
GET(pyawaitable_get_arb_impl, void *);

// Integer values

SAVE(pyawaitable_save_int_impl);
UNPACK(pyawaitable_unpack_int_impl);
SET(pyawaitable_set_int_impl, long);
GET(pyawaitable_get_int_impl, long);

#undef SAVE
#undef UNPACK
#undef GET
#undef SET

#endif
