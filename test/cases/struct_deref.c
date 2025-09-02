#include <stdio.h>

struct myStruct {
    int x;
};

int main() {
    struct myStruct a = {123};
    struct myStruct *p = &a;

    printf("%d\n", p->x);

    return 0;
}
