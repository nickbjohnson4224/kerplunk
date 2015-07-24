#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <sodium.h>

#include "go.h"

#define MAX_STRINGSIZE 512

// position flags

#define VISITED 4 // temporary mark for flood fills

// utility functions

static bool _try_capture(struct go_state *state, size_t row, size_t col, bool pretend);
static void _score(struct go_state *state);

static uint64_t _go_pos_hash[24][32][2];

bool go_init(void) {
    
    if (sodium_init() == -1) {
        return false;
    }

    // build psudorandom zobrist hash lookup table
    const uint8_t k[32] = "kerplunk zobrist hash lookup tab";
    const uint8_t n[32] = "00000000000000000000000000000000";
    crypto_stream_chacha20((void*) _go_pos_hash, sizeof(_go_pos_hash), n, k);

    return true;
}

void go_setup(struct go_state *state, size_t size, size_t hcap, uint16_t *hcaps) {
    assert(size <= 21);

    // initialize blank board
    memset(state, 0, sizeof(struct go_state));
    state->size = size;
    state->turn = (hcap == 0) ? BLACK : WHITE;

    for (size_t i = 0; i < hcap; i++) {
        const uint8_t row = hcaps[i] >> 8;
        const uint8_t col = hcaps[i] & 0xFF;
        assert(row < 24 && col < 32);

        assert(state->board[row][col] == EMPTY);
        state->board[row][col] = BLACK;
        state->hash ^= _go_pos_hash[row][col][0];
    }
}

void go_copy(const struct go_state *state, struct go_state *out) {
    memcpy(out, state, sizeof(struct go_state));
}

static bool _try_capture(struct go_state *state, size_t row, size_t col, bool pretend) {
    uint8_t stack[MAX_STRINGSIZE][2];
    uint8_t list[MAX_STRINGSIZE][2];
    size_t stack_next = 0;
    size_t list_next = 0;

    const size_t size = state->size;
    assert(size <= 21);

    const uint8_t color = state->board[row][col];

    // push initial element
    stack[0][0] = row;
    stack[0][1] = col;
    stack_next++;

    // DFS of connected stones to find liberties (and mark string)
    while (stack_next) {

        // pop location
        uint8_t r = stack[stack_next-1][0];
        uint8_t c = stack[stack_next-1][1];
        stack_next--;

        if (state->board[r][c] & VISITED) {
            continue;
        }

        // check neighbors
        uint8_t N[4][2] = {{r-1, c}, {r+1, c}, {r, c-1}, {r, c+1}};
        for (size_t i = 0; i < 4; i++) {
            uint8_t r1 = N[i][0];
            uint8_t c1 = N[i][1];

            if (state->board[r1][c1] == color) {
                // note: this precludes (r1, c1) being marked VISITED

                // push neighbor
                stack[stack_next][0] = r1;
                stack[stack_next][1] = c1;
                stack_next++;
                
                assert(stack_next <= MAX_STRINGSIZE);
            }
            else if (state->board[r1][c1] == EMPTY) {
                if (r1 && r1 <= size && c1 && c1 <= size) {
                    // found liberty, no capture
                    
                    // clean up marks
                    for (size_t j = 0; j < list_next; j++) {
                        state->board[list[j][0]][list[j][1]] &= ~VISITED;
                    }

                    return false;
                }
            }
        }
        
        // add to visited list
        list[list_next][0] = r;
        list[list_next][1] = c;
        list_next++;
        
        assert(list_next <= MAX_STRINGSIZE);

        state->board[r][c] |= VISITED;
    }

    // no liberties found

    if (!pretend) {
    
        // remove all marked positions
        uint64_t hash_delta = 0;
        for (size_t j = 0; j < list_next; j++) {
            const uint8_t r = list[j][0];
            const uint8_t c = list[j][1];

            hash_delta ^= _go_pos_hash[r][c][color-1];
            state->board[r][c] = EMPTY;
        }

        // record capture counts
        if (color == BLACK) {
            state->bcaps += list_next;
        }
        else {
            state->wcaps += list_next;
        }
        
        state->hash ^= hash_delta;
    }
    else {

        // just remove marks
        for (size_t j = 0; j < list_next; j++) {
            state->board[list[j][0]][list[j][1]] &= ~VISITED;
        }
    }

    return true;
}

static void _score(struct go_state *state) {
    uint8_t stack[MAX_STRINGSIZE][2];
    uint8_t list[MAX_STRINGSIZE][2];

    const size_t size = state->size;
    assert(size <= 19);

    // fill in or mark all holes
    size_t row = 1;
    size_t col = 1;
    while (1) {

        // find next unvisited hole
        while (state->board[row][col] != EMPTY) {
            col++;
            if (col > size) {
                col = 0;
                row++;
                if (row > size) {
                    break;
                }
            }
        }

        if (row > size) {
            break;
        }
    
        size_t stack_next = 0;
        size_t list_next = 0;

        // push initial element
        stack[0][0] = row;
        stack[0][1] = col;
        stack_next++;

        // DFS to find extent of hole and who owns it
        bool white_edge = false;
        bool black_edge = false;
        while (stack_next) {

            // pop location
            uint8_t r = stack[stack_next-1][0];
            uint8_t c = stack[stack_next-1][1];
            stack_next--;

            if (state->board[r][c] & VISITED) {
                continue;
            }

            // check neighbors
            uint8_t N[4][2] = {{r-1, c}, {r+1, c}, {r, c-1}, {r, c+1}};
            for (size_t i = 0; i < 4; i++) {
                uint8_t r1 = N[i][0];
                uint8_t c1 = N[i][1];

                if (state->board[r1][c1] == EMPTY) {
                    // note: this precludes (r1, c1) being marked VISITED

                    if (r1 && r1 <= size && c1 && c1 <= size) {
                        // push neighbor
                        stack[stack_next][0] = r1;
                        stack[stack_next][1] = c1;
                        stack_next++;

                        assert(stack_next <= MAX_STRINGSIZE);
                    }
                }
                else if (state->board[r1][c1] == BLACK) {
                    black_edge = true;
                }
                else if (state->board[r1][c1] == WHITE) {
                    white_edge = true;
                }
            }
            
            // add to visited list
            list[list_next][0] = r;
            list[list_next][1] = c;
            list_next++;
            
            assert(list_next <= MAX_STRINGSIZE);

            state->board[r][c] |= VISITED;
        }

        if (white_edge == black_edge) {
            // no owner, leave marks
        }
        else if (black_edge) {

            // owned by black
            for (size_t j = 0; j < list_next; j++) {
                state->board[list[j][0]][list[j][1]] = BLACK;
            }
        }
        else if (white_edge) {

            // owned by white
            for (size_t j = 0; j < list_next; j++) {
                state->board[list[j][0]][list[j][1]] = WHITE;
            }
        }
    }

    // remove all marks and tally score
    int16_t black_count = 0;
    int16_t white_count = 0;
    for (size_t r = 1; r <= size; r++) {
        for (size_t c = 1; c <= size; c++) {
            if (state->board[r][c] == BLACK) {
                black_count++;
            }
            else if (state->board[r][c] == WHITE) {
                white_count++;
            }
            else {
                state->board[r][c] = EMPTY;
            }
        }
    }

    state->score = black_count - white_count;
    state->scored = 1;

    return;
}

bool go_play(struct go_state *state, uint16_t move) {
    const size_t size = state->size;
    assert(size <= 21);

    // check if game is over
    if (state->scored) {
        return false;
    }

    if (!move) {
        if (state->passed) {
            // game over, score
            _score(state);
        }
        else {
            // normal pass (set flag)
            state->passed = 1;
        }
    }    
    else {
        const uint8_t own_color = state->turn;
        const uint8_t opp_color = own_color ^ (WHITE | BLACK);
        const size_t row = move >> 8;
        const size_t col = move & 0xFF;
        assert(row < 24 && col < 32);

        // check if move is in range
        if (row == 0 || row > size || col == 0 || col > size) {
            return false;
        }
        
        // check if space is open
        if (state->board[row][col] != EMPTY) {
            return false;
        }

        // (tentatively) place piece
        state->board[row][col] = own_color;

        // check for neighbors and neighbor properties
        uint8_t neighbor_colors = state->board[row-1][col]
            | state->board[row+1][col]
            | state->board[row][col-1]
            | state->board[row][col+1];

        bool any_captures = false;
        if (neighbor_colors & opp_color) {
            // attempt captures

            #define TRYCAP(r,c) \
                ((state->board[r][c]&opp_color)&&\
                _try_capture(state,r,c, /*pretend*/ false))

            if TRYCAP(row+1, col) {
                any_captures = true;
            }
            if TRYCAP(row-1, col) {
                any_captures = true;
            }
            if TRYCAP(row, col+1) {
                any_captures = true;
            }
            if TRYCAP(row, col-1) {
                any_captures = true;
            }

            #undef TRYCAP
        }

        if (!any_captures) {
            // check for suicide

            if (_try_capture(state, row, col, /*pretend*/ true)) {

                // revert placement
                state->board[row][col] = EMPTY;
                return false;
            }
        }
        
        // update hash
        state->hash ^= _go_pos_hash[row][col][own_color-1];

        // clear passed flag
        state->passed = 0;
    }

    // next player's turn
    state->turn ^= WHITE | BLACK;

    return true;
}

bool go_legal(struct go_state *state, uint16_t move) {
    const size_t size = state->size;
    assert(size <= 21);

    // check if game is over
    if (state->scored) {
        return false;
    }

    // check for pass
    if (!move) {
        return true;
    }

    const uint8_t own_color = state->turn;
    const uint8_t opp_color = own_color ^ (WHITE | BLACK);
    const size_t row = move >> 8;
    const size_t col = move & 0xFF;
    assert(row < 24 && col < 32);

    // check if move is in range
    if (row == 0 || row > size || col == 0 || col > size) {
        return false;
    }
    
    // check if space is open
    if (state->board[row][col] != EMPTY) {
        return false;
    }

    // check for neighbors and neighbor properties
    uint8_t neighbor_colors = state->board[row-1][col]
        | state->board[row+1][col]
        | state->board[row][col-1]
        | state->board[row][col+1];

    // check for isolated point
    if (neighbor_colors == EMPTY) {
        return true;
    }

    // (tentatively) place piece
    state->board[row][col] = state->turn;

    if (neighbor_colors & opp_color) {
        // attempt captures

        #define TRYCAP(r,c) \
            ((state->board[r][c]&opp_color)&&\
            _try_capture(state,r,c, /*pretend*/ true))

        if TRYCAP(row+1, col) {
            state->board[row][col] = EMPTY;
            return true;
        }
        if TRYCAP(row-1, col) {
            state->board[row][col] = EMPTY;
            return true;
        }
        if TRYCAP(row, col+1) {
            state->board[row][col] = EMPTY;
            return true;
        }
        if TRYCAP(row, col-1) {
            state->board[row][col] = EMPTY;
            return true;
        }

        #undef TRYCAP
    }

    // check for suicide
    if (_try_capture(state, row, col, /*pretend*/ true)) {
        state->board[row][col] = EMPTY;
        return false;
    }

    state->board[row][col] = EMPTY;
    return true;
}

void go_moves(struct go_state *state, uint16_t *moves, size_t *count) {
    const size_t size = state->size;
    assert(size <= 21);

    moves[0] = 0; // pass is always legal

    size_t _count = 1;
    for (uint8_t r = 1; r <= size; r++) {
        for (uint8_t c = 1; c <= size; c++) {
            if (state->board[r][c] == EMPTY) {
                moves[_count] = (r << 8) | c;
                _count++;
            }
        }
    }

    *count = _count;
}

void go_moves_exact(struct go_state *state, uint16_t *moves, size_t *count) {
    const size_t size = state->size;
    assert(size <= 21);

    moves[0] = 0; // pass is always legal

    size_t _count = 1;
    for (uint8_t r = 1; r <= size; r++) {
        for (uint8_t c = 1; c <= size; c++) {
            uint16_t move = (r << 8) | c;
            if (state->board[r][c] == EMPTY && go_legal(state, move)) {
                moves[_count] = move;
                _count++;
            }
        }
    }

    *count = _count;
}

void go_print(struct go_state *state, FILE *stream) {
    
    // so beautiful...

    fputs("\xe2\x95\x94", stream);
    for (size_t c = 0; c <= state->size; c++) {
        fputs("\xe2\x95\x90\xe2\x95\x90", stream);
    }
    fputs("\xe2\x95\x90\xe2\x95\x97\n", stream);

    fputs("\xe2\x95\x91", stream);
    for (size_t c = 0; c <= state->size; c++) {
        fputs("  ", stream);
    }
    fputs(" \xe2\x95\x91\n", stream);

    for (size_t r = 1; r <= state->size; r++) {
        fputs("\xe2\x95\x91  ", stream);
        for (size_t c = 1; c <= state->size; c++) {
            if (state->board[r][c] == EMPTY) {
                fputs("\xc2\xb7 ", stream);
            }
            else if (state->board[r][c] == BLACK) {
                fputs("\xe2\xac\xa4 ", stream);
            }
            else if (state->board[r][c] == WHITE) {
                fputs("\xe2\x97\xaf ", stream);
            }
        }
        fputs(" \xe2\x95\x91\n", stream);
    }
    
    fputs("\xe2\x95\x91", stream);
    for (size_t c = 0; c <= state->size; c++) {
        fputs("  ", stream);
    }
    fputs(" \xe2\x95\x91\n", stream);

    fputs("\xe2\x95\x9a", stream);
    for (size_t c = 0; c <= state->size; c++) {
        fputs("\xe2\x95\x90\xe2\x95\x90", stream);
    }
    fputs("\xe2\x95\x90\xe2\x95\x9d\n", stream);

    if (state->scored) {
        fprintf(stream, "GAME OVER; score = %+d\n", (int) state->score);
    }
    else {
        if (state->turn == BLACK) {
            fprintf(stream, "BLACK to play");
        }
        else {
            fprintf(stream, "WHITE to play");
        }

        if (state->passed) {
            fprintf(stream, "; previous player PASSED\n");
        }
        else {
            fprintf(stream, "\n");
        }
    }
}
