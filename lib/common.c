#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include "common.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void *pool_base;
static void *pool_current;
static size_t pool_size;

int pool_init(size_t size) {
    pool_size = size;
    pool_base = malloc(size);

    if (pool_base == NULL) {
        return -1;
    }

    pool_current = pool_base;
    return 0;
}

void pool_close() {
    free(pool_base);
    pool_current = NULL;
    pool_size = 0;
}

void *pool_alloc_align(size_t size, size_t align) {
    uintptr_t current_address = (uintptr_t) pool_current;
    uintptr_t aligned_address = (current_address + align - 1) & ~(align - 1);

    assert(aligned_address + size <= (uintptr_t) pool_base + pool_size); // TODO: Grow if needed

    memset((void *) aligned_address, 0, size);

    pool_current = (void *) (aligned_address + size);
    return (void *) aligned_address;
}

char *pool_alloc_copy_str(char *str) {
    char *as = pool_alloc(strlen(str) + 1, char);
    strcpy(as, str);
    return as;
}

int binsearchs(char *target, char *arr[], size_t size) {
    int low = 0;
    int high = size - 1;
    int comp;

    while (low <= high) {
        int mid = (low + high) / 2;

        if ((comp = strcmp(target, arr[mid])) < 0) {
            high = mid - 1;
        } else if (comp > 0) {
            low = mid + 1;
        } else {
            return mid;
        }
    }

    return -1;
}

int binsearchi(int target, int arr[], size_t size) {
    int low = 0;
    int high = size - 1;

    while (low <= high) {
        int mid = (low + high) / 2;

        if (target < arr[mid]) {
            high = mid - 1;
        } else if (target > arr[mid]) {
            low = mid + 1;
        } else {
            return mid;
        }
    }

    return -1;
}

int binsearch_span(Span target, char *arr[], size_t size) {
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

int spancmp(Span sp1, Span sp2) {
    byte *cp1, *cp2;

    for (cp1 = sp1.ptr, cp2 = sp2.ptr; *cp1 == *cp2 && cp1 < sp1.end && cp2 < sp2.end; cp1++, cp2++)
        ;

    return (cp1 < sp1.end ? *cp1 : 0) - (cp2 < sp2.end ? *cp2 : 0);
}

int spanstrcmp(Span sp, char *str) {
    for (; *str == (char) *sp.ptr && sp.ptr < sp.end && *str != '\0'; sp.ptr++, str++)
        ;

    char last = sp.ptr >= sp.end ? '\0' : (char) *sp.ptr;
    return last - *str;
}

int qsort_strcmp(const void *p1, const void *p2) {
    return strcmp(*(const char **) p1, *(const char **) p2);
}

char *path_basename_noext(char *path) {
    char *p;
    for (p = path; *p != '\0'; p++)
        ;

    char *end = p;

    for (; p >= path && *p != '/'; p--) {
        if (*p == '.') {
            end = p;
        }
    }

    if (*p == '/') p++;

    size_t len = end - p + 1;
    char *r = pool_alloc(len, char);
    memcpy(r, p, len);
    r[len - 1] = '\0';
    return r;
}

char *path_join(int count, ...) {
    va_list ap;
    va_start(ap, count);
    char *p;

    size_t len = 0;
    int i = count;
    while (i-- > 0) {
        p = va_arg(ap, char *);
        len += strlen(p) + 1;
    }

    va_end(ap);

    char *r = pool_alloc(len, char);

    va_start(ap, count);
    i = count;
    char *pos = r;
    while (i-- > 0) {
        p = va_arg(ap, char *);

        while ((*pos++ = *p++))
            ;
        pos--;
        if (i != 0) *pos++ = '/';
    }

    *pos = '\0';
    va_end(ap);
    return r;
}

char *path_withext(char *name, char *ext) {
    size_t len = strlen(name) + strlen(ext) + 1;
    char *r = pool_alloc(len, char);
    char *pos = r;

    while ((*pos++ = *name++))
        ;
    pos--;

    while ((*pos++ = *ext++))
        ;
    pos--;
    *pos = '\0';
    return r;
}