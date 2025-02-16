#include <pyawaitable/array.h>
#include <pyawaitable/optimize.h>

static inline void
call_deallocator_maybe(pyawaitable_array *array, Py_ssize_t index)
{
    if (array->deallocator != NULL && array->items[index] != NULL) {
        array->deallocator(array->items[index]);
        array->items[index] = NULL;
    }
}

_PyAwaitable_INTERNAL(int)
pyawaitable_array_init_with_size(
    pyawaitable_array * array,
    pyawaitable_array_deallocator deallocator,
    Py_ssize_t initial
)
{
    assert(array != NULL);
    assert(initial > 0);
    void **items = PyMem_Calloc(sizeof(void *), initial);
    if (PyAwaitable_UNLIKELY(items == NULL)) {
        return -1;
    }

    array->capacity = initial;
    array->items = items;
    array->length = 0;
    array->deallocator = deallocator;

    return 0;
}

static int
resize_if_needed(pyawaitable_array *array)
{
    if (array->length == array->capacity) {
        // Need to resize
        array->capacity *= 2;
        void **new_items = PyMem_Realloc(
            array->items,
            sizeof(void *) * array->capacity
        );
        if (PyAwaitable_UNLIKELY(new_items == NULL)) {
            return -1;
        }

        array->items = new_items;
    }

    return 0;
}

_PyAwaitable_INTERNAL(int) PyAwaitable_PURE
pyawaitable_array_append(pyawaitable_array *array, void *item)
{
    pyawaitable_array_ASSERT_VALID(array);
    array->items[array->length++] = item;
    if (resize_if_needed(array) < 0) {
        array->items[--array->length] = NULL;
        return -1;
    }
    return 0;
}

_PyAwaitable_INTERNAL(int)
pyawaitable_array_insert(
    pyawaitable_array * array,
    Py_ssize_t index,
    void *item
)
{
    pyawaitable_array_ASSERT_VALID(array);
    pyawaitable_array_ASSERT_INDEX(array, index);
    ++array->length;
    if (resize_if_needed(array) < 0) {
        // Grow the array beforehand, otherwise it's
        // going to be a mess putting it back together if
        // allocation fails.
        --array->length;
        return -1;
    }

    for (Py_ssize_t i = array->length - 1; i > index; --i) {
        array->items[i] = array->items[i - 1];
    }

    array->items[index] = item;
    return 0;
}

_PyAwaitable_INTERNAL(void)
pyawaitable_array_set(pyawaitable_array * array, Py_ssize_t index, void *item)
{
    pyawaitable_array_ASSERT_VALID(array);
    pyawaitable_array_ASSERT_INDEX(array, index);
    call_deallocator_maybe(array, index);
    array->items[index] = item;
}

static void
remove_no_dealloc(pyawaitable_array *array, Py_ssize_t index)
{
    for (Py_ssize_t i = index; i < array->length - 1; ++i) {
        array->items[i] = array->items[i + 1];
    }
    --array->length;
}

_PyAwaitable_INTERNAL(void)
pyawaitable_array_remove(pyawaitable_array * array, Py_ssize_t index)
{
    pyawaitable_array_ASSERT_VALID(array);
    pyawaitable_array_ASSERT_INDEX(array, index);
    call_deallocator_maybe(array, index);
    remove_no_dealloc(array, index);
}

_PyAwaitable_INTERNAL(void *)
pyawaitable_array_pop(pyawaitable_array * array, Py_ssize_t index)
{
    pyawaitable_array_ASSERT_VALID(array);
    pyawaitable_array_ASSERT_INDEX(array, index);
    void *item = array->items[index];
    remove_no_dealloc(array, index);
    return item;
}

_PyAwaitable_INTERNAL(void)
pyawaitable_array_clear_items(pyawaitable_array * array)
{
    pyawaitable_array_ASSERT_VALID(array);
    for (Py_ssize_t i = 0; i < array->length; ++i) {
        call_deallocator_maybe(array, i);
        array->items[i] = NULL;
    }

    array->length = 0;
}

_PyAwaitable_INTERNAL(void)
pyawaitable_array_clear(pyawaitable_array * array)
{
    pyawaitable_array_ASSERT_VALID(array);
    pyawaitable_array_clear_items(array);
    PyMem_Free(array->items);

    // It would be nice if others could reuse the allocation for another
    // dynarray later, so clear all the fields.
    array->items = NULL;
    array->length = 0;
    array->capacity = 0;
    array->deallocator = NULL;
}
