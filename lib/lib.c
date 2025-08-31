#include <stdio.h>
#include "lib.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "common.h"
#include "html_render.h"
#include "lexer.h"

#include "res/reset.inc"
#include "res/style.inc"

void write_css(unsigned char *data, unsigned int data_len, char *filename, char *dir);

RenderErrorType render(char *srcfile, char *dstdir, RenderError *err) {
    FILE *srcfp = fopen(srcfile, "r");

    if (!srcfp) {
        return OPEN_SRC_FILE_ERROR;
    }

    fseek(srcfp, 0, SEEK_END);
    long srclen = ftell(srcfp);
    rewind(srcfp);

    if (pool_init(1024 * 1024) < 0) {
        return MEM_ALLOC_ERROR;
    }

    byte *srcbuf = pool_alloc(srclen, byte);
    fread(srcbuf, 1, srclen, srcfp);
    fclose(srcfp);

    LexerError *lerr = pool_alloc_struct(LexerError);
    int nlines;
    Token *tokenp = tokenize(srcbuf, srclen, &nlines, lerr);

    if (tokenp == NULL) {
        err->error = (void *) lerr;
        return LEXER_ERROR;
    }

    NodeHeader *node = parse(tokenp);

    int direrr = mkdir(dstdir, 0777);
    assert(direrr == 0 || errno == EEXIST);

    char *html_file = path_join(2, dstdir, path_withext(path_basename_noext(srcfile), ".html"));

    FILE *html_filep = fopen(html_file, "w");

    if (!html_filep) {
        fprintf(stderr, "Could not open for writing %s\n", html_file);
        exit(1);
    }

    write_css(res_reset_css, res_reset_css_len, "reset.css", dstdir);
    write_css(res_style_css, res_style_css_len, "style.css", dstdir);

    gen_html(node, srcfile, nlines, html_filep);
    fclose(html_filep);
    pool_close();
    return SUCCESS;
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
