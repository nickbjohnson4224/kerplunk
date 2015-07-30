#ifndef KERPLUNK_GTREE_H_
#define KERPLUNK_GTREE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "go.h"

//
// generic game tree structure
//

struct gtree;

struct gtree_node {
    struct go_state state;

    size_t reldepth;
    struct gtree *tree;
    struct gtree_node *parent;
    const struct gtree_schema *schema;

    size_t num_moves;

    // moves, child pointers and tag data embedded past this point using magic
    uint8_t _vdata[];
};

struct gtree_tagtype {
    size_t size_per_state; // bytes per state
    size_t size_per_move; // bytes per move
    
    void (*_init)(void *state_tag, void *move_tags, const struct gtree_node *node);
    void (*_free)(void *state_tag, void *move_tags);
};

struct gtree_schema {
    size_t num_tags;
    size_t cap_tags;

    size_t size_base;
    size_t size_per_move;
    
    // vdata_size = size_base + num_moves * size_per_move
    
    size_t movedata_base_offset;
    
    // moves = _data + movedata_base_offset
    // children = _data + movedata_base_offset +
    //     num_moves * sizeof(go_move)

    struct _gtree_schema_tag {
        const char *tagname;
        const struct gtree_tagtype *tagtype;

        size_t movetag_size;
        size_t statetag_offset;
        size_t movetags_offset_per_move;

        // statetag = _data + statetag_offset
        // movetags = _data + movedata_base_offset + 
        //     num_moves * movetags_offset_per_move
        // movetag[move_index] = _data + movedata_base_offset +
        //     num_moves * movetags_offset_per_move +
        //     move_index * movetag_size

    } *tags;
};

struct gtree {
    const struct gtree_schema *schema;
    struct gtree_node *root;
    size_t reldepth;
    size_t maxdepth;
};

// schema operations
void gtree_schema_init(struct gtree_schema *schema);
int  gtree_schema_add(struct gtree_schema *schema, const char *tagname, const struct gtree_tagtype *tagtype);
int  gtree_schema_get_tagid(struct gtree_schema *schema, const char *tagname);
void gtree_schema_free(struct gtree_schema *schema);

// whole-tree operations
bool gtree_setup(struct gtree *gtree, const struct go_state *state, const struct gtree_schema *schema);
void gtree_free(struct gtree *gtree);
bool gtree_walk(struct gtree *gtree, void *state, void (*func)(void *state, struct gtree_node *node));
bool gtree_descend_to(struct gtree *gtree, struct gtree_node *node);

// node operations
struct gtree_node *gtree_child(struct gtree_node *node, go_move move, bool expand);
void gtree_prune(struct gtree_node *node, go_move move);
void *gtree_statetag(struct gtree_node *node, int tagid);
void *gtree_movetag(struct gtree_node *node, int tagid, go_move move);
void *gtree_movetags(struct gtree_node *node, int tagid);
int gtree_depth(struct gtree_node *node);
int gtree_move_index(struct gtree_node *node, go_move move);

#endif//KERPLUNK_GTREE_H_
