#ifndef KERPLUNK_FEATURES_METRIC_H_
#define KERPLUNK_FEATURES_METRIC_H_

#include <stdlib.h>

#include "../assert.h"
#include "../go.h"

static inline int metric_l1(go_move p1, go_move p2) {
    assert(p1 && p2);

    const int r1 = GO_MOVE_ROW(p1);
    const int c1 = GO_MOVE_COL(p1);
    const int r2 = GO_MOVE_ROW(p2);
    const int c2 = GO_MOVE_COL(p2);

    return abs(r1 - r2) + abs(c1 - c2);
}

static inline int metric_l2_squared(go_move p1, go_move p2) {
    assert(p1 && p2);

    const int r1 = GO_MOVE_ROW(p1);
    const int c1 = GO_MOVE_COL(p1);
    const int r2 = GO_MOVE_ROW(p2);
    const int c2 = GO_MOVE_COL(p2);

    return (r1 - r2) * (r1 - r2) + (c1 - c2) * (c1 - c2);
}

#endif//KERPLUNK_FEATURES_METRIC_H_
