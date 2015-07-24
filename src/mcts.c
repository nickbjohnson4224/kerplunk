#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "mcts.h"

void mc_run_random_playout(struct go_state *state) {
    unsigned int rand_state = rand();

    while (!state->scored) {
        uint16_t all_moves[512];
        size_t num_moves;
        go_moves_loose(state, all_moves, &num_moves);

        uint16_t move;
        do {
            move = all_moves[rand_r(&rand_state) % num_moves];
        } while (!go_legal(state, move));

        go_play(state, move);
    }
}

struct mcts_tree *mcts_new(struct go_state *state) {
    size_t num_moves;
    uint16_t moves[512];
    go_moves(state, moves, &num_moves);

    struct mcts_tree *tree = malloc(
        sizeof(struct mcts_tree) +
        num_moves * sizeof(struct mcts_tree*)
    );

    if (!tree) {
        return NULL;
    }

    go_copy(state, &tree->state);
    tree->num_playouts = 0;
    tree->num_black_wins = 0;
    tree->parent = NULL;
    tree->num_moves = num_moves;
    for (size_t i = 0; i < num_moves; i++) {
        tree->moves[i] = moves[i];
        tree->subtrees[i] = NULL;
    }

    return tree;
}

struct mcts_tree *mcts_descend(struct mcts_tree *tree, uint16_t move) {

    for (size_t i = 0; i < tree->num_moves; i++) {
        if (tree->moves[i] == move && tree->subtrees[i]) {
            struct mcts_tree *subtree = tree->subtrees[i];
            subtree->parent = NULL;
            tree->subtrees[i] = NULL;
            mcts_free(tree);
            return subtree;
        }
    }

    return NULL;
}

void mcts_free(struct mcts_tree *tree) {
    
    if (!tree) {
        return;
    }

    assert(!tree->parent);

    for (size_t i = 0; i < tree->num_moves; i++) {
        if (tree->subtrees[i]) {
            tree->subtrees[i]->parent = NULL;
            mcts_free(tree->subtrees[i]);
            tree->subtrees[i] = NULL;
        }
    }

    free(tree);
}

#define EXPAND_THRESHOLD 2
#define OPTIMISM 10

void mcts_run_random_playout(struct mcts_tree *tree) {
    unsigned int rand_state = rand();

    // descend tree, choosing max UCT node at each level
    size_t depth = 1;
    while (1) {
        
        if (tree->num_playouts < EXPAND_THRESHOLD) {
            break;
        }

        if (tree->num_moves == 0) {
            break;
        }

        //double curr_wr = (double) tree->num_black_wins / tree->num_playouts;
        
        double best_uct = (tree->state.turn == BLACK) ? -100. : 100.;
        struct mcts_tree *best_subtree = NULL;
        for (size_t i = 0; i < tree->num_moves; i++) {
            if (!tree->subtrees[i]) {
                
                // derive successor position
                struct go_state sub_state;
                go_copy(&tree->state, &sub_state);
                go_play(&sub_state, tree->moves[i]);
                
                // expand
                tree->subtrees[i] = mcts_new(&sub_state);
                tree->subtrees[i]->parent = tree;

                // always choose newly-expanded nodes
                best_subtree = tree->subtrees[i];
                break;
            }
            else {
                struct mcts_tree *st = tree->subtrees[i];

                double wr = (double) (st->num_black_wins + (tree->state.turn == BLACK) ? OPTIMISM : -OPTIMISM) / st->num_playouts;
                double jitter = 0.001 * rand_r(&rand_state) / RAND_MAX;
                wr += jitter;
                double uct = wr;

                if (tree->state.turn == BLACK) {
                    // max node
                    if (uct > best_uct) {
                        best_uct = uct;
                        best_subtree = st;
                    }
                }
                else {
                    // min node
                    if (uct < best_uct) {
                        best_uct = uct;
                        best_subtree = st;
                    }
                }
            }
        }

        assert(best_subtree != NULL);
        tree = best_subtree;
        depth++;
    }

    // perform playout
    struct go_state playout_state;
    go_copy(&tree->state, &playout_state);
    mc_run_random_playout(&playout_state);

    bool b_won = (playout_state.score - 5.5) > 0 ? true : false;

    // propagate back to root
    while (tree) {
        tree->num_playouts++;
        if (b_won) {
            tree->num_black_wins++;
        }

        tree = tree->parent;
    }
}

uint16_t mcts_choose(struct mcts_tree *tree) {
    unsigned int rand_state = rand();
    
    double best_wr = (tree->state.turn == BLACK) ? -100. : 100.;
    uint16_t best_move = 0;
    for (size_t i = 0; i < tree->num_moves; i++) {
        if (tree->subtrees[i]) {
            struct mcts_tree *st = tree->subtrees[i];
            double wr = (double) (st->num_black_wins - (tree->state.turn == BLACK) ? OPTIMISM : -OPTIMISM) / st->num_playouts;
            double jitter = 0.001 * rand_r(&rand_state) / RAND_MAX;
            wr += jitter;

            if (tree->state.turn == BLACK) {
                // max
                if (wr > best_wr) {
                    best_wr = wr;
                    best_move = tree->moves[i];
                }
            }
            else {
                // min
                if (wr < best_wr) {
                    best_wr = wr;
                    best_move = tree->moves[i];
                }
            }
        }
    }

    return best_move;
}
