#ifndef PYAWAITABLE_BACKPORT_H
#define PYAWAITABLE_BACKPORT_H
#include <Python.h>

#ifndef _PyObject_Vectorcall
PyObject *_PyObject_VectorcallBackport(
    PyObject *obj,
    PyObject **args,
    size_t nargsf,
    PyObject *kwargs
);

#define PyObject_CallNoArgs(o) PyObject_CallObject(o, NULL)
#define PyObject_Vectorcall _PyObject_VectorcallBackport
#define PyObject_VectorcallDict _PyObject_FastCallDict
#endif

#if PY_VERSION_HEX < 0x030c0000
PyObject *PyErr_GetRaisedException(void);
void PyErr_SetRaisedException(PyObject *err);
#endif

#ifndef Py_NewRef
PyObject *Py_NewRef_Backport(PyObject *o);
#define Py_NewRef Py_NewRef_Backport
#endif

#ifndef Py_XNewRef
PyObject *Py_XNewRef_Backport(PyObject *o);
#define Py_XNewRef Py_XNewRef_Backport
#endif

#endif
