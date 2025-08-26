#ifndef ZHABA_HTML_READER_H
#define ZHABA_HTML_READER_H
#include <stdio.h>

typedef struct HtmlReader HtmlReader;

typedef struct {
    char *tag;
    char *class;
} HtmlRecord;

HtmlReader *html_new_reader(FILE *filep);
void html_close_reader(HtmlReader *);
HtmlRecord *html_next_tag(HtmlReader *);
char *html_read_content(HtmlReader *);

#endif //ZHABA_HTML_READER_H