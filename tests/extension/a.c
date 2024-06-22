#include <Python.h>
#include <pyawaitable.h>

PyObject *
test2(PyObject *self, PyObject *args)
{
    return pyawaitable_new();
}
