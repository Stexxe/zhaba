#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

typedef struct attr {
    char *name;
    char *value;
    struct attr *next;
} Attr;

typedef struct {
    char *name;
    char *text_buffer;
    char *text_buffer_end;
    size_t text_buffer_size;
    Attr *first_attr;
} Tag;

struct HtmlHandle {
    FILE *filep;
    Tag *stack;
    size_t stack_size;
    Tag *stack_top;
};

static void write_open_tag(struct HtmlHandle *h, Tag *tag) {
    fprintf(h->filep, "<%s>", tag->name);
    // TODO: Write attributes
    fwrite(tag->text_buffer, tag->text_buffer_end - tag->text_buffer, 1, h->filep);
}

struct HtmlHandle *html_new(FILE *fp) {
    struct HtmlHandle *h = pool_alloc_struct(struct HtmlHandle);
    h->filep = fp;
    h->stack_size = 128;
    h->stack = pool_alloc(sizeof(Tag) * h->stack_size, Tag);
    h->stack_top = h->stack;
    return h;
}

void html_close(struct HtmlHandle *h) {
    // TODO: Check all tags closed

    fclose(h->filep);
}

void html_open_tag(struct HtmlHandle *h, char *tag_name) {
    if (h->stack_top > h->stack) { // Write previous header
        write_open_tag(h, h->stack_top - 1);
    }

    assert(h->stack_top <= h->stack + h->stack_size);
    Tag *tag = h->stack_top;
    tag->name = pool_alloc(strlen(tag_name) + 1, char);
    strcpy(tag->name, tag_name);
    tag->text_buffer_size = 1024;
    tag->text_buffer = malloc(sizeof(char) * tag->text_buffer_size);
    assert(tag->text_buffer != NULL);
    tag->text_buffer_end = tag->text_buffer;
    tag->first_attr = NULL;

    // fwrite(tag->text_buffer, sizeof(char), tag->text_buffer_size, h->filep);

    // h->stack_top = tag;
    h->stack_top++;
}

void html_close_tag(struct HtmlHandle *h) {
    assert(h->stack_top > h->stack);
    Tag *tag = h->stack_top - 1;

    if (tag == h->stack) {
        write_open_tag(h, tag);
    }

    fprintf(h->filep, "</%s>", tag->name);

    free(tag->text_buffer);
    h->stack_top--;
}

void html_add_attr(struct HtmlHandle *h, char *name, char *value) {

}

void html_write_text(struct HtmlHandle *h, char *text) {

}
