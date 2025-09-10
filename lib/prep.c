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

// TODO: Combine out and outsz
static char *expand(Span sp, char *dirpath, DefineTable *def_table, char *out, int *outsz) {
    char *outp = out;
    assert(*outsz > 0);

    for (byte *srcp = sp.ptr; srcp < sp.end;) {
        if (*srcp == '#') {
            ++srcp;
            for ( ; srcp < sp.end && isspace(*srcp); srcp++)
                ;

            Span directive = {srcp, srcp};

            for ( ; srcp < sp.end && !isspace(*srcp) ; srcp++) {
                directive.end++;
            }

            if (spanstrcmp(directive, "include") == 0) {
                for ( ; srcp < sp.end && isspace(*srcp); srcp++)
                    ;

                if (*srcp == '"') {
                    ++srcp;
                    Span local_path = {srcp, srcp};

                    for ( ; srcp < sp.end && *srcp != '"'; srcp++) {
                        local_path.end++;
                    }

                    assert(*srcp == '"');
                    ++srcp;

                    for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
                        ;

                    if (srcp < sp.end && *srcp == '\n') srcp++;

                    char *inc_path = path_join_ssp(dirpath, local_path);

                    outp = prep_expand(inc_path, def_table, outp, outsz);
                } else if (*srcp == '<') {
                    ++srcp;
                    Span ext_path = {srcp, srcp};

                    for ( ; srcp < sp.end && *srcp != '>'; srcp++) {
                        ext_path.end++;
                    }

                    assert(*srcp == '>');
                    ++srcp;

                    for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
                        ;

                    if (srcp < sp.end && *srcp == '\n') srcp++;

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
                for ( ; srcp < sp.end && isspace(*srcp); srcp++)
                    ;

                Span id = {srcp, srcp};

                for ( ; srcp < sp.end && !isspace(*srcp) ; srcp++) {
                    id.end++;
                }

                for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
                    ;

                Span *content = pool_alloc_struct(Span);
                if (*srcp == '\n') {
                    content->ptr = content->end = NULL;
                    srcp = min(srcp + 1, sp.end);
                } else {
                    content->ptr = content->end = srcp;
                    for ( ; srcp < sp.end && !isspace(*srcp) ; srcp++) {
                        content->end++;
                    }
                    for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
                        ;

                    if (srcp < sp.end && *srcp == '\n') srcp++;
                }

                prep_define_set(def_table, id, content);
            } else if (spanstrcmp(directive, "ifdef") == 0 || spanstrcmp(directive, "ifndef") == 0) {
                for ( ; srcp < sp.end && isspace(*srcp); srcp++)
                    ;

                Span id = {srcp, srcp};

                for ( ; srcp < sp.end && !isspace(*srcp) ; srcp++) {
                    id.end++;
                }

                for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
                    ;

                if (srcp < sp.end && *srcp == '\n') srcp++;

                Span content = {srcp, srcp};

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

                srcp = content.end + termlen + (*content.end == '\n' ? 1 : 0);
            } else if (spanstrcmp(directive, "undef") == 0) {
                for ( ; srcp < sp.end && isspace(*srcp); srcp++)
                    ;

                Span id = {srcp, srcp};

                for ( ; srcp < sp.end && !isspace(*srcp) ; srcp++) {
                    id.end++;
                }

                for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
                    ;

                if (srcp < sp.end && *srcp == '\n') srcp++;

                prep_define_set(def_table, id, NULL);
            } else if (spanstrcmp(directive, "if") == 0) {
                for ( ; srcp < sp.end && isspace(*srcp); srcp++)
                    ;

                Span expr = {srcp, srcp};

                for ( ; srcp < sp.end && *srcp != '\n'; srcp++) {
                    expr.end++;
                }

                for ( ; srcp < sp.end && isspace(*srcp) && *srcp != '\n'; srcp++)
                    ;

                if (srcp < sp.end && *srcp == '\n') srcp++;
                Span content = {srcp, srcp};

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

                srcp = content.end + termlen + (*content.end == '\n' ? 1 : 0);
            } else {
                fprintf(stderr, "expand: unrecognized directive ");
                for (byte *cp = directive.ptr; cp < directive.end; cp++) {
                    fprintf(stderr, "%c", *cp);
                }
                fprintf(stderr, "\n");
                assert(0);
            }
        } else if (*srcp == '/') {
            Span comment = {srcp, srcp + 1};

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
            srcp = comment.end;
        } else if (*srcp == '"' || *srcp == '\'') {
            Span literal = {srcp, srcp + 1};

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
            srcp = literal.end;
        } else if (isalpha(*srcp)) {
            Span id = {srcp, srcp};
            for ( ; srcp < sp.end && isalnum(*srcp) || *srcp == '_'; srcp++) {
                id.end++;
            }

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
            *outp++ = (char) *srcp++;
            *outsz -= 1;
        }
    }

    return outp;
}

enum tokenType {
    PREP_UNKNOWN_TOKEN,
    PREP_PLUS_TOKEN,
    PREP_IDENTIFIER_TOKEN,
    PREP_DEFINED_TOKEN,
    PREP_OPEN_PAREN_TOKEN, PREP_CLOSE_PAREN_TOKEN
};

struct prepToken {
    enum tokenType type;
    Span span;
};

enum nodeType {
    PREP_UNKNOWN_NODE,
    PREP_NODE_DEFINED
};

struct prepNode {
    enum nodeType type;
    struct prepToken *start_token;
    struct prepToken *end_token;
};

struct definedNode {
    struct prepNode header;
    struct prepToken *id;
};

static struct prepToken tokens[128];

static bool eval_expr(Span expr, DefineTable *def_table) {
    byte *p;
    struct prepToken *tp = tokens;

    for (p = expr.ptr; p < expr.end;) {
        if (*p == '+') {
            *tp++ = (struct prepToken) {PREP_PLUS_TOKEN, p, p + 1};
        } else if (isalpha(*p)) {
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
    struct prepToken *binop = NULL;
    for (struct prepToken *t = tokens; t < end_token; t++) {
        if (t->type == PREP_PLUS_TOKEN) { // Any binary op token
            binop = t;
            break;
        }
    }

    struct prepNode *root_node = NULL;

    if (binop == NULL) {
        struct prepToken *t = tokens;
        struct prepNode *node;
        switch (t->type) {
            case PREP_DEFINED_TOKEN: {
                struct definedNode *n = pool_alloc_struct(struct definedNode);
                n->header = (struct prepNode) {PREP_NODE_DEFINED, t};
                if (t->type == PREP_OPEN_PAREN_TOKEN) t++;
                t++;
                assert(t->type == PREP_IDENTIFIER_TOKEN);
                n->id = t;
                t++;
                if (t->type == PREP_CLOSE_PAREN_TOKEN) t++;

                n->header.end_token = t;
                node = (struct prepNode *) n;
            } break;
            default: {
                assert(0);
            } break;
        }

        if (root_node == NULL) root_node = node;
    } else {
        // TODO: Handle binary op
    }

    bool res = false;

    if (root_node) {
        switch (root_node->type) {
            case PREP_NODE_DEFINED: {
                struct definedNode *n = (struct definedNode *) root_node;
                res = prep_define_get(def_table, n->id->span) != NULL;
            } break;
            default: {
                assert(0);
            } break;
        }
    }

    return res;
}