/*-*- mode: c;-*-*/
#if !defined(_syntax_h_)
#define _syntax_h_
/***************************
 **
 ** Project: *current project*
 **
 ** Datatype List:
 **    <routine-list-end>
 **
 **
 **
 ***/
#include "water.h"
#include <copper.h>
#include <static_table.h>

/* */
typedef struct water_any      *H2oAny;
typedef struct water_define   *H2oDefine;
typedef struct water_text     *H2oText;
typedef struct water_operator *H2oOperator;
typedef struct water_count    *H2oCount;
typedef struct water_range    *H2oRange;
typedef struct water_branch   *H2oBranch;
typedef struct water_assign   *H2oAssign;
typedef struct water_tree     *H2oTree;
/* */
typedef struct water_cell     *H2oCell;
typedef struct water_stack    *H2oStack;
typedef struct water_buffer   *H2oBuffer;
typedef struct water          *Water;

union water_node {
    H2oAny      any;
    H2oDefine   define;
    H2oText     text;
    H2oOperator operator;
    H2oCount    count;
    H2oRange    range;
    H2oBranch   branch;
    H2oAssign   assign;
    H2oTree     tree;
} __attribute__ ((__transparent_union__));

typedef union water_node H2oNode;

union water_target {
    H2oAny      *any;
    H2oDefine   *define;
    H2oText     *text;
    H2oOperator *operator;
    H2oCount    *count;
    H2oRange    *range;
    H2oBranch   *branch;
    H2oAssign   *assign;
    H2oTree     *tree;
    H2oNode     *node;
} __attribute__ ((__transparent_union__));

typedef union water_target H2oTarget;

typedef enum water_type {
    water_define,
    water_identifer,
    water_label,
    water_event,
    water_predicate,
    water_not,
    water_assert,
    water_and,
    water_or,
    water_zero_plus,
    water_one_plus,
    water_maybe,
    water_count,
    water_range,
    water_select,
    water_tuple,
    water_assign,
    water_leaf,
    water_tree,
    water_sequence,
    //-------
    water_void
} H2oType;

// use for
// and as a generic node
struct water_any {
    H2oType type;
};

// use for
// - identifier = ....
struct water_define {
    H2oType   type;
    H2oDefine next;
    H2oText   name;
    H2oNode   match;
};

// use for
// - identifer
// - label
// - event
// - predicate
struct water_text {
    H2oType  type;
    PrsData  value;
};

// use for
// - e !
// - e &
// - e +
// - e *
// - e ?
// - leaf
struct water_operator {
    H2oType type;
    H2oNode value;
};

// use for
// - number
struct water_count {
    H2oType  type;
    unsigned count;
};

// use for
// - e [min,max]
struct water_range {
    H2oType  type;
    H2oNode  value;
    H2oCount min; // assert(min < max) where max != 0
    H2oCount max; // zero = infinity
};

// use for
// - e1 e2 / (select)
// - e1 e2 ; (tuple)
// - sequence
struct water_branch {
    H2oType type;
    H2oNode before;
    H2oNode after;
};

// use for
// - LABEL -> EVENT
struct water_assign {
    H2oType type;
    H2oText label;
    H2oText event;
};

// use for
// - tree
struct water_tree {
    H2oType  type;
    H2oNode  reference;
    H2oNode  childern;
};

/*------------------------------------------------------------*/

struct water_buffer {
    const char *filename;
    FILE       *input;
    unsigned    cursor;
    ssize_t     read;
    size_t      allocated;
    char       *line;
};

// use for the node stack
struct water_cell {
    H2oNode value;
    H2oCell next;
};

struct water_stack {
    H2oCell top;
    H2oCell free_list;
};

struct water {
    struct prs_input base;

    struct static_table node;
    struct static_table event;

    // parsing context
    struct water_stack  stack;
    struct water_buffer buffer;

    H2oDefine rule;
};

extern unsigned int h2o_global_debug;

#endif
