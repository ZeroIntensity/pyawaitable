#include <pyawaitable/dist.h>
#include <pyawaitable/init.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>

static int
dict_add_type(PyObject *state, PyTypeObject *obj)
{
    assert(obj != NULL);
    assert(state != NULL);
    assert(PyDict_Check(state));
    assert(obj->tp_name != NULL);

    Py_INCREF(obj);
    if (PyType_Ready(obj) < 0) {
        Py_DECREF(obj);
        return -1;
    }

    if (PyDict_SetItemString(state, obj->tp_name, (PyObject *)obj) < 0) {
        Py_DECREF(obj);
        return -1;
    }
    Py_DECREF(obj);
    return 0;
}

static int
init_state(PyObject *state)
{
    assert(state != NULL);
    assert(PyDict_Check(state));
    if (dict_add_type(state, &PyAwaitable_Type) < 0) {
        return -1;
    }

    if (dict_add_type(state, &_PyAwaitableGenWrapperType) < 0) {
        return -1;
    }

    PyObject *version = PyLong_FromLong(PyAwaitable_MAGIC_NUMBER);
    if (version == NULL) {
        return -1;
    }

    if (PyDict_SetItemString(state, "magic_version", version) < 0) {
        Py_DECREF(version);
        return -1;
    }

    Py_DECREF(version);
    return 0;
}

static PyObject *
create_state(void)
{
    PyObject *state = PyDict_New();
    if (state == NULL) {
        return NULL;
    }

    if (init_state(state) < 0) {
        Py_DECREF(state);
        return NULL;
    }

    return state;
}

static PyObject *
interp_get_dict(void)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    assert(interp != NULL);
    PyObject *interp_state = PyInterpreterState_GetDict(interp);
    if (interp_state == NULL) {
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
state_corrupted(const char *err, PyObject *found)
{
    assert(err != NULL);
    assert(found != NULL);
    PyErr_Format(
        PyExc_SystemError,
        "PyAwaitable corruption! %s: %R",
        err,
        found
    );
    Py_DECREF(found);
    return -1;
}

static PyObject *
get_state_value(PyObject *state, const char *name)
{
    assert(name != NULL);
    PyObject *str = PyUnicode_FromString(name);
    if (str == NULL) {
        return NULL;
    }

    PyObject *version = PyDict_GetItemWithError(state, str);
    Py_DECREF(str);
    return version;
}

static long
get_state_version(PyObject *state)
{
    assert(state != NULL);
    assert(PyDict_Check(state));

    PyObject *version = get_state_value(state, "magic_version");
    if (version == NULL) {
        return -1;
    }

    if (!PyLong_CheckExact(version)) {
        return state_corrupted("Non-int version number", version);
    }

    long version_num = PyLong_AsLong(version);
    if (version_num == -1 && PyErr_Occurred()) {
        Py_DECREF(version);
        return -1;
    }

    if (version_num < 0) {
        return state_corrupted("Found <0 version somehow", version);
    }

    return version_num;
}

static PyObject *
find_module_for_version(PyObject *interp_dict, long version)
{
    PyObject *list = PyDict_GetItemString(interp_dict, "_pyawaitable_states");
    if (list == NULL) {
        return not_initialized();
    }

    if (!PyList_CheckExact(list)) {
        state_corrupted("_pyawaitable_states is not a list", list);
        return NULL;
    }

    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(list); ++i) {
        PyObject *mod = PyList_GET_ITEM(list, i);
        long got_version = get_state_version(mod);

        if (got_version == -1) {
            return NULL;
        }

        if (got_version == version) {
            return mod;
        }
    }

    return not_initialized();
}

static PyObject *
find_top_level_state(PyObject **interp_dict)
{
    PyObject *dict = interp_get_dict();
    if (dict == NULL) {
        return NULL;
    }

    PyObject *mod = PyDict_GetItemString(dict, "_pyawaitable_state");
    if (mod == NULL) {
        return not_initialized();
    }

    if (interp_dict != NULL) {
        *interp_dict = dict;
    }
    return mod;
}

static PyAwaitable_thread_local PyObject *pyawaitable_fast_state = NULL;

_PyAwaitable_INTERNAL(PyObject *)
_PyAwaitable_GetState(void)
{
    if (pyawaitable_fast_state != NULL) {
        return pyawaitable_fast_state;
    }

    PyObject *interp_dict;
    PyObject *state = find_top_level_state(&interp_dict); // Borrowed reference
    if (state == NULL) {
        return NULL;
    }

    long version = get_state_version(state);
    if (version == -1) {
        return NULL;
    }

    if (version != PyAwaitable_MAGIC_NUMBER) {
        // Not our module!
        state = find_module_for_version(interp_dict, PyAwaitable_MAGIC_NUMBER);
        if (state == NULL) {
            return NULL;
        }
    }

    // We want this to be a borrowed reference
    pyawaitable_fast_state = state;
    return state;
}

static PyAwaitable_thread_local PyTypeObject *pyawaitable_fast_aw = NULL;
static PyAwaitable_thread_local PyTypeObject *pyawaitable_fast_gw = NULL;

_PyAwaitable_API(PyTypeObject *)
PyAwaitable_GetType(void)
{
    if (pyawaitable_fast_aw != NULL) {
        return pyawaitable_fast_aw;
    }
    PyObject *state = _PyAwaitable_GetState();
    if (state == NULL) {
        return NULL;
    }

    PyTypeObject *pyawaitable_type = (PyTypeObject *)get_state_value(
        state,
        "_PyAwaitableType"
    );
    if (pyawaitable_type == NULL) {
        return NULL;
    }

    // Should be an immortal reference
    pyawaitable_fast_aw = pyawaitable_type;
    return pyawaitable_type;
}


_PyAwaitable_INTERNAL(PyTypeObject *)
_PyAwaitable_GetGenWrapperType(void)
{
    if (pyawaitable_fast_gw != NULL) {
        return pyawaitable_fast_gw;
    }
    PyObject *state = _PyAwaitable_GetState();
    if (state == NULL) {
        return NULL;
    }

    PyTypeObject *gw_type = (PyTypeObject *)get_state_value(
        state,
        "_PyAwaitableGenWrapperType"
    );
    if (gw_type == NULL) {
        return NULL;
    }

    pyawaitable_fast_gw = gw_type;
    return (PyTypeObject *)gw_type;
}

static int
add_state_to_list(PyObject *interp_dict, PyObject *state)
{
    assert(interp_dict != NULL);
    assert(state != NULL);
    assert(PyDict_Check(interp_dict));
    assert(PyDict_Check(state));

    PyObject *pyawaitable_list = Py_XNewRef(
        PyDict_GetItemString(
            interp_dict,
            "_pyawaitable_states"
        )
    );
    if (pyawaitable_list == NULL) {
        // No list has been set
        pyawaitable_list = PyList_New(1);
        if (pyawaitable_list == NULL) {
            // Memory error
            return -1;
        }

        if (
            PyDict_SetItemString(
                interp_dict,
                "_pyawaitable_states",
                pyawaitable_list
            ) < 0
        ) {
            Py_DECREF(pyawaitable_list);
            return -1;
        }
    }

    if (PyList_Append(pyawaitable_list, state) < 0) {
        Py_DECREF(pyawaitable_list);
        return -1;
    }

    Py_DECREF(pyawaitable_list);
    return 0;
}

_PyAwaitable_API(int)
PyAwaitable_Init(void)
{
    PyObject *dict = interp_get_dict();
    if (dict == NULL) {
        return -1;
    }

    PyObject *state = create_state();
    if (state == NULL) {
        return -1;
    }

    PyObject *existing = PyDict_GetItemString(dict, "_pyawaitable_state");
    if (existing != NULL) {
        /* Oh no, PyAwaitable has been used twice! */
        long version = get_state_version(existing);
        if (version == -1) {
            Py_DECREF(state);
            return -1;
        }

        if (version == PyAwaitable_MAGIC_NUMBER) {
            // Yay! It just happens that it's the same version as us.
            // Let's just reuse it.
            Py_DECREF(state);
            return 0;
        }

        if (add_state_to_list(dict, state) < 0) {
            Py_DECREF(state);
            return -1;
        }

        Py_DECREF(state);
        return 0;
    }

    if (PyDict_SetItemString(dict, "_pyawaitable_state", state) < 0) {
        Py_DECREF(state);
        return -1;
    }

    Py_DECREF(state);
    return 0;
}
