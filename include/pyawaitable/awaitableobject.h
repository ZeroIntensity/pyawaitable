/* *INDENT-OFF* */
#ifndef PYAWAITABLE_AWAITABLE_H
#define PYAWAITABLE_AWAITABLE_H

#include <Python.h>
#include <awaitable.h>
#include <stdbool.h>

typedef struct {
  PyObject *coro;
  awaitcallback callback;
  awaitcallback_err err_callback;
  bool done;
} awaitable_callback;

struct _AwaitableObject {
  PyObject_HEAD awaitable_callback **aw_callbacks;
  Py_ssize_t aw_callback_size;
  PyObject *aw_result;
  PyObject *aw_gen;
  PyObject **aw_values;
  Py_ssize_t aw_values_size;
  void **aw_arb_values;
  Py_ssize_t aw_arb_values_size;
  Py_ssize_t aw_state;
  bool aw_done;
};

extern PyTypeObject _AwaitableType;

#endif
