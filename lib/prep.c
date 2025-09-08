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

    char *outp = out;
    byte *srcp = src;

    while (srcp < src + srcsize) {
        if (*srcp == '#') {
            ++srcp;
            Span directive = {srcp, srcp};

            for ( ; srcp < src + srcsize && !isspace(*srcp) ; srcp++) {
                directive.end++;
            }

            if (spanstrcmp(directive, "include") == 0) {
                for ( ; srcp < src + srcsize && isspace(*srcp); srcp++)
                    ;

                if (*srcp == '"') {
                    ++srcp;
                    Span local_path = {srcp, srcp};

                    for ( ; srcp < src + srcsize && *srcp != '"'; srcp++) {
                        local_path.end++;
                    }

                    assert(*srcp == '"');
                    ++srcp;

                    for ( ; srcp < src + srcsize && isspace(*srcp) && *srcp != '\n'; srcp++)
                        ;

                    if (srcp < src + srcsize && *srcp == '\n') srcp++;

                    char *inc_path = path_join_ssp(dirpath, local_path);

                    prep_expand(inc_path, def_table, outp, outsz);
                    assert(*outsz >= 0);
                }
            } else if (spanstrcmp(directive, "define") == 0) {
                for ( ; srcp < src + srcsize && isspace(*srcp); srcp++)
                    ;

                Span id = {srcp, srcp};

                for ( ; srcp < src + srcsize && !isspace(*srcp) ; srcp++) {
                    id.end++;
                }

                for ( ; srcp < src + srcsize && isspace(*srcp); srcp++)
                    ;

                Span *content = pool_alloc_struct(Span);
                content->ptr = content->end = srcp;
                for ( ; srcp < src + srcsize && !isspace(*srcp) ; srcp++) {
                    content->end++;
                }
                for ( ; srcp < src + srcsize && isspace(*srcp) && *srcp != '\n'; srcp++)
                    ;

                if (srcp < src + srcsize && *srcp == '\n') srcp++;

                prep_define_set(def_table, id, content);
            } else {
                fprintf(stderr, "expand: unrecognized directive ");
                for (byte *cp = directive.ptr; cp < directive.end; cp++) {
                    fprintf(stderr, "%c", *cp);
                }
                fprintf(stderr, "\n");
                assert(0);
            }
        } else if (isalpha(*srcp)) {
            Span id = {srcp, srcp};
            for ( ; srcp < src + srcsize && isalnum(*srcp) || *srcp == '_'; srcp++) {
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

    return out;
}