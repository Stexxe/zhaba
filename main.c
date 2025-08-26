#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "lib/lexer.h"
#include "lib/lib.h"

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

    char *outdir = ".";
    if (argc >= 3) {
        outdir = argv[2];
    }

    RenderError err;
    RenderErrorType res = render(argv[1], outdir, &err);

    if (res < 0) {
        switch (res) {
            case OPEN_SRC_FILE_ERROR: {
                throwerr("Could not open %s\n", argv[1]);
            } break;
            case MEM_ALLOC_ERROR: {
                throwerr("Unable to allocate memory\n");
            }
            case LEXER_ERROR: {
                LexerError *lerr = (LexerError *) err.error;
                throwerr("tokenize: unexpected token '%c' at %d:%d", lerr->token, lerr->line, lerr->column);
            } break;
            default: {
                // Do nothing
            } break;
        }
    }

    return 0;
}
