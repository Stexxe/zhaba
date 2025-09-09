#include <stdio.h>

#include "lib/common.h"
#include "lib/prep.h"

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "Usage: %s src-file\n", argv[0]);
    }

    pool_init(2 * 1024 * 1024);
    char *path = argv[1];

    int outsz = 10 * 1024;
    char *expanded_src = pool_alloc(outsz, char);

    char *spaths[] = {
        "/usr/lib/gcc/x86_64-linux-gnu/13/include",
        "/usr/local/include",
        "/usr/include/x86_64-linux-gnu",
        "/usr/include"
    };

    prep_search_paths_set(spaths, sizeof(spaths) / sizeof(spaths[0]));
    DefineTable *def_table = prep_define_newtable();

    char *srcend = prep_expand(path, def_table, expanded_src, &outsz);
    *srcend = '\0';

    printf("%s", expanded_src);
}
