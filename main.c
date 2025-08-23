#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "html_render.h"

int main(int argc, char** argv) {
    if (argc <= 1) {
        fprintf(stderr, "Usage: %s src-file [out-dir]\n", argv[0]);
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
    NodeHeader *node = parse(tokenp);

    char *outdir = ".";
    if (argc >= 3) {
        outdir = argv[2];
    }

    char *html_file = path_join(2, outdir, path_withext(path_basename_noext(argv[1]), ".html"));

    FILE *html_filep = fopen(html_file, "w");

    if (!html_filep) {
        fprintf(stderr, "Could not open for writing %s\n", html_file);
        exit(1);
    }

    gen_html(node, html_filep);

    return 0;
}
