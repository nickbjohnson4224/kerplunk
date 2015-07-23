#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#include "mcts.h"

void mc_run_random_playout(struct go_state *state) {
    unsigned int rand_state = rand();

    while (!state->scored) {
        uint16_t all_moves[19 * 19 + 1];
        size_t num_moves;
        go_moves(state, all_moves, &num_moves);

        uint16_t move;
        do {
            move = all_moves[rand_r(&rand_state) % num_moves];
        } while (!go_legal(state, move));

        go_play(state, move);
    }
}

struct mcts_tree *mcts_tree_new(struct go_state *state) {
    size_t num_moves;
    uint16_t moves[512];
    go_moves_exact(state, moves, &num_moves);

    struct mcts_tree *tree = malloc(
        sizeof(struct mcts_tree) +
        num_moves * sizeof(struct mcts_tree*)
    );

    if (!tree) {
        return NULL;
    }

    go_copy(state, &tree->state);
    tree->num_playouts = 0;
    tree->num_wins = 0;
    tree->parent = NULL;
    tree->num_moves = num_moves;
    for (size_t i = 0; i < num_moves; i++) {
        tree->moves[i] = moves[i];
        tree->subtrees[i] = NULL;
    }

    return tree;
}

struct mcts_tree *mcts_tree_descend(struct mcts_tree *tree, uint16_t move) {

    for (size_t i = 0; i < tree->num_moves; i++) {
        if (tree->moves[i] == move && tree->subtrees[i]) {
            struct mcts_tree *subtree = tree->subtrees[i];
            subtree->parent = NULL;
            tree->subtrees[i] = NULL;
            mcts_tree_free(tree);
            return subtree;
        }
    }

    return NULL;
}

void mcts_tree_free(struct mcts_tree *tree) {
    
    if (!tree) {
        return;
    }

    assert(!tree->parent);

    for (size_t i = 0; i < tree->num_moves; i++) {
        if (tree->subtrees[i]) {
            tree->subtrees[i]->parent = NULL;
            mcts_tree_free(tree->subtrees[i]);
            tree->subtrees[i] = NULL;
        }
    }

    free(tree);
}

void mcts_run_random_playouts(struct mcts_tree *tree, size_t count) {
    
}
