#ifndef ZHABA_HTML_WRITER_H
#define ZHABA_HTML_WRITER_H

#include <stdio.h>

typedef struct HtmlHandle HtmlHandle;

HtmlHandle *html_new(FILE *);
void html_close(HtmlHandle *);
void html_open_tag(char *tag);
void html_close_tag();
void html_add_attr(char *name, char *value);
void html_write_text(char *text);

#endif //ZHABA_HTML_WRITER_H