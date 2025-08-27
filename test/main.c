#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <asm-generic/errno-base.h>
#include <sys/stat.h>

#include "html_reader.h"
#include "../lib/lib.h"

static char *path_ext(char *p);
static char *path_join(char *p1, char *p2);
static char *path_replace_ext(char *p, char *ext);

#define SOURCE_MAX_LEN 1024
static char source[SOURCE_MAX_LEN];

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s cases-dir\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    DIR *dir = opendir(argv[1]);

    if (!dir) {
        fprintf(stderr, "Could not open %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    struct dirent *ent;
    char *outdir = "temp";
    int direrr = mkdir(outdir, 0777);
    assert(direrr == 0 || errno == EEXIST);

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        assert(ent->d_type == DT_REG);

        if (strcmp(path_ext(ent->d_name), ".c") == 0) {
            char *srcpath = path_join(argv[1], ent->d_name);
            RenderError err;
            RenderErrorType res = render(srcpath, outdir, &err);

            if (res < 0) {
                fprintf(stderr, "render: unexpected error %d\n", res);
                exit(EXIT_FAILURE);
            }

            char *htmlpath = path_join(outdir, path_replace_ext(ent->d_name, ".html"));

            FILE *htmlfp = fopen(htmlpath, "r");
            assert(htmlfp != NULL);

            HtmlReader *hr = html_new_reader(htmlfp);

            HtmlRecord record;
            do {
                record = html_next_tag(hr);

                if (strcmp("source", record.class) == 0) {
                    html_read_content(hr, source, SOURCE_MAX_LEN);
                }

            } while (!record.eof);

            html_close_reader(hr);
        }
    }

    return 0;
}

static char *path_ext(char *p) {
    char *cp;
    for (cp = p; *cp != '\0'; cp++)
        ;

    for (; cp >= p && *cp != '.'; cp--)
        ;

    return *cp == '.' ? cp : cp + 1;
}

static char *path_join(char *p1, char *p2) {
    char *p = (char *) malloc(strlen(p1) + strlen(p2) + 2);
    assert(p != NULL);
    sprintf(p, "%s/%s", p1, p2);
    return p;
}

static char *path_replace_ext(char *p, char *ext) {
    char *cp = p + strlen(p);

    for (; cp >= p && *cp != '.'; cp--)
        ;

    if (*cp == '.') {
        size_t ext_len = strlen(ext);
        char *res = malloc((cp - p) + ext_len + 1);
        stpncpy(res, p, cp - p);
        stpncpy(res + (cp - p), ext, ext_len + 1);
        return res;
    }

    return p;
}