#include "prep.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

typedef struct DefineKv {
    Span key;
    void *value;
    struct DefineKv *next;
} DefineKv;

struct DefineTable {
    DefineKv **ptr;
    size_t size;
};

DefineTable *prep_define_newtable() {
    DefineTable *t = pool_alloc_struct(DefineTable);

    t->size = 32;
    t->ptr = pool_alloc(sizeof(DefineKv *) * t->size, DefineKv *);

    for (int i = 0; i < t->size; i++) {
        t->ptr[i] = NULL;
    }

    return t;
}

uint hash(Span key, size_t tsize) {
    uint hash = 2166136261;

    for (byte *cp = key.ptr; cp < key.end; cp++) {
        hash ^= (uint)(*cp);
        hash *= 16777619;
    }

    return hash % tsize;
}

static DefineKv *new_kv(Span key, void *value) {
    DefineKv *kv = pool_alloc_struct(DefineKv);
    kv->key = key;
    kv->value = value;
    kv->next = NULL;
    return kv;
}

void prep_define_set(DefineTable *table, Span key, void *value) {
    uint h = hash(key, table->size);
    DefineKv *kv = table->ptr[h];

    if (kv == NULL) {
        table->ptr[h] = new_kv(key, value);
    } else {
        DefineKv *head = kv;
        DefineKv *prev_found = NULL, *found = NULL;

        for (kv = head; kv != NULL; prev_found = kv, kv = kv->next) {
            if (spancmp(kv->key, key) == 0) {
                found = kv;
                break;
            }
        }

        DefineKv *nkv = new_kv(key, value);

        if (found) {
            nkv->next = found->next;
            if (prev_found) prev_found->next = nkv;
            else table->ptr[h] = nkv;
        } else {
            nkv->next = head;
            table->ptr[h] = nkv;
        }
    }
}

void *prep_define_get(DefineTable *table, Span key) {
    uint h = hash(key, table->size);
    DefineKv *kv = table->ptr[h];

    for ( ; kv != NULL; kv = kv->next) {
        if (spancmp(kv->key, key) == 0) return kv->value;
    }

    return NULL;
}

// typedef struct {
//     char *directive;
//     void (*expand)(FILE *, char *);
// } Expander;
//
// #define MAX_BUF_SIZE 1024
// static char buf[MAX_BUF_SIZE];
//
// static void expand_nothing(FILE* srcfp, char *out) {}
//
// static void expand_include(FILE* srcfp, char *out) {
//     int c;
//     while ((c = getc(srcfp)) != EOF && isspace(c))
//         ;
//
//     if (c == '"') {
//         char *local_path = buf;
//         while ((c = getc(srcfp)) != EOF && c != '"') {
//             *local_path++ = (char) c;
//         }
//
//         *local_path = '\0';
//
//         while ((c = getc(srcfp)) != EOF && isspace(c) && c != '\n')
//             ;
//
//         // Get srcfile dir
//         // Build path to the file
//         // Read file
//         // Write to out in a loop
//     }
// }
//
// // TODO: Sort once
// static Expander prep_directives[] = {
//     (Expander) {"define", expand_nothing},
//     (Expander) {"elif", expand_nothing},
//     (Expander) {"else", expand_nothing},
//     (Expander) {"endif", expand_nothing},
//     (Expander) {"error", expand_nothing},
//     (Expander) {"if", expand_nothing},
//     (Expander) {"ifdef", expand_nothing},
//     (Expander) {"ifndef", expand_nothing},
//     (Expander) {"include", expand_include},
//     (Expander) {"line", expand_nothing},
//     (Expander) {"pragma", expand_nothing},
//     (Expander) {"undef", expand_nothing},
// };
//
// #define PREP_DIRECTIVES_SIZE (sizeof(prep_directives) / sizeof(prep_directives[0]))
//
// static int binsearch_lex_span(char *target, Expander *arr, size_t size) {
//     int low = 0;
//     int high = (int) size - 1;
//     int comp;
//
//     while (low <= high) {
//         int mid = (low + high) / 2;
//
//         if ((comp = strcmp(target, arr[mid].directive)) < 0) {
//             high = mid - 1;
//         } else if (comp > 0) {
//             low = mid + 1;
//         } else {
//             return mid;
//         }
//     }
//
//     return -1;
// }

static char **search_paths;
static size_t search_paths_size;

void prep_search_paths_set(char **paths, size_t paths_size) {
    search_paths = paths;
    search_paths_size = paths_size;
}

static char *expand(Span sp, char *dirpath, DefineTable *def_table, char *out, int *outsz);
static bool eval_expr(Span expr, DefineTable *def_table);

byte *read_src(char *path, size_t *size) {
    FILE *f = fopen(path, "r");
    assert(f); // TODO: Error handling
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);

    byte *data = pool_alloc(*size, byte);
    size_t rsz = fread(data, 1, *size, f);
    assert(rsz == *size); // TODO: Error handling
    fclose(f);
    return data;
}

char *prep_expand(char *srcfile, DefineTable *def_table, char *out, int *outsz) {
    size_t srcsize;
    byte *src = read_src(srcfile, &srcsize);

    char *srcdir_end;

    for (srcdir_end = srcfile + strlen(srcfile); srcdir_end >= srcfile && *srcdir_end != '/'; srcdir_end--)
        ;

    char *dirpath;
    if (srcdir_end >= srcfile) {
        dirpath = pool_alloc(srcdir_end - srcfile + 1, char);
        strncpy(dirpath, srcfile, srcdir_end - srcfile);
    } else {
        dirpath = pool_alloc(1 + 1, char);
        strncpy(dirpath, ".", 1);
    }

    return expand((Span){src, src + srcsize}, dirpath, def_table, out, outsz);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

struct reader {
    byte *ptr;
    byte *end;
    byte cur;
};


// true if it has something
static bool readnext(struct reader *r) {
    if (r->ptr >= r->end) return false;

    r->cur = *r->ptr;
    r->ptr++;

    if (r->ptr >= r->end) return true;

    if (r->cur == '\\') {
        r->cur = (*r->ptr == '\n' ? ' ' : *r->ptr);
        r->ptr++;
    }

    return true;
}

static void read_while(struct reader *r, int (*cmp)(int)) {
    bool cond = true;
    while (readnext(r) && (cond = cmp(r->cur)))
        ;

    if (!cond) {
        r->ptr--;
    }
}

static void read_until(struct reader *r, int (*cmp)(int)) {
    bool cond = true;
    while (readnext(r) && !(cond = cmp(r->cur)))
        ;

    if (cond) {
        r->ptr--;
    }
}

static void read_until_char(struct reader *r, byte c) {
    bool cond = true;
    while (readnext(r) && !(cond = (c == r->cur)))
        ;

    if (cond) {
        r->ptr--;
    }
}

static void read_spaces_until_lf(struct reader *r) {
    while (readnext(r) && isspace(r->cur) && r->cur != '\n')
        ;
}

static int isid(int c) {
    return isalnum(c) || c == '_';
}

// TODO: Combine out and outsz
static char *expand(Span sp, char *dirpath, DefineTable *def_table, char *out, int *outsz) {
    char *outp = out;
    assert(*outsz > 0);

    struct reader r = {.ptr = sp.ptr, .end = sp.end};

    while (readnext(&r)) {
        if (r.cur == '#') {
            read_while(&r, isspace);
            Span directive = {r.ptr, r.ptr};
            read_until(&r, isspace);
            directive.end = r.ptr;

            if (spanstrcmp(directive, "include") == 0) {
                read_while(&r, isspace);

                if (r.cur == '"') {
                    readnext(&r);
                    Span local_path = {r.ptr, r.ptr};

                    read_until_char(&r, '"');
                    local_path.end = r.ptr;

                    assert(r.cur == '"');
                    readnext(&r);

                    read_spaces_until_lf(&r);
                    char *inc_path = path_join_ssp(dirpath, local_path);

                    outp = prep_expand(inc_path, def_table, outp, outsz);
                } else if (r.cur == '<') {
                    readnext(&r);
                    Span ext_path = {r.ptr, r.ptr};

                    read_until_char(&r, '>');
                    ext_path.end = r.ptr;

                    assert(r.cur == '>');
                    readnext(&r);

                    read_spaces_until_lf(&r);

                    char *inc_path = NULL;
                    for (int i = 0; i < search_paths_size; i++) {
                        char *p = search_paths[i];
                        inc_path = path_join_ssp(p, ext_path);
                        if (file_exists(inc_path)) break;
                    }

                    assert(inc_path != NULL);
                    outp = prep_expand(inc_path, def_table, outp, outsz);
                }
            } else if (spanstrcmp(directive, "define") == 0) {
                read_while(&r, isspace);

                Span id = {r.ptr, r.ptr};

                read_until(&r, isspace);
                id.end = r.ptr;
                while (readnext(&r) && isspace(r.cur) && r.cur != '\n')
                    ;

                Span *content = pool_alloc_struct(Span);
                if (r.cur == '\n') {
                    content->ptr = content->end = NULL;
                    // r.ptr = min(r.ptr + 1, r.end);
                } else {
                    content->ptr = content->end = r.ptr;
                    read_until(&r, isspace);
                    content->end = r.ptr;
                    read_spaces_until_lf(&r);
                }

                prep_define_set(def_table, id, content);
            } else if (spanstrcmp(directive, "ifdef") == 0 || spanstrcmp(directive, "ifndef") == 0) {
                read_while(&r, isspace);

                Span id = {r.ptr, r.ptr};

                read_until(&r, isspace);
                id.end = r.ptr;

                read_spaces_until_lf(&r);

                Span content = {r.ptr, r.ptr};

                char *term = "#endif";
                size_t termlen = strlen(term);
                for ( ; content.end + termlen <= sp.end; content.end++) {
                    if (spanstrcmp((Span){content.end, content.end + termlen}, term) == 0) {
                        if (*(content.end-1) == '\n') content.end--;
                        break;
                    }
                }

                void *repl = prep_define_get(def_table, id);

                if ((spanstrcmp(directive, "ifdef") == 0) && repl || (spanstrcmp(directive, "ifndef") == 0 && !repl)) {
                    outp = expand(content, dirpath, def_table, outp, outsz);
                }

                r.ptr = content.end + termlen + (*content.end == '\n' ? 1 : 0);
            } else if (spanstrcmp(directive, "undef") == 0) {
                read_while(&r, isspace);

                Span id = {r.ptr, r.ptr};

                read_until(&r, isspace);
                id.end = r.ptr;

                read_spaces_until_lf(&r);

                prep_define_set(def_table, id, NULL);
            } else if (spanstrcmp(directive, "if") == 0) {
                read_while(&r, isspace);

                Span expr = {r.ptr, r.ptr};

                read_until_char(&r, '\n');
                expr.end = r.ptr;

                read_spaces_until_lf(&r);
                Span content = {r.ptr, r.ptr};

                char *term = "#endif";
                size_t termlen = strlen(term);
                for ( ; content.end + termlen <= sp.end; content.end++) {
                    if (spanstrcmp((Span){content.end, content.end + termlen}, term) == 0) {
                        if (*(content.end-1) == '\n') content.end--;
                        break;
                    }
                }

                if (eval_expr(expr, def_table)) {
                    outp = expand(content, dirpath, def_table, outp, outsz);
                }

                r.ptr = content.end + termlen + (*content.end == '\n' ? 1 : 0);
            } else {
                fprintf(stderr, "expand: unrecognized directive '");
                for (byte *cp = directive.ptr; cp < directive.end; cp++) {
                    fprintf(stderr, "%c", *cp);
                }
                fprintf(stderr, "'\n");
                assert(0);
            }
        } else if (r.cur == '/') {
            Span comment = {r.ptr-1, r.ptr};

            if (comment.end < sp.end) {
                if (*comment.end == '/') {
                    for (; comment.end < sp.end && *comment.end != '\n' ; comment.end++)
                        ;
                } else if (*comment.end == '*') {
                    for (; comment.end+1 < sp.end && (*comment.end != '*' || *(comment.end+1) != '/'); comment.end++)
                        ;

                    comment.end += 2;
                }
            }

            for (byte *cp = comment.ptr; cp < sp.end && cp < comment.end; cp++) {
                *outp++ = (char) *cp;
                *outsz -= 1;
            }
            r.ptr = comment.end;
        } else if (r.cur == '"' || r.cur == '\'') {
            Span literal = {r.ptr, r.ptr + 1};

            for (; literal.end < sp.end; literal.end++) {
                if (*literal.end == '\\') {
                    ++literal.end;
                } else if (*literal.end == *literal.ptr) {
                    break;
                }
            }

            for (byte *cp = literal.ptr; cp < sp.end && cp < literal.end; cp++) {
                *outp++ = (char) *cp;
                *outsz -= 1;
            }
            r.ptr = literal.end;
        } else if (isalpha(r.cur)) {
            Span id = {r.ptr-1, r.ptr};
            read_while(&r, isid);
            id.end = r.ptr;

            Span *repl = (Span *) prep_define_get(def_table, id);

            if (repl == NULL) {
                for ( ; id.ptr < id.end; id.ptr++) {
                    *outp++ = (char) *id.ptr;
                    *outsz -= 1;
                }
            } else {
                for (byte *bp = repl->ptr; bp < repl->end; bp++) {
                    *outp++ = (char) *bp;
                    *outsz -= 1;
                }
            }
        } else {
            *outp++ = (char) r.cur;
            *outsz -= 1;
        }
    }

    // for (byte *srcp = sp.ptr; srcp < sp.end;) {
    //     readnext(&r);
    //     if (b == '#') {
    //         // ++srcp;
    //
    //         while (readnext(&srcp, sp.end) && isspace(readnext(&srcp, srcp))) {}
    //         for ( ; srcp < sp.end && isspace(*srcp); srcp++)
    //             ;
    //
    //         Span directive = {srcp, srcp};
    //
    //         for ( ; srcp < sp.end && !isspace(*srcp) ; srcp++) {
    //             directive.end++;
    //         }
    //
    //         if (spanstrcmp(directive, "include") == 0) {
    //             for ( ; srcp < sp.end && isspace(*srcp); srcp++)
    //                 ;
    //
    //             if (*srcp == '"') {
    //                 ++srcp;
    //                 Span local_path = {srcp, srcp};
    //
    //                 for ( ; srcp < sp.end && *srcp != '"'; srcp++) {
    //                     local_path.end++;
    //                 }
    //
    //                 assert(*srcp == '"');
    //                 ++srcp;
    //
    //                 for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
    //                     ;
    //
    //                 if (srcp < sp.end && *srcp == '\n') srcp++;
    //
    //                 char *inc_path = path_join_ssp(dirpath, local_path);
    //
    //                 outp = prep_expand(inc_path, def_table, outp, outsz);
    //             } else if (*srcp == '<') {
    //                 ++srcp;
    //                 Span ext_path = {srcp, srcp};
    //
    //                 for ( ; srcp < sp.end && *srcp != '>'; srcp++) {
    //                     ext_path.end++;
    //                 }
    //
    //                 assert(*srcp == '>');
    //                 ++srcp;
    //
    //                 for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
    //                     ;
    //
    //                 if (srcp < sp.end && *srcp == '\n') srcp++;
    //
    //                 char *inc_path = NULL;
    //                 for (int i = 0; i < search_paths_size; i++) {
    //                     char *p = search_paths[i];
    //                     inc_path = path_join_ssp(p, ext_path);
    //                     if (file_exists(inc_path)) break;
    //                 }
    //
    //                 assert(inc_path != NULL);
    //                 outp = prep_expand(inc_path, def_table, outp, outsz);
    //             }
    //         } else if (spanstrcmp(directive, "define") == 0) {
    //             for ( ; srcp < sp.end && isspace(*srcp); srcp++)
    //                 ;
    //
    //             Span id = {srcp, srcp};
    //
    //             for ( ; srcp < sp.end && !isspace(*srcp) ; srcp++) {
    //                 id.end++;
    //             }
    //
    //             for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
    //                 ;
    //
    //             Span *content = pool_alloc_struct(Span);
    //             if (*srcp == '\n') {
    //                 content->ptr = content->end = NULL;
    //                 srcp = min(srcp + 1, sp.end);
    //             } else {
    //                 content->ptr = content->end = srcp;
    //                 for ( ; srcp < sp.end && !isspace(*srcp) ; srcp++) {
    //                     content->end++;
    //                 }
    //                 for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
    //                     ;
    //
    //                 if (srcp < sp.end && *srcp == '\n') srcp++;
    //             }
    //
    //             prep_define_set(def_table, id, content);
    //         } else if (spanstrcmp(directive, "ifdef") == 0 || spanstrcmp(directive, "ifndef") == 0) {
    //             for ( ; srcp < sp.end && isspace(*srcp); srcp++)
    //                 ;
    //
    //             Span id = {srcp, srcp};
    //
    //             for ( ; srcp < sp.end && !isspace(*srcp) ; srcp++) {
    //                 id.end++;
    //             }
    //
    //             for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
    //                 ;
    //
    //             if (srcp < sp.end && *srcp == '\n') srcp++;
    //
    //             Span content = {srcp, srcp};
    //
    //             char *term = "#endif";
    //             size_t termlen = strlen(term);
    //             for ( ; content.end + termlen <= sp.end; content.end++) {
    //                 if (spanstrcmp((Span){content.end, content.end + termlen}, term) == 0) {
    //                     if (*(content.end-1) == '\n') content.end--;
    //                     break;
    //                 }
    //             }
    //
    //             void *repl = prep_define_get(def_table, id);
    //
    //             if ((spanstrcmp(directive, "ifdef") == 0) && repl || (spanstrcmp(directive, "ifndef") == 0 && !repl)) {
    //                 outp = expand(content, dirpath, def_table, outp, outsz);
    //             }
    //
    //             srcp = content.end + termlen + (*content.end == '\n' ? 1 : 0);
    //         } else if (spanstrcmp(directive, "undef") == 0) {
    //             for ( ; srcp < sp.end && isspace(*srcp); srcp++)
    //                 ;
    //
    //             Span id = {srcp, srcp};
    //
    //             for ( ; srcp < sp.end && !isspace(*srcp) ; srcp++) {
    //                 id.end++;
    //             }
    //
    //             for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
    //                 ;
    //
    //             if (srcp < sp.end && *srcp == '\n') srcp++;
    //
    //             prep_define_set(def_table, id, NULL);
    //         } else if (spanstrcmp(directive, "if") == 0) {
    //             for ( ; srcp < sp.end && isspace(*srcp); srcp++)
    //                 ;
    //
    //             Span expr = {srcp, srcp};
    //
    //             for ( ; srcp < sp.end && *srcp != '\n'; srcp++) {
    //                 expr.end++;
    //             }
    //
    //             for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
    //                 ;
    //
    //             if (srcp < sp.end && *srcp == '\n') srcp++;
    //             Span content = {srcp, srcp};
    //
    //             char *term = "#endif";
    //             size_t termlen = strlen(term);
    //             for ( ; content.end + termlen <= sp.end; content.end++) {
    //                 if (spanstrcmp((Span){content.end, content.end + termlen}, term) == 0) {
    //                     if (*(content.end-1) == '\n') content.end--;
    //                     break;
    //                 }
    //             }
    //
    //             if (eval_expr(expr, def_table)) {
    //                 outp = expand(content, dirpath, def_table, outp, outsz);
    //             }
    //
    //             srcp = content.end + termlen + (*content.end == '\n' ? 1 : 0);
    //         } else {
    //             fprintf(stderr, "expand: unrecognized directive ");
    //             for (byte *cp = directive.ptr; cp < directive.end; cp++) {
    //                 fprintf(stderr, "%c", *cp);
    //             }
    //             fprintf(stderr, "\n");
    //             assert(0);
    //         }
    //     } else if (b == '/') {
    //         Span comment = {srcp, srcp + 1};
    //
    //         if (comment.end < sp.end) {
    //             if (*comment.end == '/') {
    //                 for (; comment.end < sp.end && *comment.end != '\n' ; comment.end++)
    //                     ;
    //             } else if (*comment.end == '*') {
    //                 for (; comment.end+1 < sp.end && (*comment.end != '*' || *(comment.end+1) != '/'); comment.end++)
    //                     ;
    //
    //                 comment.end += 2;
    //             }
    //         }
    //
    //         for (byte *cp = comment.ptr; cp < sp.end && cp < comment.end; cp++) {
    //             *outp++ = (char) *cp;
    //             *outsz -= 1;
    //         }
    //         srcp = comment.end;
    //     } else if (b == '"' || b == '\'') {
    //         Span literal = {srcp, srcp + 1};
    //
    //         for (; literal.end < sp.end; literal.end++) {
    //             if (*literal.end == '\\') {
    //                 ++literal.end;
    //             } else if (*literal.end == *literal.ptr) {
    //                 break;
    //             }
    //         }
    //
    //         for (byte *cp = literal.ptr; cp < sp.end && cp < literal.end; cp++) {
    //             *outp++ = (char) *cp;
    //             *outsz -= 1;
    //         }
    //         srcp = literal.end;
    //     } else if (isalpha(b)) {
    //         Span id = {srcp, srcp};
    //         for ( ; srcp < sp.end && isalnum(*srcp) || *srcp == '_'; srcp++) {
    //             id.end++;
    //         }
    //
    //         Span *repl = (Span *) prep_define_get(def_table, id);
    //
    //         if (repl == NULL) {
    //             for ( ; id.ptr < id.end; id.ptr++) {
    //                 *outp++ = (char) *id.ptr;
    //                 *outsz -= 1;
    //             }
    //         } else {
    //             for (byte *bp = repl->ptr; bp < repl->end; bp++) {
    //                 *outp++ = (char) *bp;
    //                 *outsz -= 1;
    //             }
    //         }
    //     } else {
    //         *outp++ = (char) b;
    //         *outsz -= 1;
    //     }
    // }

    return outp;
}

enum tokenType {
    PREP_UNKNOWN_TOKEN,
    PREP_PLUS_TOKEN,
    PREP_IDENTIFIER_TOKEN,
    PREP_DEFINED_TOKEN,
    PREP_OPEN_PAREN_TOKEN, PREP_CLOSE_PAREN_TOKEN,
    PREP_AND_TOKEN
};

struct prepToken {
    enum tokenType type;
    Span span;
};

static struct prepToken tokens[128];
static int stack[64];
static int stackTop = 0;

static void push(int x) {
    assert(stackTop < 64);
    stack[stackTop++] = x;
}

static int pop() {
    assert(stackTop > 0);
    return stack[--stackTop];
}

static struct prepToken *eval(DefineTable *def_table, struct prepToken *tokenp, struct prepToken *end_token);

static bool eval_expr(Span expr, DefineTable *def_table) {
    byte *p;
    struct prepToken *tp = tokens;

    for (p = expr.ptr; p < expr.end;) {
        if (*p == '+') {
            *tp++ = (struct prepToken) {PREP_PLUS_TOKEN, p, p + 1};
        } else if (*p == '&') {
            struct prepToken tok = {PREP_AND_TOKEN, p, p};
            p++;
            assert(*p == '&');
            p++;
            tok.span.end = p;
            *tp++ = tok;
        } else if (isalpha(*p) || *p == '_') {
            struct prepToken tok = {PREP_IDENTIFIER_TOKEN, p, p};
            for ( ; isalnum(*p) || *p == '_' ; p++, tok.span.end++)
                ;

            if (spanstrcmp(tok.span, "DEFINED") == 0 || spanstrcmp(tok.span, "defined") == 0) tok.type = PREP_DEFINED_TOKEN;

            *tp++ = tok;
        } else if (isspace(*p)) {
            p++;
        } else {
            assert(0);
        }
    }

    struct prepToken *end_token = tp;

    for (struct prepToken *t = tokens; t < end_token; ) {
        t = eval(def_table, t, end_token);
    }
    return (bool) pop();
}

static struct prepToken *eval(DefineTable *def_table, struct prepToken *tokenp, struct prepToken *end_token) {
    switch (tokenp->type) {
        case PREP_DEFINED_TOKEN: {
            push(prep_define_get(def_table, (tokenp + 1)->span) != NULL);
            tokenp += 2;
        } break;
        case PREP_AND_TOKEN: {
            tokenp = eval(def_table, tokenp + 1, end_token);
            push(pop() && pop());
        } break;
        default: {
            assert(0);
        } break;
    }

    return tokenp;
}