/*
 * defines.h
 */
#pragma once
#include <Python.h>
#include <structmember.h>
#include <datetime.h>
#include <stdbool.h>

/* inline helpers. */
static inline void _SetAllListItems(PyObject *all_list, int count, ...) {
  va_list valist;
  va_start(valist, count);
  for (int i = 0; i < count; i++) {
    PyList_SET_ITEM(all_list, i, PyUnicode_FromString(va_arg(valist, const char *)));
  }
  va_end(valist);
}

static inline PyObject *_DecrefModuleAndReturnNULL(PyObject *m) {
  Py_XDECREF(m);
  return NULL;
}

static inline PyObject *_PyType_CreateInstance(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
  PyObject *instance = type->tp_new(type, Py_None, Py_None);
  if (type->tp_init(instance, args, kwargs) != 0) {
    PyErr_Print();
    Py_XDECREF(instance);
    return NULL;
  }

  return instance;
}

static inline PyObject *_PyObject_GetCallableMethod(PyObject *obj, const char *name) {
  PyObject *method = PyObject_GetAttrString(obj, name);
  if (!PyCallable_Check(method)) {
    Py_XDECREF(method);
    return NULL;
  }

  return method;
}

/* defines needed in the module init function. If only they were in the Python.h include file. */
#define PY_CREATE_MODULE(moduledef) \
PyObject *m; \
m = PyModule_Create(&moduledef); \
if (m == NULL) \
  return NULL
#define PY_CREATE_MODULE_AND_DECREF_ON_FAILURE(moduledef, decref) \
PyObject *m; \
m = PyModule_Create(&moduledef); \
if (m == NULL) { \
  Py_XDECREF(decref); \
  return NULL; \
}
#define PY_TYPE_IS_READY_OR_RETURN_NULL(type) \
if (PyType_Ready(&type) < 0) \
  return NULL
#define PY_TYPE_ADD_TO_MODULE_OR_RETURN_NULL(name, type) \
if (PyModule_AddObject(m, Py_STRINGIFY(name), Py_NewRef(&type)) < 0) \
  return _DecrefModuleAndReturnNULL(m)
#define PY_ADD_CAPSULE_TO_MODULE_OR_RETURN_NULL(objname, ptr, capsuleName) \
PyObject *capsule = PyCapsule_New((void *)ptr, capsuleName, NULL); \
if (capsule == NULL) \
  return _DecrefModuleAndReturnNULL(m); \
if (PyModule_AddObject(m, Py_STRINGIFY(objname), capsule) < 0) { \
  Py_XDECREF(capsule); \
  return _DecrefModuleAndReturnNULL(m); \
}
#define PY_ADD_ALL_ATTRIBUTE(count, ...) \
/* Create and set __all__ list. */ \
PyObject* all_list = PyList_New(count); \
if (all_list == NULL) { \
  Py_XDECREF(m); \
  return NULL; \
} \
_SetAllListItems(all_list, count, __VA_ARGS__); \
/* Set __all__ in the module. */ \
if (PyModule_AddObject(m, "__all__", all_list) < 0) { \
  Py_XDECREF(all_list); \
  return _DecrefModuleAndReturnNULL(m); \
} \
return m

#define PyType_CreateInstance(type, typeobj, args, kwargs) ((type *)_PyType_CreateInstance(typeobj, args, kwargs))
#define PyObject_GetCallableMethod(type, name) _PyObject_GetCallableMethod((PyObject*)type, name)
