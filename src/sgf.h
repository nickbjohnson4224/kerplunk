#ifndef KERPLUNK_SGF_H_
#define KERPLUNK_SGF_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "go.h"

//
// linear (non-branching) Go SGF parser
//

struct sgf_record {
    
    // metadata
    char *name;
    char *date;
    char *b_name;
    char *b_rank;
    char *w_name;
    char *w_rank;
    char *copyright;
   
    // setup
    char *ruleset; // default: "Chinese"
    size_t size;
    size_t handicap; // if 0, B plays first; otherwise W plays first
    uint16_t *handicaps;
    float komi;

    // scoring/results
    char *result;
    int16_t base_score; // score before komi

    // move sequence
    size_t num_moves;
    uint16_t *moves;
};

bool sgf_load(struct sgf_record *record, FILE *stream, bool verbose);
void sgf_dump(struct sgf_record *record, FILE *stream);
void sgf_free(struct sgf_record *record);

#endif//KERPLUNK_SGF_H_
