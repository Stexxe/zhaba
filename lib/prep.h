#ifndef ZHABA_PREP_H
#define ZHABA_PREP_H

#include "common.h"
#include "parser.h"

typedef struct DefineTable DefineTable;

void prep_define_set(DefineTable *table, Span key, void *value);
void *prep_define_get(DefineTable *table, Span key);
DefineTable *prep_define_newtable();
char *prep_expand(char *srcfile, DefineTable *def_table, char *out, int *outsz);
void prep_search_paths_set(char **, size_t);

#endif //ZHABA_PREP_H