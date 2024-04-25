/* *INDENT-OFF* */
// This code follows PEP 7 and CPython ABI conventions
#include <Python.h>
#include "pyerrors.h"
#include <stdarg.h>
#include <stdbool.h>
#include <awaitable.h>
/*
 * To avoid any possible include conflicts if other
 * headers have the same file name.
 */
#include "../include/defines.h"

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

// forward declaration
static PyTypeObject _AwaitableGenWrapperType;

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

typedef struct {
    PyObject *coro;
    awaitcallback callback;
    awaitcallback_err err_callback;
    bool done;
} awaitable_callback;

struct _AwaitableObject {
    PyObject_HEAD
    awaitable_callback **aw_callbacks;
    Py_ssize_t aw_callback_size;
    PyObject *aw_result;
    PyObject *aw_gen;
    PyObject **aw_values;
    Py_ssize_t aw_values_size;
    void **aw_arb_values;
    Py_ssize_t aw_arb_values_size;
    Py_ssize_t aw_state;
    bool aw_done;
};

typedef struct {
    PyObject_HEAD
    PyObject *gw_result;
    AwaitableObject *gw_aw;
    PyObject *gw_current_await;
} GenWrapperObject;

PyDoc_STRVAR(awaitable_doc,
    "Awaitable transport utility for the C API.");

static PyObject *
awaitable_new_func(PyTypeObject *tp, PyObject *args, PyObject *kwds)
{
    assert(tp != NULL);
    assert(tp->tp_alloc != NULL);

    PyObject *self = tp->tp_alloc(tp, 0);
    if (self == NULL) {
        return NULL;
    }

    AwaitableObject *aw = (AwaitableObject *) self;
    aw->aw_callbacks = NULL;
    aw->aw_callback_size = 0;
    aw->aw_result = NULL;
    aw->aw_gen = NULL;
    aw->aw_values = NULL;
    aw->aw_values_size = 0;
    aw->aw_state = 0;
    aw->aw_done = false;

    return (PyObject *) aw;
}


static PyObject *
gen_new(PyTypeObject *tp, PyObject *args, PyObject *kwds)
{
    assert(tp != NULL);
    assert(tp->tp_alloc != NULL);

    PyObject *self = tp->tp_alloc(tp, 0);
    if (self == NULL) {
        return NULL;
    }

    GenWrapperObject *g = (GenWrapperObject *) self;
    g->gw_result = NULL;
    g->gw_aw = NULL;
    g->gw_current_await = NULL;

    return (PyObject *) g;
}

static void
gen_dealloc(PyObject *self)
{
    GenWrapperObject *g = (GenWrapperObject *) self;
    Py_XDECREF(g->gw_current_await);
    Py_XDECREF(g->gw_aw);
    Py_XDECREF(g->gw_result);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
_awaitable_genwrapper_new(AwaitableObject *aw)
{
    assert(aw != NULL);
    GenWrapperObject *g = (GenWrapperObject *) gen_new(
        &_AwaitableGenWrapperType,
        NULL,
        NULL
    );

    if (g == NULL) return NULL;
    g->gw_aw = (AwaitableObject *) Py_NewRef((PyObject *) aw);
    return (PyObject *) g;
}

static void
_awaitable_genwrapper_set_result(PyObject *gen, PyObject *result)
{
    assert(gen != NULL);
    assert(result != NULL);
    Py_INCREF(gen);
    GenWrapperObject *g = (GenWrapperObject *) gen;

    g->gw_result = Py_NewRef(result);
    Py_DECREF(gen);
}

static int
fire_err_callback(PyObject *self, PyObject *await, awaitable_callback *cb)
{
    assert(PyErr_Occurred() != NULL);
    if (!cb->err_callback) {
        cb->done = true;
        Py_DECREF(cb->coro);
        Py_XDECREF(await);
        return -1;
    }

    Py_INCREF(self);
    PyObject *err = PyErr_GetRaisedException();

    int e_res = cb->err_callback(self, err);
    cb->done = true;

    Py_DECREF(self);

    if (e_res < 0) {
        if (PyErr_Occurred())
            PyErr_Print();

        if (e_res == -1)
            PyErr_SetRaisedException(err);

        // if the res is -1, the error is restored. otherwise, it is not
        Py_DECREF(err);
        Py_DECREF(cb->coro);
        Py_XDECREF(await);
        return -1;
    };

    Py_DECREF(err);

    return 0;
}

static PyObject *
gen_next(PyObject *self)
{
    GenWrapperObject *g = (GenWrapperObject *) self;
    AwaitableObject *aw = g->gw_aw;
    awaitable_callback *cb;
    if (((aw->aw_state + 1) > aw->aw_callback_size) &&
        g->gw_current_await == NULL) {
        PyErr_SetObject(PyExc_StopIteration,
                        g->gw_result ?
                        g->gw_result :
                        Py_None);
        return NULL;
    }

    if (g->gw_current_await == NULL) {
        cb = aw->aw_callbacks[aw->aw_state++];

        if (Py_TYPE(cb->coro)->tp_as_async == NULL ||
            Py_TYPE(cb->coro)->tp_as_async->am_await == NULL) {
            PyErr_Format(PyExc_TypeError, "%R has no __await__", cb->coro);
            return NULL;
        }

        g->gw_current_await = Py_TYPE(cb->coro)->tp_as_async->am_await(
                                                            cb->coro);
        if (g->gw_current_await == NULL) {
            if (fire_err_callback((PyObject *) aw, g->gw_current_await, cb) < 0) {
                return NULL;
            }

            return gen_next(self);
        }
    } else {
        cb = aw->aw_callbacks[aw->aw_state - 1];
    }

    PyObject *result = Py_TYPE(g->gw_current_await
                        )->tp_iternext(g->gw_current_await);

    if (result == NULL) {
        PyObject *occurred = PyErr_Occurred();
        if (!occurred) {
            // coro is done
            g->gw_current_await = NULL;
            return gen_next(self);
        }

        if (!PyErr_GivenExceptionMatches(occurred, PyExc_StopIteration)) {
            if (fire_err_callback((PyObject *) aw, g->gw_current_await, cb) < 0) {
                return NULL;
            }
            g->gw_current_await = NULL;
            return gen_next(self);
        }

        if (cb->callback == NULL) {
            // coro is done, but with a result
            // we can disregard the result if theres no callback
            g->gw_current_await = NULL;
            PyErr_Clear();
            return gen_next(self);
        }

        PyObject *type, *value, *traceback;
        PyErr_Fetch(&type, &value, &traceback);
        PyErr_NormalizeException(&type, &value, &traceback);

        if (value == NULL) {
            value = Py_NewRef(Py_None);
        } else {
            Py_INCREF(value);
            PyObject* tmp = PyObject_GetAttrString(value, "value");
            if (tmp == NULL) {
                Py_DECREF(value);
                return NULL;
            }
            Py_DECREF(value);
            value = tmp;
            Py_INCREF(value);
        }

        Py_INCREF(aw);
        int result = cb->callback((PyObject *) aw, value);
        if (result < -1) {
            // -2 or lower denotes that the error should be deferred
            // regardless of whether a handler is present
            return NULL;
        }

        if (result < 0) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_SystemError, "callback returned -1 without exception set");
                return NULL;
            }
            if (fire_err_callback((PyObject *) aw, g->gw_current_await, cb) < 0) {
                PyErr_Restore(type, value, traceback);
                return NULL;
            }
        }

        cb->done = true;
        g->gw_current_await = NULL;
        return gen_next(self);
    }

    return result;
}


static PyObject *
awaitable_next(PyObject *self)
{
    AwaitableObject *aw = (AwaitableObject *) self;


    if (aw->aw_done) {
        PyErr_SetString(PyExc_RuntimeError, "cannot reuse awaitable");
        return NULL;
    }

    PyObject* gen = _awaitable_genwrapper_new(aw);

    if (gen == NULL) {
        return NULL;
    }

    aw->aw_gen = Py_NewRef(gen);
    aw->aw_done = true;
    return gen;
}

static void
awaitable_dealloc(PyObject *self)
{
    AwaitableObject *aw = (AwaitableObject *) self;
    if (aw->aw_values) {
        for (int i = 0; i < aw->aw_values_size; i++)
            Py_DECREF(aw->aw_values[i]);
        PyMem_Free(aw->aw_values);
    }

    Py_XDECREF(aw->aw_gen);
    Py_XDECREF(aw->aw_result);

    for (int i = 0; i < aw->aw_callback_size; i++) {
        awaitable_callback *cb = aw->aw_callbacks[i];
        if (!cb->done) Py_DECREF(cb->coro);
        PyMem_Free(cb);
    }

    if (aw->aw_arb_values) PyMem_Free(aw->aw_arb_values);
    Py_TYPE(self)->tp_free(self);
}

void
_awaitable_cancel(PyObject *aw)
{
    assert(aw != NULL);
    Py_INCREF(aw);

    AwaitableObject *a = (AwaitableObject *) aw;

    for (int i = 0; i < a->aw_callback_size; i++) {
        awaitable_callback* cb = a->aw_callbacks[i];
        if (!cb->done)
            Py_DECREF(cb->coro);
    }

    PyMem_Free(a->aw_callbacks);
    a->aw_callback_size = 0;
    Py_DECREF(aw);
}

static PyObject *
awaitable_send_with_arg(PyObject *self, PyObject *value)
{
    AwaitableObject *aw = (AwaitableObject *) self;
    if (aw->aw_gen == NULL) {
        PyObject* gen = awaitable_next(self);
        if (gen == NULL)
            return NULL;

        Py_RETURN_NONE;
    }

    return gen_next(aw->aw_gen);
}

static PyObject *
awaitable_send(PyObject *self, PyObject *args)
{
    PyObject *value;

    if (!PyArg_ParseTuple(args, "O", &value))
        return NULL;

    return awaitable_send_with_arg(self, value);
}

static PyObject *
awaitable_close(PyObject *self, PyObject *args)
{
    _awaitable_cancel(self);
    AwaitableObject *aw = (AwaitableObject *) self;
    aw->aw_done = true;
    Py_RETURN_NONE;
}

static PyObject *
awaitable_throw(PyObject *self, PyObject *args)
{
    PyObject *type;
    PyObject *value = NULL;
    PyObject *traceback = NULL;

    if (!PyArg_ParseTuple(args, "O|OO", &type, &value, &traceback))
        return NULL;

    if (PyType_Check(type)) {
        PyObject *err = PyObject_Vectorcall(type, (PyObject*[]) { value }, 1, NULL);
        if (err == NULL) {
            return NULL;
        }

        if (traceback)
            if (PyException_SetTraceback(err, traceback) < 0)
            {
                Py_DECREF(err);
                return NULL;
            }

        PyErr_Restore(err, NULL, NULL);
    } else
        PyErr_Restore(Py_NewRef(type), Py_XNewRef(value), Py_XNewRef(traceback));

    AwaitableObject *aw = (AwaitableObject *) self;
    if ((aw->aw_gen != NULL) && (aw->aw_state != 0)) {
        GenWrapperObject *gw = (GenWrapperObject *) aw->aw_gen;
        awaitable_callback* cb = aw->aw_callbacks[aw->aw_state - 1];
        if (cb == NULL)
            return NULL;

        if (fire_err_callback(self, gw->gw_current_await, cb) < 0)
            return NULL;
    } else
        return NULL;

    assert(NULL);
}

#if PY_MINOR_VERSION > 9
static PySendResult
awaitable_am_send(PyObject *self, PyObject *arg, PyObject **presult) {
    PyObject *send_res = awaitable_send_with_arg(self, arg);
    if (send_res == NULL) {
        PyObject *occurred = PyErr_Occurred();
        if (PyErr_GivenExceptionMatches(occurred, PyExc_StopIteration)) {
            PyObject *item = PyObject_GetAttrString(occurred, "value");

            if (item == NULL) {
                return PYGEN_ERROR;
            }

            *presult = item;
            return PYGEN_RETURN;
        }
        *presult = NULL;
        return PYGEN_ERROR;
    }
    AwaitableObject *aw = (AwaitableObject *) self;
    *presult = send_res;

    return PYGEN_NEXT;
}
#endif

static PyMethodDef awaitable_methods[] = {
    {"send", awaitable_send, METH_VARARGS, NULL},
    {"close", awaitable_close, METH_VARARGS, NULL},
    {"throw", awaitable_throw, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static PyAsyncMethods async_methods = {
    #if PY_MINOR_VERSION > 9
    .am_await = awaitable_next,
    .am_send = awaitable_am_send
    #else
    .am_await = awaitable_next
    #endif
};

static PyTypeObject _AwaitableGenWrapperType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_genwrapper", 
    .tp_basicsize = sizeof(GenWrapperObject),
    .tp_dealloc = gen_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = gen_next,
    .tp_new = gen_new,
};

static PyTypeObject _AwaitableType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_awaitable",
    .tp_basicsize = sizeof(AwaitableObject),
    .tp_dealloc = awaitable_dealloc,
    .tp_as_async = &async_methods,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = awaitable_doc,
    .tp_iternext = awaitable_next,
    .tp_new = awaitable_new_func,
    .tp_methods = awaitable_methods
};


int
_awaitable_await(
    PyObject *aw,
    PyObject *coro,
    awaitcallback cb,
    awaitcallback_err err
)
{
    assert(aw != NULL);
    assert(coro != NULL);
    Py_INCREF(coro);
    Py_INCREF(aw);
    AwaitableObject *a = (AwaitableObject *) aw;

    awaitable_callback *aw_c = PyMem_Malloc(sizeof(awaitable_callback));
    if (aw_c == NULL) {
        Py_DECREF(aw);
        Py_DECREF(coro);
        PyErr_NoMemory();
        return -1;
    }

    ++a->aw_callback_size;
    if (a->aw_callbacks == NULL) {
        a->aw_callbacks = PyMem_Calloc(a->aw_callback_size,
        sizeof(awaitable_callback *));
    } else {
        a->aw_callbacks = PyMem_Realloc(a->aw_callbacks,
        sizeof(awaitable_callback *) * a->aw_callback_size
    );
    }

    if (a->aw_callbacks == NULL) {
        --a->aw_callback_size;
        Py_DECREF(aw);
        Py_DECREF(coro);
        PyMem_Free(aw_c);
        PyErr_NoMemory();
        return -1;
    }

    aw_c->coro = coro; // steal our own reference
    aw_c->callback = cb;
    aw_c->err_callback = err;
    a->aw_callbacks[a->aw_callback_size - 1] = aw_c;
    Py_DECREF(aw);

    return 0;
}

int
_awaitable_set_result(PyObject *awaitable, PyObject *result)
{
    assert(awaitable != NULL);
    assert(result != NULL);
    Py_INCREF(result);
    Py_INCREF(awaitable);

    AwaitableObject *aw = (AwaitableObject *) awaitable;
    if (aw->aw_gen == NULL) {
        PyErr_SetString(PyExc_TypeError, "no generator is currently present");
        Py_DECREF(awaitable);
        Py_DECREF(result);
        return -1;
    }
    _awaitable_genwrapper_set_result(aw->aw_gen, result);
    Py_DECREF(awaitable);
    Py_DECREF(result);
    return 0;
}

int
_awaitable_unpack(PyObject *awaitable, ...)
{
    assert(awaitable != NULL);
    AwaitableObject *aw = (AwaitableObject *) awaitable;
    Py_INCREF(awaitable);

    if (aw->aw_values == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "awaitable object has no stored values");
        Py_DECREF(awaitable);
        return -1;
    }

    va_list args;
    va_start(args, awaitable);

    for (int i = 0; i < aw->aw_values_size; i++) {
        PyObject **ptr = va_arg(args, PyObject **);
        if (ptr == NULL) continue;
        *ptr = aw->aw_values[i];
        // borrowed reference
    }

    va_end(args);
    Py_DECREF(awaitable);
    return 0;
}

int
_awaitable_save(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    assert(awaitable != NULL);
    assert(nargs != 0);
    Py_INCREF(awaitable);
    AwaitableObject *aw = (AwaitableObject *) awaitable;

    va_list vargs;
    va_start(vargs, nargs);
    int offset = aw->aw_values_size;

    if (aw->aw_values == NULL)
        aw->aw_values = PyMem_Calloc(
            nargs,
            sizeof(PyObject *)
        );
    else
        aw->aw_values = PyMem_Realloc(
            aw->aw_values,
            sizeof(PyObject *) * (aw->aw_values_size + nargs)
        );

    if (aw->aw_values == NULL) {
        PyErr_NoMemory();
        Py_DECREF(awaitable);
        return -1;
    }

    aw->aw_values_size += nargs;

    for (int i = offset; i < aw->aw_values_size; i++)
        aw->aw_values[i] = Py_NewRef(va_arg(vargs, PyObject*));

    va_end(vargs);
    Py_DECREF(awaitable);
    return 0;
}

int
_awaitable_unpack_arb(PyObject *awaitable, ...)
{
    assert(awaitable != NULL);
    AwaitableObject *aw = (AwaitableObject *) awaitable;
    Py_INCREF(awaitable);

    if (aw->aw_values == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "awaitable object has no stored arbitrary values");
        Py_DECREF(awaitable);
        return -1;
    }

    va_list args;
    va_start(args, awaitable);

    for (int i = 0; i < aw->aw_arb_values_size; i++) {
        void **ptr = va_arg(args, void **);
        if (ptr == NULL) continue;
        *ptr = aw->aw_arb_values[i];
    }

    va_end(args);
    Py_DECREF(awaitable);
    return 0;
}

int
_awaitable_save_arb(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    assert(awaitable != NULL);
    assert(nargs != 0);
    Py_INCREF(awaitable);
    AwaitableObject *aw = (AwaitableObject *) awaitable;

    va_list vargs;
    va_start(vargs, nargs);
    int offset = aw->aw_arb_values_size;

    if (aw->aw_arb_values == NULL)
        aw->aw_arb_values = PyMem_Calloc(
            nargs,
            sizeof(void *)
        );
    else
        aw->aw_arb_values = PyMem_Realloc(
            aw->aw_arb_values,
            sizeof(void *) * (aw->aw_arb_values_size + nargs)
        );

    if (aw->aw_arb_values == NULL) {
        PyErr_NoMemory();
        Py_DECREF(awaitable);
        return -1;
    }

    aw->aw_arb_values_size += nargs;

    for (int i = offset; i < aw->aw_arb_values_size; i++)
        aw->aw_arb_values[i] = va_arg(vargs, void *);

    va_end(vargs);
    Py_DECREF(awaitable);
    return 0;
}

PyObject *
_awaitable_new(void)
{
    PyObject *aw = awaitable_new_func(&_AwaitableType, NULL, NULL);
    return aw;
}

static PyModuleDef awaitable_module = {
    PyModuleDef_HEAD_INIT,
    "_pyawaitable",
    NULL,
    -1
};

PyMODINIT_FUNC PyInit_pyawaitable()
{
    PY_TYPE_IS_READY_OR_RETURN_NULL(_AwaitableType);
    PY_TYPE_IS_READY_OR_RETURN_NULL(_AwaitableGenWrapperType);
    PY_CREATE_MODULE(awaitable_module);
    PY_TYPE_ADD_TO_MODULE_OR_RETURN_NULL(_awaitable, _AwaitableType);
    PY_TYPE_ADD_TO_MODULE_OR_RETURN_NULL(_genwrapper, _AwaitableGenWrapperType);

    awaitable_api[0] = &_AwaitableType;
    awaitable_api[1] = &_AwaitableGenWrapperType;
    awaitable_api[2] = _awaitable_new;
    awaitable_api[3] = _awaitable_await;
    awaitable_api[4] = _awaitable_cancel;
    awaitable_api[5] = _awaitable_set_result;
    awaitable_api[6] = _awaitable_save;
    awaitable_api[7] = _awaitable_save_arb;
    awaitable_api[8] = _awaitable_unpack;
    awaitable_api[9] = _awaitable_unpack_arb;
    PY_ADD_CAPSULE_TO_MODULE_OR_RETURN_NULL(_api, awaitable_api, NULL);
    return m;
}
