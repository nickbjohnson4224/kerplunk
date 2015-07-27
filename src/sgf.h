#ifndef KERPLUNK_SGF_H_
#define KERPLUNK_SGF_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "record.h"
#include "go.h"

//
// linear (non-branching) Go SGF parser
//

bool sgf_load(struct game_record *record, FILE *stream, bool verbose);
void sgf_dump(struct game_record *record, FILE *stream);

#endif//KERPLUNK_SGF_H_
