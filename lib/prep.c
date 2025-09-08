#include "prep.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "parser.h"

typedef struct DefineKv {
    Span key;
    NodeHeader *expr;
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

static DefineKv *new_kv(Span key, NodeHeader *expr) {
    DefineKv *kv = pool_alloc_struct(DefineKv);
    kv->key = key;
    kv->expr = expr;
    kv->next = NULL;
    return kv;
}

void prep_define_set(DefineTable *table, Span key, NodeHeader *expr) {
    uint h = hash(key, table->size);
    DefineKv *kv = table->ptr[h];

    if (kv == NULL) {
        table->ptr[h] = new_kv(key, expr);
    } else {
        DefineKv *head = kv;
        DefineKv *prev_found = NULL, *found = NULL;

        for (kv = head; kv != NULL; prev_found = kv, kv = kv->next) {
            if (spancmp(kv->key, key) == 0) {
                found = kv;
                break;
            }
        }

        DefineKv *nkv = new_kv(key, expr);

        if (found) {
            nkv->next = found->next;
            if (prev_found) prev_found->next = nkv;
        } else {
            nkv->next = head;
            table->ptr[h] = nkv;
        }
    }
}

NodeHeader *prep_define_get(DefineTable *table, Span key) {
    uint h = hash(key, table->size);
    DefineKv *kv = table->ptr[h];

    for ( ; kv != NULL; kv = kv->next) {
        if (spancmp(kv->key, key) == 0) return kv->expr;
    }

    return NULL;
}

typedef struct {
    char *directive;
    void (*expand)(FILE *, char *);
} Expander;

#define MAX_BUF_SIZE 1024
static char buf[MAX_BUF_SIZE];

static void expand_nothing(FILE* srcfp, char *out) {}

static void expand_include(FILE* srcfp, char *out) {
    int c;
    while ((c = getc(srcfp)) != EOF && isspace(c))
        ;

    if (c == '"') {
        char *local_path = buf;
        while ((c = getc(srcfp)) != EOF && c != '"') {
            *local_path++ = (char) c;
        }

        *local_path = '\0';

        while ((c = getc(srcfp)) != EOF && isspace(c) && c != '\n')
            ;

        // Get srcfile dir
        // Build path to the file
        // Read file
        // Write to out in a loop
    }
}

// TODO: Sort once
static Expander prep_directives[] = {
    (Expander) {"define", expand_nothing},
    (Expander) {"elif", expand_nothing},
    (Expander) {"else", expand_nothing},
    (Expander) {"endif", expand_nothing},
    (Expander) {"error", expand_nothing},
    (Expander) {"if", expand_nothing},
    (Expander) {"ifdef", expand_nothing},
    (Expander) {"ifndef", expand_nothing},
    (Expander) {"include", expand_include},
    (Expander) {"line", expand_nothing},
    (Expander) {"pragma", expand_nothing},
    (Expander) {"undef", expand_nothing},
};

#define PREP_DIRECTIVES_SIZE (sizeof(prep_directives) / sizeof(prep_directives[0]))

static int binsearch_lex_span(char *target, Expander *arr, size_t size) {
    int low = 0;
    int high = (int) size - 1;
    int comp;

    while (low <= high) {
        int mid = (low + high) / 2;

        if ((comp = strcmp(target, arr[mid].directive)) < 0) {
            high = mid - 1;
        } else if (comp > 0) {
            low = mid + 1;
        } else {
            return mid;
        }
    }

    return -1;
}

char *prep_expand(char *srcfile, char *out, int outsz) {
    FILE *srcfp = fopen(srcfile, "r");

    if (!srcfp) return NULL; // TODO: Error handling

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

    // int outsz = 2 * 1024 * 1024;
    // char *out = pool_alloc(outsz, char);
    char *outp = out;

    int c, i;
    while ((c = getc(srcfp)) != EOF) {
        if (c == '#') {
            char *directive = buf;
            while ((c = getc(srcfp)) != EOF && !isspace(c)) {
                *directive++ = (char) c;
            }

            *directive = '\0';

            if (strcmp(buf, "include") == 0) {
                while ((c = getc(srcfp)) != EOF && isspace(c))
                    ;

                if (c == '"') {
                    char *local_path = buf;
                    while ((c = getc(srcfp)) != EOF && c != '"') {
                        *local_path++ = (char) c;
                    }

                    *local_path = '\0';

                    while ((c = getc(srcfp)) != EOF && isspace(c) && c != '\n')
                        ;

                    char *inc_path = path_join(2, dirpath, buf);

                    prep_expand(inc_path, outp, outsz);
                    assert(outsz >= 0);
                }
            }

            // if ((i = binsearch_lex_span(buf, prep_directives, PREP_DIRECTIVES_SIZE)) >= 0) {
            //     if (c != EOF) ungetc(c, srcfp);
            //     prep_directives[i].expand(srcfp);
            // }
        } else { // TODO: Handle string, char literals, comments
            *outp++ = (char) c;
            outsz--;
        }
    }

    return out;
}