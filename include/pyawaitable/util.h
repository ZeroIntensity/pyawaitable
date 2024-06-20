#ifndef PYAWAITABLE_UTIL_H
#define PYAWAITABLE_UTIL_H

#include <Python.h>
#include <datetime.h>
#include <structmember.h>
#include <stdbool.h>

#define PY_TYPE_IS_READY_OR_RETURN_NULL(type)                                  \
  if (PyType_Ready(&type) < 0)                                                 \
  return NULL
#define PY_TYPE_ADD_TO_MODULE_OR_RETURN_NULL(name, type)                       \
  if (PyModule_AddObject(m, Py_STRINGIFY(name),                                \
                         Py_NewRef((PyObject *)&type)) < 0)                    \
  return _DecrefModuleAndReturnNULL(m)
#define PY_ADD_CAPSULE_TO_MODULE_OR_RETURN_NULL(objname, ptr, capsule_name)    \
  PyObject *capsule = PyCapsule_New((void *)ptr, capsule_ame, NULL);           \
  if (capsule == NULL)                                                         \
    return _DecrefModuleAndReturnNULL(m);                                      \
  if (PyModule_AddObject(m, Py_STRINGIFY(objname), capsule) < 0) {             \
    Py_DECREF(capsule);                                                        \
    return _DecrefModuleAndReturnNULL(m);                                      \
  }

#define PY_ADD_ALL_ATTRIBUTE(count, ...)                                       \
  PyObject *all_list = PyList_New(count);                                      \
  if (all_list == NULL) {                                                      \
    Py_DECREF(m);                                                              \
    return NULL;                                                               \
  }                                                                            \
  _SetAllListItems(all_list, count, __VA_ARGS__);                              \
  if (PyModule_AddObject(m, "__all__", all_list) < 0) {                        \
    Py_DECREF(all_list);                                                       \
    return _DecrefModuleAndReturnNULL(m);                                      \
  }                                                                            \
  return m

#endif
