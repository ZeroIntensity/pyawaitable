#ifndef PYAWAITABLE_BACKPORT_H
#define PYAWAITABLE_BACKPORT_H

#ifndef _PyObject_Vectorcall
#define PyObject_CallNoArgs(o) PyObject_CallObject( \
    o, \
    NULL \
)
static PyObject* _PyObject_VectorcallBackport(PyObject* obj,
                                              PyObject** args,
                                              size_t nargsf, PyObject* kwargs) {
    PyObject* tuple = PyTuple_New(nargsf);
    if (!tuple) return NULL;
    for (size_t i = 0; i < nargsf; i++) {
        Py_INCREF(args[i]);
        PyTuple_SET_ITEM(
            tuple,
            i,
            args[i]
        );
    }
    PyObject* o = PyObject_Call(
        obj,
        tuple,
        kwargs
    );
    Py_DECREF(tuple);
    return o;
}
#define PyObject_Vectorcall _PyObject_VectorcallBackport
#define PyObject_VectorcallDict _PyObject_FastCallDict
#endif

#if PY_VERSION_HEX < 0x030c0000
PyObject *PyErr_GetRaisedException(void) {
    PyObject *type, *val, *tb;
    PyErr_Fetch(&type, &val, &tb);
    PyErr_NormalizeException(&type, &val, &tb);
    Py_XDECREF(type);
    Py_XDECREF(tb);
    // technically some entry in the traceback might be lost; ignore that
    return val;
}

void PyErr_SetRaisedException(PyObject *err) {
    PyErr_Restore(err, NULL, NULL);
}
#endif


#ifndef Py_NewRef
static inline PyObject* Py_NewRef(PyObject* o) {
    Py_INCREF(o);
    return o;
}
#endif

#ifndef Py_XNewRef
static inline PyObject* Py_XNewRef(PyObject* o) {
    Py_XINCREF(o);
    return o;
}
#endif

#endif
