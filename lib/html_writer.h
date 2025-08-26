#ifndef ZHABA_HTML_WRITER_H
#define ZHABA_HTML_WRITER_H

#include <stdio.h>

typedef struct HtmlHandle HtmlHandle;

HtmlHandle *html_new(FILE *);
void html_close(HtmlHandle *);
void html_open_tag(HtmlHandle *, char *tag);
void html_close_tag(HtmlHandle *);
void html_add_attr(HtmlHandle *, char *name, char *value);
void html_add_flag(HtmlHandle *, char *name);
void html_write_text_raw(HtmlHandle *, char *text);
void html_write_textf(HtmlHandle *, char *fmt, ...);
void html_write_token(HtmlHandle *, Token *);
void html_add_doctype(HtmlHandle *);

#endif //ZHABA_HTML_WRITER_H