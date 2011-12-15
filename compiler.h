/*-*- mode: c;-*-*/
/**********************************************************************
This file is part of Water.

Water is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Water is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Water.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/
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
typedef struct water_symbol   *H2oSymbol;
typedef struct water_table    *H2oTable;
typedef struct water_buffer   *H2oBuffer;
typedef struct water_parser   *H2oParser;

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
    water_any,
    water_and,
    water_assert,
    water_childern,
    water_count,
    water_define,
    water_event,
    water_identifer,
    water_label,
    water_leaf,
    water_maybe,
    water_not,
    water_one_plus,
    water_or,
    water_predicate,
    water_range,
    water_select,
    water_sequence,
    water_tuple,
    water_zero_plus,
    //-------
    water_void
} H2oType;

// use for
// and as a generic node
// - leaf
struct water_any {
    H2oType  type;
    unsigned id;
};

// use for
// - identifier = ....
struct water_define {
    H2oType   type;
    unsigned  id;
    H2oDefine next;
    CuData    name;
    H2oNode   match;
};

// use for
// - identifer
// - label
// - event
// - predicate
struct water_text {
    H2oType  type;
    unsigned id;
    CuData   value;
    unsigned index;
};

// use for
// - e !
// - e &
// - e +
// - e *
// - e ?
// - childern
struct water_operator {
    H2oType  type;
    unsigned id;
    H2oNode  value;
};

// use for
// - number
struct water_count {
    H2oType  type;
    unsigned id;
    unsigned count;
};

// use for
// - e [min,max]
struct water_range {
    H2oType  type;
    unsigned id;
    H2oNode  value;
    unsigned min; // assert(min < max) where max != 0
    unsigned max; // zero = infinity
};

// use for
// - e1 e2 select
// - e1 e2 tuple
// - e1 e2 sequence
// - e1 e2 and
// - e1 e2 or
struct water_branch {
    H2oType  type;
    unsigned id;
    H2oNode  before;
    H2oNode  after;
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

struct water_symbol {
    CuData    name;
    unsigned  index;
    H2oSymbol next;
};

struct water_table {
    struct static_table map;
    unsigned            count;
    H2oSymbol           last;
};

struct water_parser {
    struct copper base;

    struct static_table node;
    struct static_table action;

    // parsing context
    struct water_stack  stack;
    struct water_buffer buffer;

    FILE* output;

    struct water_table identifer; // water_Apply
    struct water_table label;     // water_Root
    struct water_table event;     // water_Event
    struct water_table predicate; // water_Predicate

    H2oDefine rule;
};

extern unsigned int h2o_global_debug;

extern bool water_Create(const char* infile, const char* outfile, H2oParser *target);
extern void water_Parse(H2oParser water, const char* name);
extern bool water_Free(H2oParser value);

#endif
