#include <stdio.h>

struct myStruct {
    int x;
};

int main() {
    struct myStruct a = {123};

    printf("%d\n", a.x);

    return 0;
}
