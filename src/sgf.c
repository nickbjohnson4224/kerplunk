#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "sgf.h"
#include "go.h"

static int _fgetc_skip_whitespace(FILE *stream) {
    while (1) {
        int c = fgetc(stream);
        if (c == EOF || !isspace(c)) {
            return c;
        }
    }
}

static char *_parse_prop_value(FILE *stream) {
    int c;

    size_t cap_buffer = 4;
    size_t len_buffer = 0;
    char *buffer = malloc(cap_buffer);
    if (!buffer) {
        return NULL;
    }

    while (1) {
        c = fgetc(stream);
        if (c == EOF) {
            free(buffer);
            return NULL;
        }

        if (c == ']') {
            // end of string
            break;
        }

        if (isspace(c)) {
            if (c == '\r') {
                c = fgetc(stream);
                if (c == '\r') {
                    ungetc(c, stream);
                }
                c = '\n';
            }
            else if (c == '\n') {
                c = fgetc(stream);
                if (c == '\n') {
                    ungetc(c, stream);
                }
                c = '\n';
            }
            else {
                c = ' ';
            }
        }

        if (c == '\\') {
            // escaped character
            c = fgetc(stream);
            if (c == EOF) {
                free(buffer);
                return NULL;
            }

            if (c == '\r') {
                c = fgetc(stream);
                if (c == '\r') {
                    ungetc(c, stream);
                }
                continue;
            }
            if (c == '\n') {
                c = fgetc(stream);
                if (c == '\n') {
                    ungetc(c, stream);
                }
                continue;
            }
        }

        if (len_buffer == cap_buffer) {
            cap_buffer *= 4;
            buffer = realloc(buffer, cap_buffer);
            if (!buffer) {
                return NULL;
            }
        }
        
        buffer[len_buffer] = c;
        len_buffer++;
    }
    
    if (len_buffer == cap_buffer) {
        cap_buffer += 1;
        buffer = realloc(buffer, cap_buffer);
        if (!buffer) {
            return NULL;
        }
    }
    
    buffer[len_buffer] = '\0';

    return buffer;
}

bool sgf_load(struct game_record *record, FILE *stream, bool verbose) {
    assert(record);

    #define PARSE_ERROR(...)\
    do {\
        if (verbose) {\
            fprintf(stderr, "parse error: " __VA_ARGS__);\
        }\
        game_record_free(record);\
        return false;\
    } while (0);

    memset(record, 0, sizeof(struct game_record));

    // defaults for optional properties
    record->size = 19;
    record->komi = 5.5;
    record->handicap = 0;

    int c;

    c = _fgetc_skip_whitespace(stream);
    if (c == EOF) {
        return false;
    }
    if (c != '(') {
        PARSE_ERROR("SGF record doesn't begin with a '('\n");
    }

    // 
    // parse header node
    //   
   
    c = _fgetc_skip_whitespace(stream);
    if (c != ';') {
        PARSE_ERROR("invalid header node start\n");
    }

    while (1) {
        char propname[2];

        // parse 1/2-letter property name

        c = _fgetc_skip_whitespace(stream);
        if (c == EOF) {
            // EOF while parsing header
            game_record_free(record);

            return false;
        }
        if (c == ';' || c == ')') {
            // end of header
            break;
        }
        if (!isalpha(c) || !isupper(c)) {
            PARSE_ERROR("property names must be in upper-case letters\n")
        }
        propname[0] = c;
        
        c = fgetc(stream);
        if (c == EOF) {
            PARSE_ERROR("EOF while parsing header");
        }
        
        if (c == '[') {
            // one-letter property
            propname[1] = ' ';
        }
        else {
            if (!isalpha(c) || !isupper(c)) {
                PARSE_ERROR("property names must be in upper-case letters\n");
            }
            propname[1] = c;
            c = fgetc(stream);
        }

        if (c != '[') {
            PARSE_ERROR("property must have at least one value\n");
        }

        // parse property value
        char *propval = _parse_prop_value(stream);
        if (!propval) {
            PARSE_ERROR("property value parse error\n");
        }
        
        #define IF_PROP(prop) if (!strncmp(propname, prop, 2))

        #define EXPECT_PROP(prop, value)\
        if (!strncmp(propname, prop, 2)) {\
            if (strcmp(propval, value)) {\
                free(propval);\
                PARSE_ERROR("expected property " prop " to have value " value "\n");\
            }\
            free(propval);\
        }

        EXPECT_PROP("GM", "1");
        EXPECT_PROP("FF", "4");

        #undef EXPECT_PROP

        #define RECORD_PROP(prop, field)\
        if (!strncmp(propname, prop, 2)) {\
            if (record->field) {\
                free(propval);\
                PARSE_ERROR("duplicate property " prop "\n");\
            }\
            record->field = propval;\
        }

        RECORD_PROP("GN", name);
        RECORD_PROP("DT", date);
        RECORD_PROP("PB", black_name);
        RECORD_PROP("BR", black_rank);
        RECORD_PROP("PW", white_name);
        RECORD_PROP("WR", white_rank);
        RECORD_PROP("PC", copyright);
        RECORD_PROP("RU", ruleset);
        RECORD_PROP("RE", result);

        #undef RECORD_PROP

        IF_PROP("KM") {
            record->komi = strtof(propval, NULL);
            free(propval);
        }

        IF_PROP("SZ") {
            record->size = strtoul(propval, NULL, 10);
            free(propval);
        }

        IF_PROP("HA") {
            size_t handicap = strtoul(propval, NULL, 10);
            if (record->handicaps && handicap != record->handicap) {
                PARSE_ERROR("inconsistent handicap count\n");
            }
            record->handicap = handicap;
            free(propval);
        }

        IF_PROP("AB") {
            if (record->handicaps) {
                free(propval);
                PARSE_ERROR("duplicate property AB\n");
            }

            size_t num_handicaps = 0;
            size_t cap_handicaps = (record->handicap) ? record->handicap : 16;
            uint16_t *handicaps = malloc(sizeof(uint16_t) * cap_handicaps);
            if (!handicaps) {
                free(propval);
                PARSE_ERROR("memory allocation error on handicaps buffer\n");
            }

            while (1) {
                if (strlen(propval) != 2) {
                    free(propval);
                    free(handicaps);
                    PARSE_ERROR("invalid handicap AB property value length\n");
                }

                if (num_handicaps >= cap_handicaps) {
                    // TODO expanding buffer 
                    // almost all handicaps are <= 9 though,
                    // and are pre-declared anyway
                    free(propval);
                    free(handicaps);
                    PARSE_ERROR("exceeded pre-declared handicap buffer\n");
                }
                
                const uint8_t col = propval[0] - 'a' + 1;
                const uint8_t row = propval[1] - 'a' + 1;
                const uint16_t pos = (row << 8) | col;

                handicaps[num_handicaps] = pos;
                num_handicaps++;

                free(propval);

                c = _fgetc_skip_whitespace(stream);
                if (c == '[') {
                    // more handicaps
                    propval = _parse_prop_value(stream);
                    if (!propval) {
                        free(handicaps);
                        PARSE_ERROR("memory allocation error on handicaps buffer\n");
                    }
                    continue;
                }
                else {
                    ungetc(c, stream);
                    break;
                }
            }

            if (record->handicap && record->handicap != num_handicaps) {
                PARSE_ERROR("inconsistent handicap count\n");
            }
            
            record->handicaps = handicaps;
            record->handicap = num_handicaps;
        }

        #undef IF_PROP
    }

    // parse result score
    

    // sanity checks on fields

    if (record->komi != floor(record->komi) && record->komi != floor(record->komi) + 0.5f) {
        record->komi = floor(record->komi) + 0.5f;
    }
    
    if (!record->ruleset || (strcmp(record->ruleset, "Japanese") &&
        strcmp(record->ruleset, "Chinese") && strcmp(record->ruleset, "AGA") &&
        strcmp(record->ruleset, "GOE") && strcmp(record->ruleset, "NZ"))) {

        PARSE_ERROR("unknown or missing ruleset\n");
    }

    if (record->handicap && !record->handicaps) {
        PARSE_ERROR("handicap stone positions not defined\n");
    }

    if (c == ')') {
        // no moves
        return true;
    }

    // 
    // parse move sequence
    //

    size_t num_moves = 0;
    size_t cap_moves = 512; // won't typically be exceeded
    uint16_t *moves = malloc(sizeof(uint16_t) * cap_moves);

    uint8_t turn = (record->handicap) ? WHITE : BLACK;
    while (1) {
        assert(c == ';');

        while (1) {

            c = _fgetc_skip_whitespace(stream);
            if (c == EOF) {
                PARSE_ERROR("EOF while parsing move\n");
            }
            else if (c == ';') {
                // end of move node
                break;
            }
            else if (c == ')') {
                // end of game record
                break;
            }
            else if (c == 'B' || c == 'W') {
                uint8_t color = (c == 'B') ? BLACK : WHITE;

                c = fgetc(stream);
                if (isalpha(c) && isalpha(c)) {
                    // other property; skip
                    goto skip_propvals;
                }

                // move
                if (c != '[') {
                    free(moves);
                    PARSE_ERROR("expected '[', got %c\n", c);
                }
                
                if (color != turn) {
                    PARSE_ERROR("turn order mismatch\n");
                }

                uint8_t row;
                uint8_t col;

                c = fgetc(stream);
                if (c == ']') {
                    // pass
                    row = 0;
                    col = 0;
                }
                else if (isalpha(c)) {
                    col = c - 'a' + 1;
                    c = fgetc(stream);
                    if (c == EOF || !isalpha(c)) {
                        free(moves);
                        PARSE_ERROR("invalid move property value\n");
                    }
                    row = c - 'a' + 1;

                    c = fgetc(stream);
                    if (c != ']') {
                        free(moves);
                        PARSE_ERROR("expected ']' at end of move property value\n");
                    }
                }
                else {
                    free(moves);
                    PARSE_ERROR("invalid move property value\n");
                }

                if (!(row == 0 && col == 0) &&
                    (row == 0 || col == 0 ||
                    row > record->size || col > record->size)) {

                    // invalid move position
                    free(moves);
                    PARSE_ERROR("invalid move position (%u, %u)\n", row, col);
                }

                uint16_t move = (row << 8) | col;

                if (num_moves >= cap_moves) {
                    cap_moves *= 2;
                    moves = realloc(moves, sizeof(uint16_t) * cap_moves);
                    if (!moves) {
                        PARSE_ERROR("memory allocation error on moves buffer\n");
                    }
                }

                moves[num_moves] = move;
                num_moves++;

                turn ^= BLACK|WHITE;
                continue;
            }
            else if (isalpha(c) && isupper(c)) {
                // other property; skip
                skip_propvals:

                while (1) {
                    c = fgetc(stream);
                    if (c == EOF) {
                        free(moves);
                        PARSE_ERROR("EOF while skipping non-move property value\n");
                    }
                    else if (c != '[') {
                        ungetc(c, stream);
                        break;
                    }

                    while (1) {
                        c = fgetc(stream);
                        if (c == '\\') {
                            c = fgetc(stream);
                            if (c == EOF) {
                                free(moves);
                                PARSE_ERROR("EOF while skipping non-move property value\n");
                            }
                        }
                        else if (c == ']') {
                            break;
                        }
                    }
                }
            }
            else {
                free(moves);
                PARSE_ERROR("invalid property name %c\n", c);
            }
        }

        if (c == ')') {
            // end of game record
            break;
        }
    }

    record->num_moves = num_moves;
    record->moves = moves;

    #undef PARSE_ERROR

    return true;
}

void sgf_dump(struct game_record *record, FILE *stream) {
    assert(record);

    #define DUMP_FIELD(prop, field)\
    if (record->field) {\
        fprintf(stream, prop "[%s]", record->field);\
    }

    // print header

    // metadata
    fprintf(stream, "(;GM[1]FF[4]SZ[%zu]", record->size);
    DUMP_FIELD("GN", name);
    DUMP_FIELD("DT", date);
    DUMP_FIELD("PB", black_name);
    DUMP_FIELD("BR", black_rank);
    DUMP_FIELD("PW", white_name);
    DUMP_FIELD("WR", white_rank);
    DUMP_FIELD("PC", copyright);
 
    // setup
    DUMP_FIELD("RU", ruleset);
    fprintf(stream, "SZ[%zu]", record->size);
    fprintf(stream, "HA[%zu]", record->handicap);

    if (record->handicap > 0) {
        fprintf(stream, "AB");
        for (size_t i = 0; i < record->handicap; i++) {
            uint8_t row = record->handicaps[i] >> 8;
            uint8_t col = record->handicaps[i] & 0xFF;
            fprintf(stream, "[%c%c]", 'a' + col - 1, 'a' + row - 1);
        }
    }

    fprintf(stream, "KM[%0.2f]", record->komi);

    // scoring/results
    if (record->result) {
        fprintf(stream, "RE[%s]", record->result);
    }
    else {
        float score = record->score;
        if (score > 0) {
            fprintf(stream, "RE[B+%0.1f]", score);
        }
        else {
            fprintf(stream, "RE[W+%0.1f]", -score);
        }
    }

    // move sequence
    uint8_t color = (record->handicap > 0) ? WHITE : BLACK;
    for (size_t i = 0; i < record->num_moves; i++) {
        uint16_t move = record->moves[i];
        if (move != 0) {
            uint8_t row = move >> 8;
            uint8_t col = move & 0xFF;
            fprintf(stream, ";%c[%c%c]", (color == BLACK) ? 'B' : 'W', 'a' + col - 1, 'a' + row - 1);
        }
        else {
            fprintf(stream, ";%c[]", (color == BLACK) ? 'B' : 'W');
        }
        color ^= BLACK | WHITE;
    }

    fprintf(stream, ")\n");

    #undef DUMP_FIELD
}
