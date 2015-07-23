#ifndef KERPLUNK_GO_H_
#define KERPLUNK_GO_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

struct go_state {
    uint8_t board[24][32];

    uint64_t hash;

    uint8_t size; // max 21, to limit valid positions to 512
    unsigned turn   : 2;
    unsigned passed : 1;
    unsigned scored : 1;
    unsigned        : 4;
    int16_t score;
};

bool go_init(void);
void go_setup(struct go_state *state, size_t size, size_t handicap, uint16_t *ha_pos);
void go_copy(const struct go_state *state, struct go_state *out);
bool go_play(struct go_state *state, uint16_t move);
bool go_legal(struct go_state *state, uint16_t move);
void go_moves(struct go_state *state, uint16_t *moves, size_t *count);
void go_moves_exact(struct go_state *state, uint16_t *moves, size_t *count);
void go_print(struct go_state *state, FILE *stream);

#endif//KERPLUNK_GO_H_
