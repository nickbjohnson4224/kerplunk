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
#include "mcts.h"

void bench() {

    // simple playout test
    const size_t trials = 10000;

    fprintf(stderr, "running %zu playouts (%d threads)\n", trials, omp_get_max_threads());

    #pragma omp parallel for default(none) schedule(static)
    for (size_t i = 0; i < trials; i++) {
        struct go_state state;
        go_setup(&state, 19, 0, NULL);
        mc_run_random_playout(&state);
    }
}

void mcts_test() {

    struct go_state state;
    go_setup(&state, 13, 0, NULL);

    struct mcts_tree *mcts = mcts_new(&state);

    while (1) {
        go_print(&mcts->state, stdout);

        if (mcts->state.scored) {
            break;
        }
        
        fprintf(stdout, "\n");

        for (size_t i = 0; i < 100000; i++) {
            mcts_run_random_playout(mcts);

            if (i % 100 == 99) {
                double wr = (double) mcts->num_black_wins / mcts->num_playouts;
                fprintf(stdout, "\r%0.1f%% (%zu playouts)", wr * 100, mcts->num_playouts);
                fflush(stdout);
            }
        }

        fprintf(stdout, "\n\n");

        uint16_t move = mcts_choose(mcts);
        mcts = mcts_descend(mcts, move);

        fprintf(stdout, "%zu playouts saved\n", mcts->num_playouts);
    }

    mcts_free(mcts);
}

void generate_states() {
    unsigned int rand_state = rand();
    const size_t size = 9;

    fprintf(stdout, "@RELATION kerplunk_states\n");

    fprintf(stdout, "@ATTRIBUTE size INTEGER\n");
    fprintf(stdout, "@ATTRIBUTE turn {B,W}\n");
    fprintf(stdout, "@ATTRIBUTE passed INTEGER\n");
    fprintf(stdout, "@ATTRIBUTE scored INTEGER\n");
    for (size_t r = 1; r <= size; r++) {
        for (size_t c = 1; c <= size; c++) {
            fprintf(stdout, "@ATTRIBUTE pos_%zu_%zu {E,B,W}\n", r, c);
        }
    }

    fprintf(stdout, "@ATTRIBUTE black_winrate INTEGER\n");

    fprintf(stdout, "@DATA\n");
    while (1) {
        struct go_state state;
        go_setup(&state, size, 0, NULL);

        while (!state.scored) {
        
            if (rand_r(&rand_state) % 100 == 0) {
                break;
            }

            uint16_t all_moves[512];
            size_t num_moves;
            go_moves(&state, all_moves, &num_moves);

            uint16_t move;
            do {
                move = all_moves[rand_r(&rand_state) % num_moves];
            } while (!go_legal(&state, move));

            go_play(&state, move);
        }

        if (state.scored) {
            continue;
        }

        size_t num_playouts = 1000;
        size_t num_black_wins = 0;

        #pragma omp parallel for
        for (size_t i = 0; i < num_playouts; i++) {
            struct go_state state1;
            go_copy(&state, &state1);
            mc_run_random_playout(&state1);

            if (state1.score - 5.5 > 0) {
                num_black_wins++;
            }
        }

        go_dump_csv(&state, stdout);
        fprintf(stdout, "%0.3f\n", (double) num_black_wins / num_playouts);
    }
}

struct rated_move {
    uint16_t move;
    double rate;
};

int rated_move_compar(const void *_a, const void *_b) {
    const struct rated_move *a = _a;
    const struct rated_move *b = _b;

    if (a->rate < b->rate) {
        return 1;
    }
    else if (a->rate == b->rate) {
        return 0;
    }
    else {
        return -1;
    }
}

void salience() {
    unsigned int rand_state = rand();
    const size_t size = 19;

    while (1) {
        struct go_state state;
        go_setup(&state, size, 0, NULL);

        while (!state.scored) {
        
            if (rand_r(&rand_state) % 100 == 0) {
                break;
            }

            uint16_t all_moves[512];
            size_t num_moves;
            go_moves(&state, all_moves, &num_moves);

            uint16_t move;
            do {
                move = all_moves[rand_r(&rand_state) % num_moves];
            } while (!go_legal(&state, move));

            go_play(&state, move);
        }

        if (state.scored) {
            continue;
        }

        go_print(&state, stdout);

        struct mcts_tree *mcts = mcts_new(&state);

        for (size_t i = 0; i < 100000; i++) {
            mcts_run_random_playout(mcts);
        }

        struct rated_move rated_moves[512];

        for (size_t i = 0; i < mcts->num_moves; i++) {
            uint16_t move = mcts->moves[i];
            double wr = (double) mcts->subtrees[i]->num_black_wins / mcts->subtrees[i]->num_playouts;
            
            rated_moves[i].move = move;
            rated_moves[i].rate = (mcts->state.turn == BLACK) ? wr : 1-wr;
        }

        qsort(rated_moves, mcts->num_moves, sizeof(struct rated_move), rated_move_compar);

        for (size_t i = 0; i < 10 && i < mcts->num_moves; i++) {
            uint8_t row = rated_moves[i].move >> 8;
            uint8_t col = rated_moves[i].move & 0xFF;

            double wr = rated_moves[i].rate;

            fprintf(stderr, "(%u, %u) %0.3f\n", row, col, wr);
        }
    }
}

int main(int argc, char **argv) {
    srand(clock());
    sodium_init();
    go_init();

    if (argc < 2) {
        fprintf(stderr, "Usage: kerplunk <cmd>\n");
        return -1;
    }

    if (!strcmp(argv[1], "bench")) {
        bench();
    }
    else if (!strcmp(argv[1], "mcts_test")) {
        mcts_test();
    }
    else if (!strcmp(argv[1], "generate")) {
        generate_states();
    }
    else if (!strcmp(argv[1], "salience")) {
        salience();
    }
    else {
        fprintf(stderr, "kerplunk: unknown command %s\n", argv[1]);
        return -1;
    }

    return 0;
}
