#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "html_reader.h"
#include "../lib/lexer.h"
#include "../lib/lib.h"
#include "../lib/parser.h"
#include "../lib/prep.h"

static char *path_joinm(char *p1, char *p2);
static char *path_replace_ext(char *p, char *ext);
static bool endswith(char *hay, char *needle);

static void decode_source(char *);
static void print_context(char *ctx, int pos, int target_pos);
static void write_escape_invisible(char, FILE *);
static void assert_equal(char *expfile, char *actual_str, char *testname);

static void run_prep_tests(char *dir);

#define SOURCE_MAX_LEN 8096
static char source[SOURCE_MAX_LEN];
static char decoded_source[SOURCE_MAX_LEN];

#define MAX_CONTEXT 80
static char actual_ctx[MAX_CONTEXT];
static char exp_ctx[MAX_CONTEXT];

#define MAX_MEM (32 * 1024 * 1024)

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s cases-dir\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    DIR *dir = opendir(argv[1]);

    if (!dir) {
        fprintf(stderr, "Could not open %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if (pool_init(MAX_MEM) < 0) {
        return fprintf(stderr, "test: cannot allocate pool of %d bytes\n", MAX_MEM);
    }

    struct dirent *ent;
    char *outdir = "temp";
    int direrr = mkdir(outdir, 0777);
    assert(direrr == 0 || errno == EEXIST);

    lexer_init();
    parser_init();

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        if (strcmp("prep", ent->d_name) == 0) {
            run_prep_tests(path_joinm(argv[1], ent->d_name));
            continue;
        }

        if (argc == 3 && strcmp(argv[2], ent->d_name) != 0) continue;

        if (endswith(ent->d_name, ".c")) {
            char *srcpath = path_joinm(argv[1], ent->d_name);
            RenderError err;
            RenderErrorType res = render(srcpath, outdir, &err);

            if (res < 0) {
                switch (res) {
                    case OPEN_SRC_FILE_ERROR: {
                        fprintf(stderr, "Could not open %s\n", argv[1]);
                    } break;
                    case MEM_ALLOC_ERROR: {
                        fprintf(stderr, "Unable to allocate memory\n");
                    }
                    case LEXER_ERROR: {
                        LexerError *lerr = (LexerError *) err.error;
                        fprintf(stderr, "tokenize: unexpected token '%c' at %d:%d", lerr->token, lerr->line, lerr->column);
                    } break;
                    default: {
                        // Do nothing
                    } break;
                }

                exit(EXIT_FAILURE);
            }

            char *htmlfilename = path_replace_ext(ent->d_name, ".html");
            char *htmlpath = path_joinm(outdir, htmlfilename);

            FILE *htmlfp = fopen(htmlpath, "r");
            assert(htmlfp != NULL);

            HtmlReader *hr = html_new_reader(htmlfp);

            HtmlRecord record;
            do {
                record = html_next_tag(hr);

                if (strcmp("source", record.class) == 0) {
                    html_read_content(hr, source, SOURCE_MAX_LEN);
                    break;
                }
            } while (!record.eof);

            html_close_reader(hr);
            fclose(htmlfp);
            decode_source(source);

            char *exp_filepath = path_joinm(argv[1], htmlfilename);
            assert_equal(exp_filepath, decoded_source, ent->d_name);
        }
    }

    return 0;
}

static void run_prep_tests(char *dir) {
    DIR *dirp = opendir(dir);

    if (!dirp) {
        fprintf(stderr, "Could not open %s\n", dir);
        exit(EXIT_FAILURE);
    }

    const int size = 2 * 1024;
    struct dirent *ent;
    while ((ent = readdir(dirp)) != NULL) {
        if (endswith(ent->d_name, ".c") && !endswith(ent->d_name, ".exp.c")) {
            int outsz = size;
            char *out = pool_alloc(outsz, char);
            DefineTable *def_table = prep_define_newtable();

            char *expanded_src = prep_expand(path_joinm(dir, ent->d_name), def_table, out, &outsz);
            expanded_src[size - outsz] = '\0';
            char *exp_filepath = path_joinm(dir, path_replace_ext(ent->d_name, ".exp.c"));
            assert_equal(exp_filepath, expanded_src, ent->d_name);
        }
    }
}

static void assert_equal(char *expfile, char *actual_str, char *testname) {
    FILE *expf = fopen(expfile, "r");

    if (!expf) {
        fprintf(stderr, "assert_equal: cannot open %s\n", expfile);
        exit(EXIT_FAILURE);
    }

    int line, column;
    line = column = 1;
    int expc;
    char *actp;
    int exp_ctx_pos = 0;
    int act_ctx_pos = 0;
    for (expc = getc(expf), actp = actual_str; expc != EOF && *actp != '\0'; expc = getc(expf), actp++) {
        exp_ctx[exp_ctx_pos++ % MAX_CONTEXT] = (char) expc;
        actual_ctx[act_ctx_pos++ % MAX_CONTEXT] = *actp;

        if (expc == *actp) {
            if (expc == '\n') {
                line++;
                column = 1;
            }
        } else {
            break;
        }

        column++;
    }

    bool act_empty = strlen(actual_str) == 0;

    if (act_empty) {
        exp_ctx[exp_ctx_pos++ % MAX_CONTEXT] = (char) expc;
    }

    if (expc != EOF || *actp != '\0') {
        int exp_target_pos = (exp_ctx_pos - 1) % MAX_CONTEXT;
        int act_target_pos = (act_ctx_pos - 1) % MAX_CONTEXT;
        int k = MAX_CONTEXT / 2;
        while ((expc = getc(expf)) != EOF && k-- > 0) {
            exp_ctx[exp_ctx_pos++ % MAX_CONTEXT] = (char) expc;
        }

        char c;
        k = MAX_CONTEXT / 2;
        while (*actp != '\0' && (c = *++actp) != '\0' && k-- > 0) {
            actual_ctx[act_ctx_pos++ % MAX_CONTEXT] = c;
        }

        k = MAX_CONTEXT / 2;

        fprintf(stderr, "Case %s failed. ", testname);
        fprintf(stderr, "Unexpected character '");
        if (!act_empty) write_escape_invisible(actual_ctx[act_target_pos], stderr);
        fprintf(stderr, "' at %d:%d\n", line, column);

        fprintf(stderr, "Expect: ");
        print_context(exp_ctx, exp_ctx_pos, exp_target_pos);
        fprintf(stderr, "\n\n");
        fprintf(stderr, "Actual: ");
        print_context(actual_ctx, act_ctx_pos, act_target_pos);
        fprintf(stderr, "\n\n");
        exit(EXIT_FAILURE);
    }

    fclose(expf);
}

static void write_escape_invisible(char c, FILE *f) {
    switch (c) {
        case '\n': fprintf(f, "\\n"); break;
        case '\t': fprintf(f, "\\t"); break;
        case '\r': fprintf(f, "\\r"); break;
        default: {
            fprintf(f, "%c", c);
        } break;
    }
}

static void print_context(char *ctx, int pos, int target_pos) {
    int i;
    int max = pos > MAX_CONTEXT ? MAX_CONTEXT : pos;
    pos %= MAX_CONTEXT;

    for (i = pos; i < max; i++) {
        if (i == target_pos) {
            fprintf(stderr, "\033[7m%c\033[0m", ctx[target_pos]);
        } else {
            fputc(ctx[i], stderr);
        }
    }

    for (i = 0; i < pos; i++) {
        if (i == target_pos) {
            fprintf(stderr, "\033[7m%c\033[0m", ctx[target_pos]);
        } else {
            fputc(ctx[i], stderr);
        }
    }
}

static int cmp(char *s1, size_t s1len, char *s2) {
    for (; *s1 == *s2; s1len--, s1++, s2++)
        ;

    return (s1len == 0 ? '\0' : *s1) - *s2;
}

// TODO: Skip attributes
static void decode_source(char *src) {
    char c;
    int len = 0;
    char *sp;
    char *destp = decoded_source;

    while ((c = *src++) != '\0') {
        if (c == '<' && isalpha(*src)) {
            sp = src;
            len = 0;

            while (*src != '\0' && *src != '>') {
                len++;
                src++;
            }

            assert(*src == '>');

            if (cmp(sp, len, "br") == 0) {
                *destp++ = '\n';
                src++;
            } else {
                *destp++ = c;
                src = sp;
            }
        } else if (c == '&') {
            sp = src;
            len = 0;
            while (isalpha(*src)) {
                src++;
                len++;
            }

            if (*src == ';' && cmp(sp, len, "nbsp") == 0) {
                *destp++ = ' ';
                src++;
            } else {
                *destp++ = c;
                src = sp;
            }
        } else {
            *destp++ = c;
        }
    }

    *destp = '\0';
}

static char *path_joinm(char *p1, char *p2) {
    char *p = (char *) malloc(strlen(p1) + strlen(p2) + 2);
    assert(p != NULL);
    sprintf(p, "%s/%s", p1, p2);
    return p;
}

static char *path_replace_ext(char *p, char *ext) {
    char *cp = p + strlen(p);

    for (; cp >= p && *cp != '.'; cp--)
        ;

    if (*cp == '.') {
        size_t ext_len = strlen(ext);
        char *res = malloc((cp - p) + ext_len + 1);
        stpncpy(res, p, cp - p);
        stpncpy(res + (cp - p), ext, ext_len + 1);
        return res;
    }

    return p;
}

static bool endswith(char *hay, char *needle) {
    char *hp, *np;

    for (hp = hay + strlen(hay), np = needle + strlen(needle); hp >= hay && np >= needle && *np == *hp; hp--, np--)
        ;

    return np < needle;
}