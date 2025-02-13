#include <pyawaitable/dist.h>
#include <pyawaitable/init.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>

static int
pyawaitable_exec(PyObject *mod)
{
    if (PyModule_AddType(mod, &PyAwaitable_Type) < 0)
    {
        return -1;
    }

    if (PyModule_AddType(mod, &_PyAwaitableGenWrapperType) < 0)
    {
        return -1;
    }

    if (
        PyModule_AddIntConstant(
            mod,
            "magic_version",
            PyAwaitable_MAGIC_NUMBER
        ) < 0
    )
    {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot pyawaitable_slots[] =
{
    {Py_mod_exec, pyawaitable_exec},
    {0, NULL}
};

static struct PyModuleDef pyawaitable_module =
{
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_pyawaitable",
    .m_size = 0,
    .m_slots = pyawaitable_slots
};

static PyObject *
interp_get_dict(void)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    assert(interp != NULL);
    PyObject *interp_state = PyInterpreterState_GetDict(interp);
    if (interp_state == NULL)
    {
        // Would be a memory error or something more exotic, but
        // there's nothing we can do.
        PyErr_SetString(
            PyExc_RuntimeError,
            "PyAwaitable: Interpreter failed to provide a state dictionary"
        );
        return NULL;
    }

    return interp_state;
}

static inline PyObject *
not_initialized(void)
{
    PyErr_SetString(
        PyExc_RuntimeError,
        "PyAwaitable hasn't been initialized yet! "
        "Did you forget to call PyAwaitable_Init()?"
    );
    return NULL;
}

static inline int
module_corrupted(const char *err, PyObject *found)
{
    PyErr_Format(
        PyExc_SystemError,
        "PyAwaitable module corruption! %s: %R",
        err,
        found
    );
    return -1;
}

static long
get_module_version(PyObject *mod)
{
    PyObject *mod_name = PyModule_GetNameObject(mod);
    if (mod_name == NULL)
    {
        assert(PyErr_Occurred());
        return -1;
    }

    if (PyUnicode_CompareWithASCIIString(mod_name, "_pyawaitable") != 0)
    {
        return module_corrupted("Invalid module name", mod_name);
    }

    PyObject *version = PyObject_GetAttrString(mod, "magic_version");
    if (version == NULL)
    {
        return -1;
    }

    if (!PyLong_CheckExact(version))
    {
        return module_corrupted("Non-int version number", version);
    }

    long version_num = PyLong_AsLong(version);
    if (version_num == -1 && PyErr_Occurred())
    {
        return -1;
    }

    if (version_num < 0)
    {
        return module_corrupted("Found <0 version somehow", version);
    }

    return version_num;
}

static PyObject *
find_module_for_version(PyObject *interp_dict, long version)
{
    PyObject *list = PyDict_GetItemString(interp_dict, "_pyawaitable_list");
    if (list == NULL)
    {
        return not_initialized();
    }

    if (!PyList_CheckExact(list))
    {
        module_corrupted("_pyawaitable_list is not a list", list);
        return NULL;
    }

    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(list); ++i)
    {
        PyObject *mod = PyList_GET_ITEM(list, i);
        long got_version = get_module_version(mod);

        if (got_version == -1)
        {
            return NULL;
        }

        if (got_version == version)
        {
            return mod;
        }
    }

    return not_initialized();
}

static PyAwaitable_thread_local PyObject *pyawaitable_fast_module = NULL;

_PyAwaitable_INTERNAL(PyObject *)
_PyAwaitable_GetModule(void)
{
    if (pyawaitable_fast_module != NULL)
    {
        return pyawaitable_fast_module;
    }

    PyObject *dict = interp_get_dict();
    if (dict == NULL)
    {
        return NULL;
    }

    PyObject *mod = PyDict_GetItemString(dict, "_pyawaitable_mod");
    if (mod == NULL)
    {
        return not_initialized();
    }

    long version = get_module_version(mod);
    if (version != PyAwaitable_MAGIC_NUMBER)
    {
        // Not our module!
        mod = find_module_for_version(dict, PyAwaitable_MAGIC_NUMBER);
        if (mod == NULL)
        {
            return NULL;
        }
    }

    // We want this to be a borrowed reference
    pyawaitable_fast_module = mod;
    return mod;
}

static PyAwaitable_thread_local PyTypeObject *pyawaitable_fast_aw = NULL;
static PyAwaitable_thread_local PyTypeObject *pyawaitable_fast_gw = NULL;

_PyAwaitable_INTERNAL(PyTypeObject *)
_PyAwaitable_GetAwaitableType(void)
{
    if (pyawaitable_fast_aw != NULL)
    {
        return pyawaitable_fast_aw;
    }
    PyObject *mod = _PyAwaitable_GetModule();
    if (mod == NULL)
    {
        return NULL;
    }

    PyTypeObject *pyawaitable_type = (PyTypeObject *)PyObject_GetAttrString(
        mod,
        "_PyAwaitableType"
    );
    if (pyawaitable_type == NULL)
    {
        return NULL;
    }

    // Should be an immortal reference
    pyawaitable_fast_aw = pyawaitable_type;
    return pyawaitable_type;
}


_PyAwaitable_INTERNAL(PyTypeObject *)
_PyAwaitable_GetGenWrapperType(void)
{
    if (pyawaitable_fast_gw != NULL)
    {
        return pyawaitable_fast_gw;
    }
    PyObject *mod = _PyAwaitable_GetModule();
    if (mod == NULL)
    {
        return NULL;
    }

    PyTypeObject *gw_type = (PyTypeObject *)PyObject_GetAttrString(
        mod,
        "_PyAwaitableGenWrapperType"
    );
    if (gw_type == NULL)
    {
        return NULL;
    }

    pyawaitable_fast_gw = gw_type;
    return (PyTypeObject *)gw_type;
}


_PyAwaitable_API(int)
PyAwaitable_Init(void)
{
    PyObject *mod = PyModuleDef_Init(&pyawaitable_module);
    if (mod == NULL)
    {
        return -1;
    }

    PyObject *dict = interp_get_dict();
    if (dict == NULL)
    {
        Py_DECREF(mod);
        return -1;
    }

    PyObject *existing = PyDict_GetItemString(dict, "_pyawaitable_mod");
    if (existing != NULL)
    {
        /* Oh no, PyAwaitable has been used twice! */
        long version = get_module_version(existing);
        if (version == -1)
        {
            Py_DECREF(mod);
            return -1;
        }

        if (version == PyAwaitable_MAGIC_NUMBER)
        {
            // Yay! It just happens that it's the same version as us.
            // Let's just reuse it.
            Py_DECREF(mod);
            return 0;
        }

        PyObject *pyawaitable_list = PyDict_GetItemString(
            dict,
            "_pyawaitable_modules"
        );
        if (pyawaitable_list == NULL)
        {
            // No list has been set
            pyawaitable_list = PyList_New(1);
            if (pyawaitable_list == NULL)
            {
                // Memory error
                Py_DECREF(mod);
                return -1;
            }
        }

        if (PyList_Append(pyawaitable_list, mod) < 0)
        {
            Py_DECREF(mod);
            return -1;
        }

        Py_DECREF(mod);
        return 0;
    }

    if (PyDict_SetItemString(dict, "_pyawaitable_mod", mod) < 0)
    {
        Py_DECREF(mod);
        return -1;
    }

    Py_DECREF(mod);
    return 0;
}
