local ffi = require('ffi')
local C = ffi.C

ffi.cdef [[

// from go.h
struct go_state {
    uint8_t  board[24][32];

    uint64_t hash;

    uint8_t  size; // max 21, to limit valid positions to 512
    uint8_t  turn;
    uint8_t  passed;
    uint8_t  scored;
    int16_t  score; // raw score before komi
    uint16_t bcaps; // black stones captured, not stones captured _by_ black
    uint16_t wcaps; // ditto
};

void go_setup(struct go_state *state, size_t size, size_t hcap, uint16_t *hcaps);
bool go_play(struct go_state *state, uint16_t move);
bool go_legal(struct go_state *state, uint16_t move);
void go_moves(struct go_state *state, uint16_t *moves, size_t *count);
void go_moves_loose(struct go_state *state, uint16_t *moves, size_t *count);
void go_print(struct go_state *state, void *stream);

// from record.h
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
    uint16_t *handicaps;
    float komi;

    // results
    char *result;
    float score;

    // move sequence
    size_t num_moves;
    uint16_t *moves;
};

void game_record_free(struct game_record *record);

struct game_replay {
    const struct game_record *record;
    size_t move_num;
    struct go_state state;
};

void replay_start(struct game_replay *replay, const struct game_record *record);
bool replay_step(struct game_replay *replay);

// from sgf.h
bool sgf_load(struct game_record *record, void *stream, bool verbose);
void sgf_dump(struct game_record *record, void *stream);

]]

kerplunk = {}

function kerplunk.go_print(state, ...)
    local args = {...}
    local stream
    if args[0] then
        stream = args[0]
    else
        stream = io.stdout
    end

    C.go_print(state, stream)
end

function kerplunk.new_replay(record)
    local replay = ffi.new('struct game_replay')

    C.replay_start(replay, record)
    return replay
end

function kerplunk.replay_step(replay)
    return C.replay_step(replay)
end

function kerplunk.sgf_load(stream)
    local record = ffi.new('struct game_record')
    ffi.gc(record, C.game_record_free)

    if not C.sgf_load(record, stream, true) then
        return nil
    end

    return record
end

function kerplunk.sgf_dump(record, stream)
    C.sgf_dump(record, stream)
end

return kerplunk
