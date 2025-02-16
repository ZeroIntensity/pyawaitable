#ifndef PYAWAITABLE_BACKPORT_H
#define PYAWAITABLE_BACKPORT_H

#include <Python.h>
#include <pyawaitable/dist.h>

#ifndef Py_NewRef
static inline PyObject *
_PyAwaitable_NO_MANGLE(Py_NewRef)(PyObject *o)
{
    Py_INCREF(o);
    return o;
}

#endif

#ifndef Py_XNewRef
static inline PyObject *
_PyAwaitable_NO_MANGLE(Py_XNewRef)(PyObject *o)
{
    Py_XINCREF(o);
    return o;
}
#endif

#if PY_VERSION_HEX < 0x030c0000
static PyObject *
_PyAwaitable_NO_MANGLE(PyErr_GetRaisedException)(void)
{
    PyObject *type, *val, *tb;
    PyErr_Fetch(&type, &val, &tb);
    PyErr_NormalizeException(&type, &val, &tb);
    Py_XDECREF(type);
    Py_XDECREF(tb);
    // technically some entry in the traceback might be lost; ignore that
    return val;
}

static void
_PyAwaitable_NO_MANGLE(PyErr_SetRaisedException)(PyObject *err)
{
    // NOTE: We need to incref the type object here, even though
    // this function steals a reference to err.
    PyErr_Restore(Py_NewRef((PyObject *) Py_TYPE(err)), err, NULL);
}
#endif

#endif
