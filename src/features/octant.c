#include <stdlib.h>
#include <stdio.h>

#include "../assert.h"
#include "octant.h"

uint16_t octant_from_matrix(uint16_t mat_pos, size_t size) {
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
