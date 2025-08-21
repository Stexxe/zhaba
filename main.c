#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "lexer.h"
#include "parser.h"

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

    pool_init(1024 * 1024);

    byte *srcbuf = pool_alloc(srclen, byte);
    fread(srcbuf, 1, srclen, srcfp);
    fclose(srcfp);

    Token *tokenp = tokenize(srcbuf, srclen);
    parse(tokenp);

    return 0;
}
