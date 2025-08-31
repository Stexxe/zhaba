#include "prep.h"

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