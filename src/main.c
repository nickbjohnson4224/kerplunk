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

int kerplunk_cat(size_t num_paths, char **paths) {

    //
    // concatenate and validate several SGF files to stdout
    // invalid SGF records are dropped (but show warnings)
    // used for importing game databases
    // 

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

        size_t record_index = 0;
        bool parse_error = false;
        while (!parse_error && !feof(file)) {
            record_index++;

            struct sgf_record record;
            if (!sgf_load(&record, file, true)) {
                parse_error = true;
                if (!feof(file)) {
                    fprintf(stderr, "error parsing game #%zu from %s\n", record_index, paths[i]);
                }
                break;
            }

            // validate game record by replaying it

            struct go_state state;
            go_setup(&state, record.size, record.handicap, record.handicaps);
            
            bool valid = true;
            for (size_t i = 0; i < record.num_moves; i++) {
                const uint16_t move = record.moves[i];
                if (!go_legal(&state, move)) {
                    fprintf(stderr, "in game #%zu of %s:\n", record_index, paths[i]);
                    go_print(&state, stderr);
                    fprintf(stderr, "illegal move (%d, %d)\n", move >> 8, move & 0xFF);
                    valid = false;
                    break;
                }

                if (!go_play(&state, move)) {
                    assert(false);
                }
            }

            if (valid) {
                sgf_dump(&record, stdout);
            }

            sgf_free(&record);


        }

        if (file != stdin) {
            fclose(file);
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    srand(clock());
    if (sodium_init() == -1) {
        perror("failure to initialize libsodium");
        return -1;
    }
    go_init();

    if (argc < 2) {
        fprintf(stderr, "Usage: kerplunk <cmd>\n");
        return -1;
    }

    if (!strcmp(argv[1], "cat")) {
        if (argc < 3) {
            fprintf(stderr, "Usage kerplunk cat [SGF_FILE]...\n");
            return -1;
        }

        kerplunk_cat(argc - 2, &argv[2]);
    }
    else {
        fprintf(stderr, "kerplunk: unknown command %s\n", argv[1]);
        return -1;
    }

    return 0;
}
