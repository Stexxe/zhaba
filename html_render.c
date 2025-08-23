#include <stdio.h>
#include "parser.h"
#include "html_writer.h"

void gen_html(NodeHeader *node, FILE *filep) {
    HtmlHandle *html = html_new(filep);

    html_open_tag(html, "html");
    html_close_tag(html);

    html_close(html);
}
