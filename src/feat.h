#ifndef KERPLUNK_FEAT_H_
#define KERPLUNK_FEAT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "go.h"

// //// Octant and Matrix Coordinate Systems ////
//
// // Definition //
//
// A Go board has many symmetries. In particular, every symmetry of the square
// is also a symmetry of the Go board, in the sense that transforming the 
// board under any of those symmetries produces an isomorphic game to the
// original. However, the normal (row, column) "matrix" coordinate system
// for the board does not nicely interact with these symmetries. So, for the
// purpose of pattern-matching, an alternative coordinate system is preferred.
//
// The "octant" coordinate system is created from the matrix system by first
// applying symmetries to the board until the given point is within a specific
// fundamental domain of the symmetry group (the upper-upper-left octant).
//
// This fundamental domain octant is highlighted in the diagram below:
//    _____________
//   |\*****|     /|
//   |  \***|   /  |
//   |    \*| /    |
//   |------|------|
//   |    / | \    |
//   |  /   |   \  |
//   |/_____|_____\|
//
// Every element of the symmetry group of the square can be produced uniquely 
// by either applying or not applying the following operations in order:
// 
//   1) Vertical reflection         (r, c) |-> (s+1-r, c)
//   2) Horizontal reflection       (r, c) |-> (r, s+1-c)
//   3) Transpose                   (r, c) |-> (c, r)
//
//   (where s is the board size)
//
// Note that these operations are all self-inverting (i.e. involutions). Also
// note that 1) and 2) commute with each other, but not with 3).
//
// So, the first coordinate of the 3-dimensional octant system is a 3-bit
// binary representation of which operations were performed to map the 
// original point into the fundamental octant. If V, H, and T are these bits,
// the coordinate is:
//
//   octant = V * 4 + H * 2 + T .
//
// This results in an implicit labeling of each octant:
//    _____________
//   |\     |     /|
//   |  \ 0 | 2 /  |
//   |  1 \ | / 3  |
//   |------|------|
//   |  5 / | \ 7  |
//   |  / 4 | 6 \  |
//   |/_____|_____\|
//
// Once this remapping is complete, the other two coordinates are easily 
// derived from the transformed matrix coordinates (row, col):
//
//   height = row
//   diaoff = col - row
//
// The resulting (octant, height, diaoff) triple is the octant coordinate.
//
// It is important to note that the octant coordinate system is not unique,
// meaning that, for some positions, there are multiple valid octant 
// coordatines. Specifically, there are two valid coordinates for each
// position on the diagonals and on the vertical and horizontal medians of
// the board, and eight for the tengen (center) point. But, these coordinates
// differ only in the octant index. So, the _canonical_ octant coordinate for
// any point is defined as the octant coordinate with the minimal-index 
// octant. This produces a unique coordinate for each point.
//
// // Encoding //
//
// For a 19x19 board, the coordinates have the following ranges:
//
//   0 <= octant <= 7
//   1 <= height <= 10
//   0 <= diaoff <= 10 - height <= 9
//
// Just like the matrix coordinates are encoded as the high and low bytes of
// a 16-bit word in the Go rules logic, the octant system has a bit-packed
// representation:
//
//   (MSB) 00000ooo hhhhdddd (LSB)
//
//   or, alternatively, (o, h, d) |-> (o << 8) | (h << 4) | d
//
// The functions matrix_to_octant and octant_to_matrix implement this
// encoding.
//
// // Notes on Usage //
//
// The height coordinate exactly matches the common Go term of the "kth line", 
// while the diaoff gives the distance to the nearest diagonal. Both are 
// important factors in human judgement, especially during the opening.
// See http://senseis.xmp.net/?HighAndLowMoves for more information.
// 
// A fourth, reduntant coordinate might also be useful:
//
//   medoff = (size + 1) / 2 - col
//
// This represents the distance to the nearest board median (horizontal or
// vertical center line). This is probably less useful than diagonal distance,
// but may still be a nice feature to feed a machine learning algorithm.
//

#define OCTANT(oct_pos) (((oct_pos) >> 8) & 0x7)
#define HEIGHT(oct_pos) (((oct_pos) >> 4) & 0xF)
#define DIAOFF(oct_pos) ((oct_pos) & 0xF)

uint16_t matrix_to_octant(uint16_t mat_pos, size_t size);
uint16_t octant_to_matrix(uint16_t oct_pos, size_t size);

// neighborhoods
void feat_neighborhood(struct go_state *board, uint16_t mat_pos, uint8_t *buffer, size_t count);

#endif//KERPLUNK_FEAT_H_
