#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "html_reader.h"

#include <assert.h>

#define TAG_MAX_LEN 256
struct HtmlReader {
    FILE *filep;
    char tag[TAG_MAX_LEN];
};

struct HtmlReader *html_new_reader(FILE *filep) {
    struct HtmlReader *reader = malloc(sizeof(struct HtmlReader));
    reader->filep = filep;
    return reader;
}

void html_close_reader(HtmlReader *r) {
    free(r);
}

void skip_spaces(HtmlReader *r) {
    int c;
    while ((c = getc(r->filep)) != EOF && isspace(c))
        ;

    if (c != EOF) ungetc(c, r->filep);
}

void read_tag(HtmlReader *r) {
    char *cp = r->tag;
    int first = getc(r->filep);

    if (isalpha(first)) {
        *cp++ = (char) first;

        int c;
        while ((c = getc(r->filep)) != EOF && (isalnum(c) || c == '_') && cp < r->tag + TAG_MAX_LEN - 1) {
            *cp++ = (char) c;
        }
        if (c != EOF) ungetc(c, r->filep);
        *cp = '\0';
    }
}

void skip(HtmlReader *r, char t) {
    int c = getc(r->filep);
    assert(c == EOF || (char) c == t);
}

void skip_until(HtmlReader *r, char t) {
    int c;
    while ((c = getc(r->filep)) != EOF && c != t)
        ;

    if (c != EOF) ungetc(c, r->filep);
}

HtmlRecord *html_next_tag(HtmlReader *r) {
    skip_spaces(r);
    int c = getc(r->filep);

    if (c == '<') {
        int next = getc(r->filep);

        if (isalpha(next)) { // start of a tag
            ungetc(next, r->filep);
        } else if (next == '!') { // doctype
            skip_until(r, '<');
            skip(r, '<');
        }

        read_tag(r);
        skip_spaces(r);
    }
}

char *html_read_content(HtmlReader *r) {

}