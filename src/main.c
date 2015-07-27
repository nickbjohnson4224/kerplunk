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
#include "record.h"
#include "feat.h"
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
            perror("cat");
            return -1;
        }

        size_t record_index = 0;
        bool parse_error = false;
        while (!parse_error && !feof(file)) {
            record_index++;

            struct game_record record;
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

            game_record_free(&record);
        }

        if (file != stdin) {
            fclose(file);
        }
    }

    return 0;
}

int kerplunk_move_features(char *path) {

    FILE *file;
    if (!strcmp(path, "-")) {
        file = stdin;
    }
    else {
        file = fopen(path, "r");
    }

    if (!file) {
        perror("move_features");
        return -1;
    }

    size_t record_index = 0;
    while (1) {
        if (record_index % 1000 == 0) {
            fprintf(stderr, "%zu game records processed\n", record_index);
        }

        struct game_record record;
        if (!sgf_load(&record, file, true)) {
            if (!feof(file)) {
                fprintf(stderr, "error parsing game #%zu from %s\n", record_index, path);
            }
            break;
        }

        // replay

        struct go_state state;
        go_setup(&state, record.size, record.handicap, record.handicaps);

        const size_t num_last_moves = 4;
        uint16_t last_moves[4];
        for (size_t i = 0; i < 4; i++) {
            last_moves[i] = 0;
        }

        for (size_t k = 0; k < record.num_moves; k++) {
            size_t num_moves;
            uint16_t moves[512];
            go_moves_loose(&state, moves, &num_moves);

            const size_t num_sample_moves = 4;
            uint16_t sample_moves[4];
            sample_moves[0] = record.moves[k];
            for (size_t i = 1; i < num_sample_moves; i++) {
                sample_moves[i] = moves[rand() % num_moves];
            }

            for (size_t i = 0; i < num_sample_moves; i++) {
                const uint16_t move = sample_moves[i];

                if (move == 0) {
                    continue;
                }

                // class (0 - non-move, 1 - move)
                fprintf(stdout, "%u", (move == record.moves[k]) ? 1 : 0);

                // feature 1: octant coordinates
                uint16_t move_oct = matrix_to_octant(move, state.size);
                fprintf(stdout, ",%u,%u", HEIGHT(move_oct)-1, DIAOFF(move_oct));

                // feature 2: radius-4 neighborhood
                uint8_t nbhd[41];
                feat_neighborhood(&state, move, nbhd, 41);
                for (size_t j = 0; j < 41; j++) {
                    if (nbhd[j] == EMPTY) {
                        fprintf(stdout, ",E");
                    }
                    else if (nbhd[i] == state.turn) {
                        fprintf(stdout, ",B");
                    }
                    else {
                        fprintf(stdout, ",W");
                    }
                }

                // feature 3: square distance to last few moves

                for (size_t j = 0; j < num_last_moves; j++) {
                    if (last_moves[i]) {
                        const int row1 = move >> 8;
                        const int col1 = move & 0xFF;
                        const int row0 = last_moves[j] >> 8;
                        const int col0 = last_moves[j] & 0xFF;

                        fprintf(stdout, ",%d", (row1-row0)*(row1-row0)+(col1-col0)*(col1-col0));
                    }
                    else {
                        fprintf(stdout, ",?");
                    }
                }

                fprintf(stdout, "\n");
            }

            go_play(&state, record.moves[k]);

            for (size_t i = num_last_moves-1; i > 0; i--) {
                last_moves[i] = last_moves[i-1];
            }
            last_moves[0] = record.moves[k];
        }

        record_index++;
    }

    if (file != stdin) {
        fclose(file);
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
            fprintf(stderr, "Usage: kerplunk cat [SGF_FILE]...\n");
            return -1;
        }

        return kerplunk_cat(argc - 2, &argv[2]);
    }
    else if (!strcmp(argv[1], "move_features")) {
        if (argc != 3) {
            fprintf(stderr, "Usage: kerplunk move_features SGF_FILE\n");
            return -1;
        }

        return kerplunk_move_features(argv[2]);
    }
    else {
        fprintf(stderr, "kerplunk: unknown command %s\n", argv[1]);
        return -1;
    }

    return 0;
}
