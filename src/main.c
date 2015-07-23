#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>

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
    go_setup(&state, 19, 0, NULL);

    struct mcts_tree *mcts = mcts_tree_new(&state);
    go_print(&mcts->state, stdout);

    mcts_run_random_playouts(mcts, 1000);
}

int main(int argc, char **argv) {
    srand(clock());
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
    else {
        fprintf(stderr, "kerplunk: unknown command %s\n", argv[1]);
        return -1;
    }

    return 0;
}
