#include <stdio.h>

#include "common.h"

struct HtmlHandle {

};

struct HtmlHandle *html_new(FILE *) {
    struct HtmlHandle *h = pool_alloc_struct(struct HtmlHandle);
}

void html_close(struct HtmlHandle *) {

}

void html_open_tag(char *tag) {

}
void html_close_tag() {

}

void html_add_attr(char *name, char *value) {

}

void html_write_text(char *text) {

}

