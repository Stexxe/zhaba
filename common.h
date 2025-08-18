#ifndef ZHABA_COMMON_H
#define ZHABA_COMMON_H
#include <string.h>

typedef enum {
    false, true
} boolean;

int binsearch(char *target, const char *arr[], size_t size) {
    size_t low = 0;
    size_t high = size - 1;
    int comp;

    while (low <= high) {
        size_t mid = low + (high - low) / 2;

        if ((comp = strcmp(target, arr[mid])) < 0) {
            high = mid - 1;
        } else if (comp > 0) {
            low = mid + 1;
        } else {
            return (int) mid;
        }
    }

    return -1;
}

#endif //ZHABA_COMMON_H