#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "lexer.h"

static char *no_close_tags[] = {
    "br", "link", "meta"
};

#define NO_CLOSE_TAGS_SIZE (sizeof(no_close_tags) / sizeof(no_close_tags[0]))

typedef struct attr {
    char *name;
    char *value;
    bool isflag;
    struct attr *next;
} Attr;

typedef struct {
    char *name;
    Attr *tail_attr;
    bool open_tag_written;
} Tag;

struct HtmlHandle {
    FILE *filep;
    Tag *stack;
    size_t stack_size;
    Tag *stack_top;
};

static Tag *push(struct HtmlHandle *h) {
    assert(h->stack_top < h->stack + h->stack_size);
    return h->stack_top++;
}

static Tag *peek(struct HtmlHandle *h) {
    assert(h->stack_top > h->stack);
    return h->stack_top - 1;
}
static Tag *pop(struct HtmlHandle *h) {
    assert(h->stack_top > h->stack);
    return --h->stack_top;
}

static bool is_empty(struct HtmlHandle *h) {
    return h->stack_top == h->stack;
}

static void write_open_tag(struct HtmlHandle *h, Tag *tag) {
    fprintf(h->filep, "<%s", tag->name);

    Attr *head = tag->tail_attr->next;
    char *first_sep = " ", *sep = "";

    // TODO: Attribute escaping
    for (Attr *attr = head->next; attr != head; attr = attr->next) {
        fprintf(h->filep, "%s", first_sep);
        fprintf(h->filep, "%s", sep);

        if (attr->isflag) {
            fprintf(h->filep, "%s", attr->name);
        } else {
            fprintf(h->filep, "%s=\"%s\"", attr->name, attr->value);
        }

        first_sep = "";
        sep = " ";
    }

    fprintf(h->filep, ">");
    tag->open_tag_written = true;
}

struct HtmlHandle *html_new(FILE *fp) {
    struct HtmlHandle *h = pool_alloc_struct(struct HtmlHandle);
    h->filep = fp;
    h->stack_size = 32;
    h->stack = pool_alloc(sizeof(Tag) * h->stack_size, Tag);
    h->stack_top = h->stack;
    return h;
}

void html_close(struct HtmlHandle *h) {
    // TODO: Check all tags closed
    fclose(h->filep);
}

void html_open_tag(struct HtmlHandle *h, char *tag_name) {
    if (!is_empty(h)) { // Write previous header
        Tag *tag = peek(h);
        if (!tag->open_tag_written) write_open_tag(h, peek(h));
    }

    Tag *tag = push(h);
    tag->name = pool_alloc_copy_str(tag_name);

    tag->tail_attr = pool_alloc_struct(Attr);
    tag->tail_attr->next = tag->tail_attr;

    tag->open_tag_written = false;
}

void html_close_tag(struct HtmlHandle *h) {
    Tag *tag = pop(h);
    if (!tag->open_tag_written) write_open_tag(h, tag);

    if (binsearch(tag->name, no_close_tags, NO_CLOSE_TAGS_SIZE) < 0) {
        fprintf(h->filep, "</%s>", tag->name);
    }
}

static Attr *alloc_add_attr(struct HtmlHandle *h) {
    Tag *tag = peek(h);

    Attr *attr = pool_alloc_struct(Attr);

    Attr *head = tag->tail_attr->next;
    tag->tail_attr->next = attr;
    attr->next = head;
    tag->tail_attr = attr;
    return attr;
}

void html_add_attr(struct HtmlHandle *h, char *name, char *value) {
    Attr *attr = alloc_add_attr(h);
    attr->name = pool_alloc_copy_str(name);
    attr->value = pool_alloc_copy_str(value);
}

void html_add_flag(struct HtmlHandle *h, char *name) {
    Attr *attr = alloc_add_attr(h);
    attr->name = pool_alloc_copy_str(name);
    attr->isflag = true;
}

void html_write_text_raw(struct HtmlHandle *h, char *text) {
    Tag *tag = peek(h);
    if (!tag->open_tag_written) write_open_tag(h, tag);

    fwrite(text, strlen(text), 1, h->filep);
}

void html_write_token(struct HtmlHandle *h, Token *t) {
    Tag *tag = peek(h);
    if (!tag->open_tag_written) write_open_tag(h, tag);

    for (byte *cp = t->span.ptr; cp < t->span.end; cp++) {
        char c = (char) *cp;

        switch (c) {
            case ' ': {
                html_write_text_raw(h, "&nbsp;");
            } break;
            case '\n': {
                html_open_tag(h, "br");
                html_close_tag(h);
            } break;
            case '<': {
                html_write_text_raw(h, "&lt;");
            } break;
            case '>': {
                html_write_text_raw(h, "&gt;");
            } break;
            default: {
                fputc(c, h->filep);
            } break;
        }
    }
}

void html_add_doctype(struct HtmlHandle *h) {
    fprintf(h->filep, "<!DOCTYPE html>\n");
}
