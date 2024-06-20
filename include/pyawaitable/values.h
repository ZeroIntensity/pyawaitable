#ifndef PYAWAITABLE_VALUES_H
#define PYAWAITABLE_VALUES_H

#include <Python.h> // PyObject

PyObject *awaitable_new_impl(void);

int awaitable_save_arb_impl(PyObject *awaitable, Py_ssize_t nargs, ...);

int awaitable_unpack_arb_impl(PyObject *awaitable, ...);

int awaitable_save_impl(PyObject *awaitable, Py_ssize_t nargs, ...);

int awaitable_unpack_impl(PyObject *awaitable, ...);

#endif
