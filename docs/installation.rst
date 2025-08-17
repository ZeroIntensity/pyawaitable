Installation
============

PyAwaitable needs to be installed as a build dependency, not a runtime
dependency. For the ``project.dependencies`` portion of your ``pyproject.toml``
file, you can completely omit PyAwaitable as a dependency.

For example, in a ``setuptools``-based project, your ``build-system`` section would look like:

.. code-block:: toml

    # pyproject.toml
    [build-system]
    requires = ["setuptools~=78.1", "pyawaitable>=2.0.0"]
    build-backend = "setuptools.build_meta"


Including the ``pyawaitable.h`` File
------------------------------------

PyAwaitable provides a number of APIs to access the include path for a
number of different build systems. Namely:

-   :func:`pyawaitable.include` is available in Python code (typically for
    ``setup.py``-based extensions).
-   :envvar:`PYAWAITABLE_INCLUDE` is accessible as an environment variable,
    but only if Python has been started without :py:option:`-S` (this is
    useful for ``scikit-build-core`` projects).
-   ``pyawaitable --include`` on the command line returns the path of the
    include directory (useful for ``meson-python`` projects).

.. note::

    PyAwaitable uses a nifty trick for building itself into your project.
    Python's packaging ecosystem isn't exactly great at distributing C
    libraries, so the ``pyawaitable.h`` actually contains the entire
    PyAwaitable source code (but with mangled names to prevent collisions
    with your own project).

    This has some pros and cons:

    - PyAwaitable doesn't need to be installed at runtime, as it's embedded
      directly into your extension. This means it's extremely portable;
      completely different PyAwaitable versions can peacefully coexist in the
      same process.
    - Enabling debug flags in your extension also means enabling debug flags
      in PyAwaitable, thus enabling assertions and whatnot.
    - However, PyAwaitable can't use the limited API, so it prevents your
      extension from using the limited API (see the note below).

Initializing PyAwaitable in Your Extension
------------------------------------------

PyAwaitable has to do a one-time initialization to get its types and other
state initialized in the Python process. This is done with
:c:func:`PyAwaitable_Init`, which can be called basically anywhere,
as long as it's called before any other PyAwaitable functions are used.

Typically, you'll want to call this in your extension's `PyInit_` function, or
in the :c:data`Py_mod_exec` function in multi-phase extensions. For example:

.. code-block:: c

    // Single-phase
    PyMODINIT_FUNC
    PyInit_mymodule()
    {
        if (PyAwaitable_Init() < 0) {
            return NULL;
        }

        return PyModule_Create(/* ... */);
    }

.. code-block:: c

    // Multi-phase
    static int
    module_exec(PyObject *mod)
    {
        return PyAwaitable_Init();
    }

.. warning::

    Unfortunately, PyAwaitable cannot be used with the
    :ref:`limited C API <limited-c-api>`. This is due to PyAwaitable needing
    :c:member:`~PyAsyncMethods.am_send` to implement the coroutine protocol
    on 3.10+, but the corresponding :ref:`heap type <heap-types>` slot
    ``Py_am_send`` was not added until 3.11. Therefore, PyAwaitable 
    annot support the limited API without dropping support for <3.11.

Examples
--------

``setuptools``
**************

.. code-block:: python

    # setup.py
    from setuptools import setup, Extension
    import pyawaitable

    if __name__ == "__main__":
        setup(
            ext_modules=[
                Extension("_module", ["src/module.c"], include_dirs=[pyawaitable.include()])
            ]
        )

``scikit-build-core``
*********************

.. code-block:: cmake

    # CMakeLists.txt
    cmake_minimum_required(VERSION 3.15...3.30)
    project(${SKBUILD_PROJECT_NAME} LANGUAGES C)

    find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

    Python_add_library(_module MODULE src/module.c WITH_SOABI)
    target_include_directories(_module PRIVATE $ENV{PYAWAITABLE_INCLUDE})
    install(TARGETS _module DESTINATION .)

``meson-python``
****************

.. code-block:: meson

    # meson.build
    project('_module', 'c')

    py = import('python').find_installation(pure: false)
    pyawaitable_include = run_command('pyawaitable --include', check: true).stdout().strip()

    py.extension_module(
        '_module',
        'src/module.c',
        install: true,
        include_directories: [pyawaitable_include],
    )

Simple Extension Example
------------------------

.. code-block:: c

    #include <Python.h>
    #include <pyawaitable.h>

    static int
    module_exec(PyObject *mod)
    {
        return PyAwaitable_Init();
    }

    /*
    Equivalent to the following Python function:

    async def async_function(coro: collections.abc.Awaitable) -> None:
        await coro

    */
    static PyObject *
    async_function(PyObject *self, PyObject *coro)
    {
        PyObject *awaitable = PyAwaitable_New();
        if (awaitable == NULL) {
            return NULL;
        }

        if (PyAwaitable_AddAwait(awaitable, coro, NULL, NULL) < 0) {
            Py_DECREF(awaitable);
            return NULL;
        }

        return awaitable;
    }

    static PyModuleDef_Slot module_slots[] = {
        {Py_mod_exec, module_exec},
        {0, NULL}
    };

    static PyMethodDef module_methods[] = {
        {"async_function", async_function, METH_O, NULL},
        {NULL, NULL, 0, NULL},
    };

    static PyModuleDef module = {
        .m_base = PyModuleDef_HEAD_INIT,
        .m_size = 0,
        .m_slots = module_slots,
        .m_methods = module_methods
    };

    PyMODINIT_FUNC
    PyInit__module()
    {
        return PyModuleDef_Init(&module);
    }
