#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include "assert.h"
#include "gtree.h"

static struct gtree_node *_create_root_node(struct gtree *tree, const struct go_state *state_);
static struct gtree_node *_create_node(struct gtree_node *parent, go_move move);
static bool _delete_subtree(struct gtree_node *node);

//
// schema operations
//

#define SCHEMA_MAX_TAGS 32

void gtree_schema_init(struct gtree_schema *schema) {
    schema->num_tags = 0;
    schema->cap_tags = 0;
    schema->size_base = 0;
    schema->size_per_move = sizeof(go_move) + sizeof(struct gtree_node*);
    schema->movedata_base_offset = 0;
    schema->tags = NULL;
}

int gtree_schema_add(struct gtree_schema *schema, const char *tagname, const struct gtree_tagtype *tagtype) {
    assert(schema);
    assert(tagname);
    assert(tagtype);
   
    // check if tag name exists
    assert(schema->num_tags < SCHEMA_MAX_TAGS);
    for (size_t i = 0; i < schema->num_tags; i++) {
        if (!strcmp(schema->tags[i].tagname, tagname)) {
            // tag name exists
            return -1;
        }
    }

    // reserve space for new tag
    assert(schema->num_tags <= schema->cap_tags);
    if (schema->num_tags == schema->cap_tags) {
        schema->cap_tags += 4;
        struct _gtree_schema_tag *tags = realloc(schema->tags, 
            schema->cap_tags * sizeof(struct _gtree_schema_tag));
        if (!tags) {
            // memory allocaton error
            return -1;
        }

        schema->tags = tags;
    }

    if (schema->num_tags >= SCHEMA_MAX_TAGS) {
        // too many tags
        return -1;
    }
    
    struct _gtree_schema_tag *tag = &schema->tags[schema->num_tags];
    schema->num_tags++;

    tag->tagname = tagname;
    tag->tagtype = tagtype;
    tag->movetag_size = tagtype->size_per_move;
    tag->statetag_offset = schema->size_base;
    schema->size_base += tagtype->size_per_state;
    tag->movetags_offset_per_move = schema->size_per_move;
    schema->size_per_move += tagtype->size_per_move;

    return schema->num_tags - 1;
}

int gtree_schema_get_tagid(struct gtree_schema *schema, const char *tagname) {
    assert(schema);
    assert(tagname);

    assert(schema->num_tags < SCHEMA_MAX_TAGS);
    for (size_t i = 0; i < schema->num_tags; i++) {
        if (!strcmp(schema->tags[i].tagname, tagname)) {
            return i;
        }
    }

    return -1;
}

void gtree_schema_free(struct gtree_schema *schema) {
    free(schema->tags);
}

//
// whole-tree operations
//

bool gtree_setup(struct gtree *tree, const struct go_state *state, const struct gtree_schema *schema) {
    assert(tree);
    assert(state);
    assert(schema);

    tree->schema = schema;
    tree->reldepth = 0;
    tree->maxdepth = 0;

    tree->root = _create_root_node(tree, state);
    if (!tree->root) {
        return false;
    }

    return true;
}

void gtree_free(struct gtree *gtree) {
    assert(gtree);

    if (gtree->root) {
        _delete_subtree(gtree->root);
    }
}

//
// node operations
//

static struct gtree_node *_create_root_node(struct gtree *tree, const struct go_state *state_) {
    assert(tree);
    assert(tree->schema);

    const struct gtree_schema *schema = tree->schema;

    struct go_state state;
    go_copy(&state, &state_);

    go_move moves[512];
    size_t num_moves;
    go_moves(&state, moves, &num_moves);

    size_t size = sizeof(struct gtree_node) +
        schema->size_base +
        schema->size_per_move * num_moves;

    struct gtree_node *node = malloc(size);
    if (!node) {
        // memory allocation error
        return NULL;
    }

    go_copy(&state, &node->state);
    node->reldepth = 0;
    node->tree = tree;
    node->parent = NULL;
    node->num_moves = num_moves;
   
    uint8_t *_vdata = node->_vdata;

    go_move *node_moves = (void*) &_vdata[schema->movedata_base_offset];
    memset(node_moves, 0, sizeof(go_move) * num_moves);

    struct gtree_node **node_children = (void*)
        &_vdata[schema->movedata_base_offset + num_moves * sizeof(go_move)];
    memset(node_children, 0, sizeof(struct gtree_node *) * num_moves);

    // initialize tag data
    for (size_t i = 0; i < schema->num_tags; i++) {
        assert(schema->tags[i].tagtype);
        
        if (!schema->tags[i].tagtype->_init) {
            continue;
        }

        const size_t statetag_offset = schema->tags[i].statetag_offset;
        const size_t movetags_offset = schema->movedata_base_offset +
            schema->tags[i].movetags_offset_per_move * num_moves;

        void *statetag = &_vdata[statetag_offset];
        void *movetags = &_vdata[movetags_offset];

        schema->tags[i].tagtype->_init(statetag, movetags, node);
    }

    return node;
}

static struct gtree_node *_create_node(struct gtree_node *parent, go_move move) {
    assert(parent);
    assert(parent->tree);
    assert(parent->tree->schema);

    const struct gtree_schema *schema = parent->tree->schema;

    struct go_state state;
    go_copy(&state, &parent->state);
    if (!go_play(&state, move)) {
        // illegal move
        return NULL;
    }

    go_move moves[512];
    size_t num_moves;
    go_moves(&state, moves, &num_moves);

    size_t size = sizeof(struct gtree_node) + 
        schema->size_base + 
        schema->size_per_move * num_moves;

    struct gtree_node *node = malloc(size);
    if (!node) {
        // memory allocation error
        return NULL;
    }

    go_copy(&state, &node->state);
    node->reldepth = parent->reldepth + 1;
    node->tree = parent->tree;
    node->parent = parent;
    node->num_moves = num_moves;

    uint8_t *_vdata = node->_vdata;

    go_move *node_moves = (void*) &_vdata[schema->movedata_base_offset];
    memset(node_moves, 0, sizeof(go_move) * num_moves);

    struct gtree_node **node_children = (void*)
        &_vdata[schema->movedata_base_offset + num_moves * sizeof(go_move)];
    memset(node_children, 0, sizeof(struct gtree_node *) * num_moves);

    // initialize tag data
    for (size_t i = 0; i < schema->num_tags; i++) {
        assert(schema->tags[i].tagtype);
        
        if (!schema->tags[i].tagtype->_init) {
            continue;
        }

        const size_t statetag_offset = schema->tags[i].statetag_offset;
        const size_t movetags_offset = schema->movedata_base_offset +
            schema->tags[i].movetags_offset_per_move * num_moves;

        void *statetag = &_vdata[statetag_offset];
        void *movetags = &_vdata[movetags_offset];

        schema->tags[i].tagtype->_init(statetag, movetags, node);
    }

    // update tree maximum depth counter
    if (node->reldepth - node->tree->reldepth > node->tree->maxdepth) {
        node->tree->maxdepth = node->reldepth - node->tree->reldepth;
    }

    return node;
}

static bool _delete_subtree(struct gtree_node *node) {
    assert(node);
    assert(node->tree);
    assert(node->tree->schema);

    const struct gtree_schema *schema = node->tree->schema;

    size_t stack_size = node->tree->maxdepth - node->reldepth + 1;
    size_t stack_top = 0;
    struct _stackentry {
        struct gtree_node *node;
        size_t move_index;
    } *stack = malloc(stack_size * sizeof(struct _stackentry));
    if (!stack) {
        return false;
    }

    stack[0].node = node;
    stack[0].move_index = 0;
    stack_top++;

    // perform postorder traversal of subtree
    while (stack_top) {
        struct _stackentry *top = &stack[stack_top];
        if (top->move_index >= top->node->num_moves) {
            // pop current node
            stack_top--;

            // delete tag data
            for (size_t i = 0; i < schema->num_tags; i++) {
                assert(schema->tags[i].tagtype);
                
                if (!schema->tags[i].tagtype->_free) {
                    continue;
                }

                const size_t statetag_offset = schema->tags[i].statetag_offset;
                const size_t movetags_offset = schema->movedata_base_offset +
                    schema->tags[i].movetags_offset_per_move * top->node->num_moves;

                uint8_t *_vdata = (void*) top->node->_vdata;
                void *statetag = &_vdata[statetag_offset];
                void *movetags = &_vdata[movetags_offset];

                schema->tags[i].tagtype->_free(statetag, movetags);
            }

            free(top->node);
        }
        else {
            // push next child
            struct gtree_node **children = (void*)
                &top->node->_vdata[schema->movedata_base_offset + 
                                   top->node->num_moves * sizeof(go_move)];
            if (children[top->move_index]) {
                stack_top++;
                stack[stack_top].node = children[top->move_index];
                stack[stack_top].move_index = 0;
            }
            top->move_index++;
        }
    }

    free(stack);
    return true;
}

struct gtree_node *gtree_child(struct gtree_node *node, go_move move, bool expand) {
    assert(node);

    const int index = gtree_move_index(node, move);
    if (index < 0) {
        return NULL;
    }

    const struct gtree_schema *schema = node->schema;
    struct gtree_node **children = (void*) 
        &node->_vdata[schema->movedata_base_offset + node->num_moves * sizeof(go_move)];
    
    if (children[index]) {
        return children[index];
    }
    else if (!expand) {
        return NULL;
    }
    else {
        // expand child node
        struct gtree_node *child = _create_node(node, move);
        if (!child) {
            // error creating child
            return NULL;
        }

        children[index] = child;
        return child;
    }
}

void gtree_prune(struct gtree_node *node, go_move move) {
    struct gtree_node *child = gtree_child(node, move, false);
    if (!child) {
        return;
    }
    _delete_subtree(child);
}

void *gtree_statetag(struct gtree_node *node, int tagid) {
    assert(node);
    assert(node->tree);
    assert(node->tree->schema);
    assert(tagid >= 0);

    const struct gtree_schema *schema = node->tree->schema;
    
    if ((size_t) tagid >= schema->num_tags) {
        return NULL;
    }

    size_t statetag_offset = schema->tags[tagid].statetag_offset;
    return (void*) &node->_vdata[statetag_offset];
}

void *gtree_movetag(struct gtree_node *node, int tagid, go_move move) {
    assert(node);
    assert(node->tree);
    assert(node->tree->schema);
    assert(tagid >= 0);

    const int index = gtree_move_index(node, move);
    if (index < 0) {
        return NULL;
    }

    const struct gtree_schema *schema = node->tree->schema;

    if ((size_t) tagid >= schema->num_tags) {
        return NULL;
    }

    size_t movetag_offset = schema->movedata_base_offset +
        schema->tags[tagid].movetags_offset_per_move * node->num_moves +
        schema->tags[tagid].movetag_size * index;

    return (void*) &node->_vdata[movetag_offset];
}

void *gtree_movetags(struct gtree_node *node, int tagid) {
    assert(node);
    assert(node->tree);
    assert(node->tree->schema);
    assert(tagid >= 0);

    const struct gtree_schema *schema = node->tree->schema;

    if ((size_t) tagid >= schema->num_tags) {
        return NULL;
    }

    size_t movetags_offset = schema->movedata_base_offset +
        schema->tags[tagid].movetags_offset_per_move * node->num_moves;

    return (void*) &node->_vdata[movetags_offset];
}

int gtree_depth(struct gtree_node *node) {
    assert(node);
    assert(node->tree);
    
    return node->reldepth - node->tree->reldepth;
}

int gtree_move_index(struct gtree_node *node, go_move move) {
    assert(node);   
   
    go_move *moves = (void*) &node->_vdata[node->schema->movedata_base_offset];
    
    // binary search through moves
    size_t lower = 0;
    size_t upper = node->num_moves-1;
    while (lower < upper) {
        size_t middle = (lower + upper) / 2;
        go_move middle_move = moves[middle];
        if (middle_move == move) {
            return middle;
        }
        if (middle_move < move) {
            lower = middle + 1;
        }
        else {
            upper = middle - 1;
        }
    }

    return -1;
}
