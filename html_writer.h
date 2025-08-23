#ifndef ZHABA_HTML_WRITER_H
#define ZHABA_HTML_WRITER_H

#include <stdio.h>

typedef struct HtmlHandle HtmlHandle;

HtmlHandle *html_new(FILE *);
void html_close(HtmlHandle *);
void html_open_tag(HtmlHandle *, char *tag);
void html_close_tag(HtmlHandle *);
void html_add_attr(HtmlHandle *, char *name, char *value);
void html_write_text(HtmlHandle *, char *text);

#endif //ZHABA_HTML_WRITER_H