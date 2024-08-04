#ifndef PYAWAITABLE_VALUES_H
#define PYAWAITABLE_VALUES_H

#include <Python.h> // PyObject, Py_ssize_t

PyObject *pyawaitable_new_impl(void);

int pyawaitable_save_arb_impl(PyObject *awaitable, Py_ssize_t nargs, ...);

int pyawaitable_unpack_arb_impl(PyObject *awaitable, ...);

int pyawaitable_save_impl(PyObject *awaitable, Py_ssize_t nargs, ...);

int pyawaitable_unpack_impl(PyObject *awaitable, ...);

int pyawaitable_save_int_impl(PyObject *awaitable, Py_ssize_t nargs, ...);

int pyawaitable_unpack_int_impl(PyObject *awaitable, ...);

#endif
