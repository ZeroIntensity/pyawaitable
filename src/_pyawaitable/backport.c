#include <Python.h>
#include <pyawaitable/backport.h>

#ifdef PYAWAITABLE_NEEDS_VECTORCALL
PyObject *
_PyObject_VectorcallBackport(
    PyObject *obj,
    PyObject **args,
    size_t nargsf,
    PyObject *kwargs
)
{
    PyObject *tuple = PyTuple_New(nargsf);
    if (!tuple)
        return NULL;
    for (size_t i = 0; i < nargsf; i++)
    {
        Py_INCREF(args[i]);
        PyTuple_SET_ITEM(tuple, i, args[i]);
    }
    PyObject *o = PyObject_Call(obj, tuple, kwargs);
    Py_DECREF(tuple);
    return o;
}

#endif

#if PY_VERSION_HEX < 0x030c0000
PyObject *
PyErr_GetRaisedException(void)
{
    PyObject *type, *val, *tb;
    PyErr_Fetch(&type, &val, &tb);
    PyErr_NormalizeException(&type, &val, &tb);
    Py_XDECREF(type);
    Py_XDECREF(tb);
    // technically some entry in the traceback might be lost; ignore that
    return val;
}

void
PyErr_SetRaisedException(PyObject *err)
{
    // NOTE: We need to incref the type object here, even though
    // this function steals a reference to err.
    PyErr_Restore(Py_NewRef((PyObject *) Py_TYPE(err)), err, NULL);
}

#endif

#ifdef PYAWAITABLE_NEEDS_NEWREF
PyObject *
Py_NewRef_Backport(PyObject *o)
{
    Py_INCREF(o);
    return o;
}

#endif

#ifdef PYAWAITABLE_NEEDS_XNEWREF
PyObject *
Py_XNewRef_Backport(PyObject *o)
{
    Py_XINCREF(o);
    return o;
}

#endif
