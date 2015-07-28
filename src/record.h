#ifndef KERPLUNK_RECORD_H_
#define KERPLUNK_RECORD_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "go.h"

//
// non-branching game record structure
//

struct game_record {

    // metadata
    char *name;
    char *date;
    char *black_name;
    char *black_rank;
    char *white_name;
    char *white_rank;
    char *copyright;

    // setup parameters
    char *ruleset;
    size_t size;
    size_t handicap;
    go_move *handicaps;
    float komi;

    // results
    char *result;
    float score;

    // move sequence
    size_t num_moves;
    go_move *moves;
};

void game_record_free(struct game_record *record);

//
// game record iterator
//

struct game_replay {
    const struct game_record *record;
    size_t move_num;
    struct go_state state;
};

void replay_start(struct game_replay *replay, const struct game_record *record);
bool replay_step(struct game_replay *replay);

#endif//KERPLUNK_RECORD_H_
