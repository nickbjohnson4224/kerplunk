#ifndef KERPLUNK_GO_H_
#define KERPLUNK_GO_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

//
// state structure
//

typedef uint8_t go_color;

#define GO_COLOR_EMPTY 0
#define GO_COLOR_BLACK 1
#define GO_COLOR_WHITE 2

struct go_state {
    go_color board[24][32];

    uint64_t hash;

    uint8_t  size; // max 21, to limit valid positions to 512
    uint8_t  turn   : 2;
    uint8_t  passed : 1;
    uint8_t  scored : 1;
    uint8_t         : 4;
    int16_t  score; // raw score before komi
    uint16_t bcaps; // black stones captured, not stones captured _by_ black
    uint16_t wcaps; // ditto
};

bool go_init(void);
void go_copy(const struct go_state *state, struct go_state *out);
bool go_equal(const struct go_state *state0, const struct go_state *state1);

//
// move representation
//

typedef uint16_t go_move; // move in matrix coordinates, 1-indexed

#define GO_MOVE_PASS 0
#define GO_MOVE(row, col) (((row) << 8) | (col))
#define GO_MOVE_ROW(move) ((move) >> 8)
#define GO_MOVE_COL(move) ((move) & 0xFF)

//
// rules implementation
//

void go_setup(struct go_state *state, size_t size, size_t hcap, go_move *hcaps);
bool go_play(struct go_state *state, go_move move);
bool go_legal(struct go_state *state, go_move move);
void go_moves(struct go_state *state, go_move *moves, size_t *count);
void go_moves_loose(struct go_state *state, go_move *moves, size_t *count);

void go_print(struct go_state *state, FILE *stream);

#endif//KERPLUNK_GO_H_
