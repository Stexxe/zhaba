#include <stdio.h>

int main() {
    int a = 3;

    switch (a) {
        case 1: {
            printf("1");
            break;
        }
        case 2: {
            printf("2");
        }
        case 3:
        case 4:
            printf("3 or 4");
            break;
        default: {
            printf("default");
        } break;
    }

    return 0;
}
