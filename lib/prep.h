#ifndef ZHABA_PREP_H
#define ZHABA_PREP_H

#include "common.h"
#include "parser.h"

typedef struct DefineTable DefineTable;

void prep_define_set(DefineTable *table, Span key, NodeHeader *expr);
NodeHeader *prep_define_get(DefineTable *table, Span key);
DefineTable *prep_define_newtable();
char *prep_expand(char *srcfile);


#endif //ZHABA_PREP_H