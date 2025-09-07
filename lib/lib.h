#ifndef LIB_H
#define LIB_H

typedef enum {
    SUCCESS = 0,
    OPEN_SRC_FILE_ERROR = -1,
    MEM_ALLOC_ERROR = -2,
    LEXER_ERROR = -3,
} RenderErrorType;

typedef struct {
    RenderErrorType type;
    void *error;
} RenderError;

RenderErrorType render(char *srcfile, char *dstdir, RenderError *);

#endif // LIB_H
