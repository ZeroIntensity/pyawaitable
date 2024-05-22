/* *INDENT-OFF* */
// This code follows PEP 7 and CPython ABI conventions

#include <pyawaitable/util.h>

void _SetAllListItems(PyObject *all_list, int count, ...) {
  va_list valist;
  va_start(valist, count);
  for (int i = 0; i < count; i++) {
    PyList_SET_ITEM(all_list, i,
                    PyUnicode_FromString(va_arg(valist, const char *)));
  }
  va_end(valist);
}

PyObject *_DecrefModuleAndReturnNULL(PyObject *m) {
  Py_XDECREF(m);
  return NULL;
}

PyObject *
_PyType_CreateInstance(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
  PyObject *instance = type->tp_new(type, Py_None, Py_None);
  if (type->tp_init(instance, args, kwargs) != 0) {
    PyErr_Print();
    Py_XDECREF(instance);
    return NULL;
  }

  return instance;
}

PyObject *_PyObject_GetCallableMethod(PyObject *obj,
                                      const char *name) {
  PyObject *method = PyObject_GetAttrString(obj, name);
  if (!PyCallable_Check(method)) {
    Py_XDECREF(method);
    return NULL;
  }

  return method;
}
