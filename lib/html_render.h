#ifndef ZHABA_HTML_RENDER_H
#define ZHABA_HTML_RENDER_H

#include <stdio.h>
#include "parser.h"

void gen_html(NodeHeader *node, char *filename, int nlines, FILE *);

#endif //ZHABA_HTML_RENDER_H
