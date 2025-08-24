#include <stdio.h>
#include "parser.h"
#include "html_writer.h"

void gen_html(NodeHeader *node, FILE *filep) {
    HtmlHandle *html = html_new(filep);

    html_add_doctype(html);
    html_open_tag(html, "html");
        html_add_attr(html, "lang", "en");
        html_open_tag(html, "head");
            html_open_tag(html, "meta");
                html_add_attr(html, "charset", "UTF-8");
            html_close_tag(html);
            html_open_tag(html, "title");
            html_write_text(html,  "hello.c"); // TODO: Replace with actual filename
            html_close_tag(html);
        html_close_tag(html);
    html_close_tag(html);

    html_close(html);
}
