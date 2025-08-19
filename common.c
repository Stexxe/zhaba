#include <stddef.h>
#include "common.h"

int binsearch(Span target, char *arr[], size_t size) {
    size_t low = 0;
    size_t high = size - 1;
    int comp;

    while (low <= high) {
        size_t mid = low + (high - low) / 2;

        if ((comp = spanstrcmp(target, arr[mid])) < 0) {
            high = mid - 1;
        } else if (comp > 0) {
            low = mid + 1;
        } else {
            return (int) mid;
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
