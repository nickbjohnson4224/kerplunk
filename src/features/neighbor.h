#ifndef KERPLUNK_FEATURES_NEIGHBOR_H_
#define KERPLUNK_FEATURES_NEIGHBOR_H_

#include <stddef.h>

#include "../go.h"

void feature_neighborhood(struct go_state *state, go_move move, go_color *buffer, size_t count);

#endif//KERPLUNK_FEATURES_NEIGHBOR_H_
