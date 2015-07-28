#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#include "feat.h"
#include "go.h"

uint16_t matrix_to_octant(uint16_t mat_pos, size_t size) {
    assert(size <= 21);

    size_t row = mat_pos >> 8;
    size_t col = mat_pos & 0xFF;
    assert(1 <= row && row <= size);
    assert(1 <= row && col <= size);

    size_t octant = 0;
    
    if (row > (size + 1) / 2) {
        octant |= 4;
        row = size + 1 - row;
    }

    if (col > (size + 1) / 2) {
        octant |= 2;
        col = size + 1 - col;
    }

    if (row > col) {
        octant |= 1;
        uint8_t tmp = row;
        row = col;
        col = tmp;
    }

    const size_t height = row;
    const size_t diaoff = col - row;
    assert(1 <= height && height <= (size + 1) / 2);
    assert(1 <= height + diaoff && height + diaoff <= (size + 1) / 2);

    return (octant << 8) | (height << 4) | diaoff;
}

uint16_t octant_to_matrix(uint16_t oct_pos, size_t size) {
    assert(size <= 21);

    size_t octant = OCTANT(oct_pos);
    size_t height = HEIGHT(oct_pos);
    size_t diaoff = DIAOFF(oct_pos);
    assert(octant < 8);

    size_t row;
    size_t col;
    if (octant & 1) {
        row = height + diaoff;
        col = height;
    }
    else {
        row = height;
        col = height + diaoff;
    }

    if (octant & 2) {
        col = size + 1 - col;
    }

    if (octant & 4) {
        row = size + 1 - row;
    }

    assert(1 <= row && row <= size);
    assert(1 <= col && col <= size);
    return (row << 8) | col;
}

void feat_neighborhood(struct go_state *state, uint16_t mat_pos, uint8_t *buffer, size_t count) {
    const size_t size = state->size;

    // table of radii
    //
    //   0 - self (1)
    //   1 - contact (4)
    //   2 - one-point, diagonal (8)
    //   3 - two-point, knight's (12)
    //   4 - three-point, large knight's, elephants (16)
    //

    size_t max_radius;
    switch (count) {
    case 1:           max_radius = 0; break;
    case 1+4:         max_radius = 1; break;
    case 1+4+8:       max_radius = 2; break;
    case 1+4+8+12:    max_radius = 3; break;
    case 1+4+8+12+16: max_radius = 4; break;
    default: assert(false); return;
    }

    const uint16_t oct_pos = matrix_to_octant(mat_pos, size);
    const int octant = OCTANT(oct_pos);
    const int height = HEIGHT(oct_pos);
    const int offset = height + DIAOFF(oct_pos);

    const int start[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    const int delta[4][2] = {{-1, 1}, {-1, -1}, {1, -1}, {1, 1}}; 

    size_t i = 0;
    const uint8_t row = GO_MOVE_ROW(mat_pos);
    const uint8_t col = GO_MOVE_COL(mat_pos);
    buffer[i] = state->board[row][col];
    i++;

    for (size_t r = 1; r <= max_radius; r++) {
        for (size_t q = 0; q < 4; q++) {
            const int h0 = height + r * start[q][0];
            const int f0 = offset + r * start[q][1];

            for (size_t dq = 0; dq < r; dq++) {
                const int h = h0 + dq * delta[q][0];
                const int f = f0 + dq * delta[q][1];

                int r = (octant & 1) ? f : h;
                int c = (octant & 1) ? h : f;

                if (octant & 2) {
                    c = size + 1 - c;
                }
                if (octant & 4) {
                    r = size + 1 - r;
                }

                uint8_t color;
                if ((r < 1) || (r > (int) size) || (c < 1) || (c > (int) size)) {
                    color = GO_COLOR_EMPTY;
                }
                else {
                    color = state->board[r][c];
                }

                buffer[i] = color;
                i++;
            }
        }
    }
}
