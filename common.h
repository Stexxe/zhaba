#ifndef ZHABA_COMMON_H
#define ZHABA_COMMON_H

#include <stddef.h>

typedef unsigned char byte;

typedef struct {
    byte *ptr;
    byte *end;
} Span;

int binsearch(Span target, char *arr[], size_t size);
int spanstrcmp(Span sp, char *str);

void pool_init(size_t);
#define pool_alloc(size, type) ((type *) pool_alloc_align((size), __alignof(type)))
#define pool_alloc_struct(type) pool_alloc(sizeof(type), type)

void *pool_alloc_align(size_t size, size_t align);

typedef struct {
    void *ptr;
    size_t size;
} Slice;

#endif //ZHABA_COMMON_H