#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <sodium.h>

#include "go.h"
#include "record.h"
#include "features.h"
#include "gtree.h"
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

            struct game_replay replay;
            replay_start(&replay, &record);
            while (replay_step(&replay));

            if (replay.move_num != record.num_moves) {
                fprintf(stderr, "in game #%zu of %s:\n", record_index, paths[i]);
                go_print(&replay.state, stderr);
            }
            else {
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
                uint16_t move_oct = octant_from_matrix(move, state.size);
                fprintf(stdout, ",%u,%u", HEIGHT(move_oct)-1, DIAOFF(move_oct));

                // feature 2: radius-4 L1 neighborhood
                uint8_t nbhd[41];
                feature_neighborhood(&state, move, nbhd, 41);
                for (size_t j = 0; j < 41; j++) {
                    if (nbhd[j] == GO_COLOR_EMPTY) {
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
                        const int sqdist = metric_l2_squared(move, last_moves[j]);
                        fprintf(stdout, ",%d", sqdist);
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
