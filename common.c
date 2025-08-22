#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include "common.h"

#include <stdint.h>

static void *pool_start;
static void *pool_current;
static size_t pool_size;

void pool_init(size_t size) {
    pool_size = size;
    pool_start = malloc(size);
    assert(pool_start != NULL);
    pool_current = pool_start;
}

void *pool_alloc_align(size_t size, size_t align) {
    uintptr_t current_address = (uintptr_t) pool_current;
    uintptr_t aligned_address = (current_address + align - 1) & ~(align - 1);

    assert(aligned_address + size <= (uintptr_t) pool_start + pool_size);

    pool_current = (void *) (aligned_address + size);
    return (void *) aligned_address;
}

int binsearch(Span target, char *arr[], size_t size) {
    int low = 0;
    int high = size - 1;
    int comp;

    while (low <= high) {
        int mid = (low + high) / 2;

        if ((comp = spanstrcmp(target, arr[mid])) < 0) {
            high = mid - 1;
        } else if (comp > 0) {
            low = mid + 1;
        } else {
            return mid;
        }
    }

    return -1;
}

int spanstrcmp(Span sp, char *str) {
    for (; *str == (char) *sp.ptr && sp.ptr < sp.end && *str != '\0'; sp.ptr++, str++)
        ;

    char last = sp.ptr >= sp.end ? '\0' : (char) *sp.ptr;
    return last - *str;
}
