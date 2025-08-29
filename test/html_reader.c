#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "html_reader.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "../lib/common.h"

#define TAG_MAX_LEN 256

struct HtmlReader {
    FILE *filep;
    char tag[TAG_MAX_LEN];
    char attr_name[TAG_MAX_LEN];
    char attr_value[TAG_MAX_LEN];
    char class[TAG_MAX_LEN];
    bool attr_is_flag;
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

void skip(HtmlReader *r, char t) {
    int c = getc(r->filep);
    assert(c == EOF || (char) c == t);
}

static bool is_attr_char(int c) {
    return !isspace(c) && c != '"' && c != '\'' && c != '>' && c != '=';
}

static void read_attr(HtmlReader *r) {
    char *namep = r->attr_name;
    int c;

    while ((c = getc(r->filep)) != EOF && is_attr_char(c)) {
        *namep++ = (char) c;
    }

    if (c != EOF) ungetc(c, r->filep);
    *namep = '\0';

    skip_spaces(r);

    c = getc(r->filep);

    if (c == '>' && namep == r->attr_name) { // No attributes
        ungetc(c, r->filep);
        return;
    } else if (c == '>' || isspace(c)) { // Attribute without value
        ungetc(c, r->filep);
        r->attr_is_flag = true;
        return;
    }

    r->attr_is_flag = false;

    assert(c == '=');
    skip_spaces(r);

    int open_quote = getc(r->filep);
    assert(open_quote == '"' || open_quote == '\'');

    char *valuep = r->attr_value;
    while ((c = getc(r->filep)) != EOF && c != open_quote) {
        *valuep++ = (char) c;
    }

    if (c != EOF) {
        assert(c == open_quote);
    }

    *valuep = '\0';
}

static int read_attrs(HtmlReader *r) {
    int c;

    r->class[0] = '\0';

    do {
        skip_spaces(r);
        read_attr(r);

        if (strcmp("class", r->attr_name) == 0) {
            strcpy(r->class, r->attr_value);
        }

        skip_spaces(r);
        c = getc(r->filep);
    } while (c != '>' && c != EOF);

    return c;
}

static int read_tag(HtmlReader *r) {
    char *tagp = r->tag;
    int c = getc(r->filep);

    if (isalpha(c)) {
        *tagp++ = (char) c;

        while ((c = getc(r->filep)) != EOF && (isalnum(c) || c == '_') && tagp < r->tag + TAG_MAX_LEN - 1) {
            *tagp++ = (char) c;
        }
        if (c != EOF) ungetc(c, r->filep);
        *tagp = '\0';

        skip_spaces(r);
        c = read_attrs(r);
    }

    return c;
}

void skip_until(HtmlReader *r, char t) {
    int c;
    while ((c = getc(r->filep)) != EOF && c != t)
        ;

    if (c != EOF) ungetc(c, r->filep);
}

HtmlRecord html_next_tag(HtmlReader *r) {
    int c;
    while ((c = getc(r->filep)) != EOF && c != '<')
        ;

    if (c == '<') {
        int next = getc(r->filep);

        if (isalpha(next)) { // start of a tag
            ungetc(next, r->filep);
        } else if (next == '!' || next == '/') { // doctype or closed tag
            skip_until(r, '<');
            skip(r, '<');
        }

        c = read_tag(r);
    }

    HtmlRecord record = {.tag = r->tag, .class = r->class, c == EOF};
    return record;
}

typedef enum {
    Text,
    OpenTag,
    CloseTag,
    AttrValue // TODO: Use
} ReadState;

static char *no_close_tags[] = {
    "br", "link", "meta"
};

#define NO_CLOSE_TAGS_SIZE (sizeof(no_close_tags) / sizeof(no_close_tags[0]))

void html_read_content(HtmlReader *r, char *buf, int maxlen) {
    int count = 1;
    int c;
    ReadState state = Text;

    char *open_tag = NULL;
    while ((c = getc(r->filep)) != EOF && count > 0 && --maxlen > 0) {
        *buf++ = (char) c;
        if (state == Text && c == '<') {
            int next = getc(r->filep);
            --maxlen;

            *buf++ = (char) next;

            if (next == '/') {
                char *closed_tag = buf;
                while ((c = getc(r->filep)) != EOF && c != '>') {
                    --maxlen;
                    *buf++ = (char) c;
                }

                if (c == '>') {
                    *buf++ = (char) c;
                }

                *(buf - 1) = '\0';

                count--;
                state = Text;
                if (strcmp(r->tag, closed_tag) == 0 && count == 0) {
                    buf = closed_tag - 2;
                    *buf = '\0';
                    break;
                }

                *(buf - 1) = (char) c;
            } else if (isalpha(next)) {
                open_tag = buf - 1;
                state = OpenTag;
                count++;
            }
        } else if (state == OpenTag) {
            if (c == '>' || c == ' ') {
                char old = *(buf-1);
                *(buf-1) = '\0';
                if (binsearch(open_tag, no_close_tags, NO_CLOSE_TAGS_SIZE) >= 0) {
                    count--;
                }

                *(buf-1) = old;
                state = Text;
            }
        }
    }

    *buf = '\0';
}