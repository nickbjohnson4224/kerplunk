#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <sodium.h>
#include <omp.h>

#include "go.h"
#include "sgf.h"
#include "mcts.h"

int replay(size_t num_paths, char **paths) {

    for (size_t i = 0; i < num_paths; i++) {
        FILE *file;
        if (!strcmp(paths[i], "-")) {
            file = stdin;
        }
        else {
            file = fopen(paths[i], "r");
        }

        if (!file) {
            perror("replay");
            return -1;
        }

        struct sgf_record record;
        if (!sgf_load(&record, file, true)) {
            fprintf(stderr, "error parsing record from %s\n", paths[i]);
            return -1;
        }

        sgf_dump(&record, stdout);

        if (file != stdin) {
            fclose(file);
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    srand(clock());
    sodium_init();
    go_init();

    if (argc < 2) {
        fprintf(stderr, "Usage: kerplunk <cmd>\n");
        return -1;
    }

    if (!strcmp(argv[1], "replay")) {
        if (argc < 3) {
            fprintf(stderr, "Usage kerplunk replay [SGF_FILE]...\n");
            return -1;
        }

        replay(argc - 2, &argv[2]);
    }
    else {
        fprintf(stderr, "kerplunk: unknown command %s\n", argv[1]);
        return -1;
    }

    return 0;
}
