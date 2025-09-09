#ifndef ZHABA_COMMON_H
#define ZHABA_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned int uint;

typedef struct {
    byte *ptr;
    byte *end;
} Span;

int binsearch_span(Span target, char *arr[], size_t size);
int binsearchs(char *target, char *arr[], size_t size);
int binsearchi(int target, int arr[], size_t size);
int spanstrcmp(Span sp, char *str);
int spancmp(Span sp1, Span sp2);

int pool_init(size_t);
void pool_close();

#define pool_alloc(size, type) ((type *) pool_alloc_align((size), __alignof(type)))
#define pool_alloc_struct(type) pool_alloc(sizeof(type), type)

char *pool_alloc_copy_str(char *);

void *pool_alloc_align(size_t size, size_t align);

typedef struct {
    void *ptr;
    size_t size;
} Slice;

int qsort_strcmp(const void *p1, const void *p2);

char *path_basename_noext(char *);
char *path_join(int, ...);
char *path_join_ssp(char *p1, Span p2);
char *path_withext(char *, char *);

bool file_exists(char *);

#endif //ZHABA_COMMON_H