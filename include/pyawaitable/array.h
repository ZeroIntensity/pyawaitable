#ifndef PYAWAITABLE_ARRAY_H
#define PYAWAITABLE_ARRAY_H

#include <Python.h>
#include <stdlib.h>

#include <pyawaitable/dist.h>
#include <pyawaitable/optimize.h>

#define _pyawaitable_array_DEFAULT_SIZE 16

/*
 * Deallocator for items on a pyawaitable_array structure. A NULL pointer
 * will never be given to the deallocator.
 */
typedef void (*_PyAwaitable_MANGLE(pyawaitable_array_deallocator))(void *);

/*
 * Internal only dynamic array for PyAwaitable.
 */
typedef struct {
    /*
     * The actual items in the dynamic array.
     * Don't access this field publicly to get
     * items--use pyawaitable_array_GET_ITEM() instead.
     */
    void **items;
    /*
     * The length of the actual items array allocation.
     */
    Py_ssize_t capacity;
    /*
     * The number of items in the array.
     * Don't use this field publicly--use pyawaitable_array_LENGTH()
     */
    Py_ssize_t length;
    /*
     * The deallocator, set by one of the initializer functions.
     * This may be NULL.
     */
    pyawaitable_array_deallocator deallocator;
} _PyAwaitable_MANGLE(pyawaitable_array);


/* Zero out the array */
static inline void
pyawaitable_array_ZERO(pyawaitable_array *array)
{
    assert(array != NULL);
    array->deallocator = NULL;
    array->items = NULL;
    array->length = 0;
    array->capacity = 0;
}

static inline void
pyawaitable_array_ASSERT_VALID(pyawaitable_array *array)
{
    assert(array != NULL);
    assert(array->items != NULL);
}

static inline void
pyawaitable_array_ASSERT_INDEX(pyawaitable_array *array, Py_ssize_t index)
{
    // Ensure the index is valid
    assert(index < array->length);
    assert(index >= 0);
}

/*
 * Initialize a dynamic array with an initial size and deallocator.
 *
 * If the deallocator is NULL, then nothing happens to items upon
 * removal and upon array clearing.
 *
 * Returns -1 upon failure, 0 otherwise.
 */
_PyAwaitable_INTERNAL(int)
pyawaitable_array_init_with_size(
    pyawaitable_array * array,
    pyawaitable_array_deallocator deallocator,
    Py_ssize_t initial
);

/*
 * Append to the array.
 *
 * Returns -1 upon failure, 0 otherwise.
 * If this fails, the deallocator is not ran on the item.
 */
_PyAwaitable_INTERNAL(int)
pyawaitable_array_append(pyawaitable_array * array, void *item);

/*
 * Insert an item at the target index. The index
 * must currently be a valid index in the array.
 *
 * Returns -1 upon failure, 0 otherwise.
 * If this fails, the deallocator is not ran on the item.
 */
_PyAwaitable_INTERNAL(int)
pyawaitable_array_insert(
    pyawaitable_array * array,
    Py_ssize_t index,
    void *item
);

/* Remove all items from the array. */
_PyAwaitable_INTERNAL(void)
pyawaitable_array_clear_items(pyawaitable_array * array);

/*
 * Clear all the fields on the array.
 *
 * Note that this does *not* free the actual dynamic array
 * structure--use pyawaitable_array_free() for that.
 *
 * It's safe to call pyawaitable_array_init() or init_with_size() again
 * on the array after calling this.
 */
_PyAwaitable_INTERNAL(void)
pyawaitable_array_clear(pyawaitable_array * array);

/*
 * Set a value at index in the array.
 *
 * If an item already exists at the target index, the deallocator
 * is called on it, if the array has one set.
 *
 * This cannot fail.
 */
_PyAwaitable_INTERNAL(void)
pyawaitable_array_set(pyawaitable_array * array, Py_ssize_t index, void *item);

/*
 * Remove the item at the index, and call the deallocator on it (if the array
 * has one set).
 *
 * This cannot fail.
 */
_PyAwaitable_INTERNAL(void)
pyawaitable_array_remove(pyawaitable_array * array, Py_ssize_t index);

/*
 * Remove the item at the index *without* deallocating it, and
 * return the item.
 *
 * This cannot fail.
 */
_PyAwaitable_INTERNAL(void *)
pyawaitable_array_pop(pyawaitable_array * array, Py_ssize_t index);

/*
 * Clear all the fields on a dynamic array, and then
 * free the dynamic array structure itself.
 *
 * The array must have been created by pyawaitable_array_new()
 */
static inline void
pyawaitable_array_free(pyawaitable_array *array)
{
    pyawaitable_array_ASSERT_VALID(array);
    pyawaitable_array_clear(array);
    PyMem_RawFree(array);
}

/*
 * Equivalent to pyawaitable_array_init_with_size() with a default size of 16.
 *
 * Returns -1 upon failure, 0 otherwise.
 */
static inline int
pyawaitable_array_init(
    pyawaitable_array *array,
    pyawaitable_array_deallocator deallocator
)
{
    return pyawaitable_array_init_with_size(
        array,
        deallocator,
        _pyawaitable_array_DEFAULT_SIZE
    );
}

/*
 * Allocate and create a new dynamic array on the heap.
 *
 * The returned pointer should be freed with pyawaitable_array_free()
 * If this function fails, it returns NULL.
 */
static inline pyawaitable_array *
pyawaitable_array_new_with_size(
    pyawaitable_array_deallocator deallocator,
    Py_ssize_t initial
)
{
    pyawaitable_array *array = PyMem_Malloc(sizeof(pyawaitable_array));
    if (PyAwaitable_UNLIKELY(array == NULL)) {
        return NULL;
    }

    if (pyawaitable_array_init_with_size(array, deallocator, initial) < 0) {
        PyMem_Free(array);
        return NULL;
    }

    pyawaitable_array_ASSERT_VALID(array); // Sanity check
    return array;
}

/*
 * Equivalent to pyawaitable_array_new_with_size() with a size of 16.
 *
 * The returned array must be freed with pyawaitable_array_free().
 * Returns NULL on failure.
 */
static inline pyawaitable_array *
pyawaitable_array_new(pyawaitable_array_deallocator deallocator)
{
    return pyawaitable_array_new_with_size(
        deallocator,
        _pyawaitable_array_DEFAULT_SIZE
    );
}

/*
 * Get an item from the array. This cannot fail.
 *
 * If the index is not valid, this is undefined behavior.
 */
static inline void *
pyawaitable_array_GET_ITEM(pyawaitable_array *array, Py_ssize_t index)
{
    pyawaitable_array_ASSERT_VALID(array);
    pyawaitable_array_ASSERT_INDEX(array, index);
    return array->items[index];
}

/*
 * Get the length of the array. This cannot fail.
 */
static inline Py_ssize_t PyAwaitable_PURE
pyawaitable_array_LENGTH(pyawaitable_array *array)
{
    pyawaitable_array_ASSERT_VALID(array);
    return array->length;
}

/*
 * Pop the item at the end the array.
 * This function cannot fail.
 */
static inline void *
pyawaitable_array_pop_top(pyawaitable_array *array)
{
    return pyawaitable_array_pop(array, pyawaitable_array_LENGTH(array) - 1);
}

#endif
