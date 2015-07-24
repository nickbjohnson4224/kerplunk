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
    size_t num_black_wins;
    
    struct mcts_tree *parent;

    size_t num_moves;
    uint16_t moves[512];
    struct mcts_tree *subtrees[];
};

struct mcts_tree *mcts_new(struct go_state *state);
struct mcts_tree *mcts_descend(struct mcts_tree *tree, uint16_t move);
void mcts_free(struct mcts_tree *tree);

void mcts_run_random_playout(struct mcts_tree *tree);
uint16_t mcts_choose(struct mcts_tree *tree);

#endif//KERPLUNK_MCTS_H_
