#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "html_render.h"

#include "res/reset.inc"
#include "res/style.inc"

void write_css(unsigned char *data, unsigned int data_len, char *filename, char *dir);
void throwerr(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    vfprintf(stderr, fmt, ap);

    va_end(ap);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        throwerr("Usage: %s src-file [out-dir]\n", argv[0]);
    }

    FILE *srcfp = fopen(argv[1], "r");

    if (!srcfp) {
        throwerr("Could not open %s\n", argv[1]);
    }

    fseek(srcfp, 0, SEEK_END);
    long srclen = ftell(srcfp);
    rewind(srcfp);

    if (pool_init(1024 * 1024) < 0) {
        throwerr("Unable to allocate memory\n");
    }

    byte *srcbuf = pool_alloc(srclen, byte);
    fread(srcbuf, 1, srclen, srcfp);
    fclose(srcfp);

    LexerError lerr;
    int nlines;
    Token *tokenp = tokenize(srcbuf, srclen, &nlines, &lerr);

    if (tokenp == NULL) {
        throwerr("tokenize: unexpected token '%c' at %d:%d", lerr.token, lerr.line, lerr.column);
    }

    NodeHeader *node = parse(tokenp);

    char *outdir = ".";
    if (argc >= 3) {
        outdir = argv[2];
    }

    int err = mkdir(outdir, 0777);
    assert(err == 0 || errno == EEXIST);

    char *html_file = path_join(2, outdir, path_withext(path_basename_noext(argv[1]), ".html"));

    FILE *html_filep = fopen(html_file, "w");

    if (!html_filep) {
        fprintf(stderr, "Could not open for writing %s\n", html_file);
        exit(1);
    }

    write_css(res_reset_css, res_reset_css_len, "reset.css", outdir);
    write_css(res_style_css, res_style_css_len, "style.css", outdir);

    gen_html(node, argv[1], nlines, html_filep);

    return 0;
}

void write_css(unsigned char *data, unsigned int data_len, char *filename, char *dir) {
    char *css_dir = path_join(2, dir, "css");

    int err = mkdir(css_dir, 0777);
    assert(err == 0 || errno == EEXIST);

    char *css_file = path_join(2, css_dir, filename);
    FILE *css_filep = fopen(css_file, "w");
    assert(css_filep);

    size_t nitems = fwrite(data, 1, data_len, css_filep);

    if (nitems <= 0) {
        fprintf(stderr, "Could not write to %s\n", css_file);
    }

    fclose(css_filep);
}
