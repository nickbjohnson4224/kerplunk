#ifndef KERPLUNK_MCTS_H_
#define KERPLUNK_MCTS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "go.h"

void mc_run_random_playout(struct go_state *state);

struct mcts_tree {
    struct go_state state;
    size_t num_playouts;
    size_t num_wins;
    
    struct mcts_tree *parent;

    size_t num_moves;
    uint16_t moves[512];
    struct mcts_tree *subtrees[];
};

struct mcts_tree *mcts_tree_new(struct go_state *state);
struct mcts_tree *mcts_descend(struct mcts_tree *tree, uint16_t move);
void mcts_tree_free(struct mcts_tree *tree);

void mcts_run_random_playouts(struct mcts_tree *tree, size_t count);

#endif//KERPLUNK_MCTS_H_
