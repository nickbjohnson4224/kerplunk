#include <stdlib.h>
#include <assert.h>

#include "record.h"
#include "go.h"

void game_record_free(struct game_record *record) {
    free(record->name);
    free(record->date);
    free(record->black_name);
    free(record->black_rank);
    free(record->white_name);
    free(record->white_rank);
    free(record->copyright);
    free(record->ruleset);
    free(record->handicaps);
    free(record->result);
    free(record->moves);
}

void replay_start(struct game_replay *replay, const struct game_record *record) {
    assert(replay != NULL);
    assert(record != NULL);

    replay->record = record;
    go_setup(&replay->state, record->size, record->handicap, record->handicaps);

    replay->move_num = 0;
}

bool replay_step(struct game_replay *replay) {
    assert(replay != NULL);
    assert(replay->record != NULL);
    
    if (replay->move_num >= replay->record->num_moves) {
        return false;
    }

    const uint16_t move = replay->record->moves[replay->move_num];
    if (!go_legal(&replay->state, move)) {
        return false;
    }

    if (!go_play(&replay->state, move)) {
        assert(false); // should always be caught by the legality check
    }

    return true;
}
