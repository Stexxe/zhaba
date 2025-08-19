#include <stdio.h>
#include <stdlib.h>
#include "lexer.c"

int main(int argc, char** argv) {
    if (argc <= 1) {
        fprintf(stderr, "Usage: %s src-file\n", argv[0]);
    }

    FILE *srcfp = fopen(argv[1], "r");

    if (!srcfp) {
        fprintf(stderr, "Could not open %s\n", argv[1]);
        exit(1);
    }

    fseek(srcfp, 0, SEEK_END);
    long srclen = ftell(srcfp);
    rewind(srcfp);

    byte *srcbuf = (byte *) malloc(srclen);
    fread(srcbuf, 1, srclen, srcfp);
    fclose(srcfp);

    tokenize(srcbuf, srclen);

    // int size;
    // rune r = read_rune(srcbuf, &size);

    return 0;
}
