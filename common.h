#ifndef ZHABA_COMMON_H
#define ZHABA_COMMON_H

typedef enum {
    false, true
} boolean;

typedef unsigned char byte;

typedef struct {
    byte *ptr;
    byte *end;
} Span;

int binsearch(Span target, char *arr[], size_t size);
int spanstrcmp(Span sp, char *str);

#endif //ZHABA_COMMON_H