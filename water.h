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
#if !defined(_water_h_)
#define _water_h_
////////////////////////////
//
// Project: *current project*
// Module:  Water
//
// CreatedBy:
// CreatedOn: 2010
//
// Purpose
//   -- <desc>
//
// Uses
//   --
//
// Interface
//   --
//
#include <stdbool.h>

typedef void                  *H2oUserNode;
typedef void                  *H2oUserMark;
typedef void                  *H2oUserType;
typedef const char            *H2oUserName;
typedef struct water_location *H2oLocation;
typedef struct water_code     *H2oCode;
typedef struct water_thread   *H2oThread;
typedef struct water_cache    *H2oCache;
typedef struct water          *Water;

/* fetch the first child of this node (if any)*/
typedef bool (*H2oGetFirst)(Water, H2oLocation);
/* fetch the next child of this node (if any) */
typedef bool (*H2oGetNext)(Water, H2oLocation);
/* fetch the match root type*/
typedef bool (*H2oMatchNode)(Water, H2oUserType, H2oUserNode);

/* user defined predicate */
/* these are ONLY call while traversing the tree */
typedef bool (*H2oPredicate)(Water, H2oUserNode);

/* user defined event actions */
/* these are ONLY call after a sucessful traversal */
typedef bool (*H2oEvent)(Water, H2oUserNode);

typedef bool (*H2oFindType) (Water, H2oUserName, H2oUserType*);      // find the H2oUserType by name
typedef bool (*H2oFindCode) (Water, H2oUserName, H2oCode*);          // find the H2oCode by name
typedef bool (*H2oAddCode)  (Water, H2oUserName, H2oCode);           // add the  H2oCode to name
typedef bool (*H2oFindEvent)(Water, H2oUserName, H2oEvent*);         // find the H2oEvent by name
typedef bool (*H2oFindPredicate)(Water, H2oUserName, H2oPredicate*); // find the H2oPredicate by name

struct water_location {
    H2oUserNode    root;    // the current root           (if any)
    H2oUserMark    offset;  // the mark for this location (if any)
    H2oUserNode    current; // the current node
};

struct water {
    /* call-backs */
    H2oGetFirst      first;
    H2oGetNext       next;
    H2oMatchNode     match;
    H2oFindType      type;
    H2oFindCode      code;
    H2oAddCode       attach;
    H2oFindPredicate predicate;
    H2oFindEvent     event;

    /* data */
    struct water_location cursor;
    H2oThread begin;
    H2oThread end;
    H2oThread free_list;
    H2oCache  cache;
};

/*-------------------------------------------------------------------*/
#ifdef H2o_ARRAY_NODE_EXAMPLE

struct example_node {
    unsigned type;
    unsigned size;
    struct example_node* childern[];
};

static bool example_GetFirst(Water water, H2oLocation location) {
    if (!water)    return false;
    if (!location) return false;

    struct example_node *node = location->root;

    if (!node) return false;

    // if (!hasChildern(node->type)) return false;

    if (0 >= node->size) return false;

    location->offset  = (H2oUserMark) 0;
    location->current = (H2oUserNode) node->childern[0];

    return true;
}

static bool example_GetNext(Water water, H2oLocation location) {
    if (!water)    return false;
    if (!location) return false;

    struct example_node *node = location->root;

    if (!node) return false;

    // if (!hasChildern(node->type)) return false;

    unsigned index = (unsigned) location->offset;

    index += 1;

    if (index >= node->size) return false;

    location->offset  = (H2oUserMark) index;
    location->current = (H2oUserNode) node->childern[index];

    return true;
}

static bool example_MatchNode(Water water,  H2oUserType utype, H2oUserNode unode) {
    if (!water) return false;
    if (!unode)  return false;

    unsigned             type = (unsigned) utype;
    struct example_node *node = unode;

    return type == node->type;
}

#endif
/*-------------------------------------------------------------------*/

typedef enum water_operation {
    // node operations
    water_Any, // match any root
    water_And,
    water_Or,
    water_Not,
    water_Assert,
    water_Apply,
    water_Root,
    water_Childern,
    water_Leaf,

    water_Predicate,
    water_Event,

    // list operations
    water_Begin, // is this the begin
    water_Tuple,
    water_Select,
    water_Sequence,
    water_ZeroPlus,
    water_OnePlus,
    water_Maybe,
    water_Range,
    water_End,  // is this the and

    water_Void
} H2oOperation;

typedef enum water_cache_type {
    rule_cache,
    root_cache,
    event_cache,
    predicate_cache,
    cache_void,
} H2oCacheType;

typedef struct water_chain    *H2oChain;
typedef struct water_function *H2oFunction;
typedef struct water_action   *H2oAction;
typedef struct water_group    *H2oGroup;

// used by
// - water_Any
// - water_Begin
// - water_End
// - water_Leaf
struct water_code {
    H2oOperation oper;
    const char*  label;
};

// used by
// - water_And
// - water_Or
// - water_Tuple
// - water_Select
// - water_Sequence
struct water_chain {
    H2oOperation oper;
    const char*  label;
    H2oCode      before;
    H2oCode      after;
};

// used by
// - water_Not
// - water_Assert
// - water_ZeroPlus
// - water_OnePlus
// - water_Maybe
// - water_Childern
struct water_function {
    H2oOperation oper;
    const char*  label;
    H2oCode      argument;
};

// used by
// - water_Apply
// - water_Predicate
// - water_Event
// - water_Root
struct water_action {
    H2oOperation oper;
    const char*  label;
    unsigned     index;
    H2oCache     cache;
    H2oUserName  name;
};

// used by
// - water_Range
struct water_group {
    H2oOperation oper;
    const char*  label;
    H2oCode      argument;
    unsigned     minimum; // minimum < maximum unless maximum == zero
    unsigned     maximum; // zero means until GetNext returns false
};

struct water_cache {
    H2oCacheType type;
    H2oCache     next;
    unsigned     count;
    const char **names;
    void       **values;
};

static inline const char* oper2text(H2oOperation oper) {
    switch (oper) {
    case water_Any       : return "Any";
    case water_And       : return "And";
    case water_Or        : return "Or";
    case water_Not       : return "Not";
    case water_Assert    : return "Assert";
    case water_Apply     : return "Apply";
    case water_Root      : return "Root";
    case water_Childern  : return "Childern";
    case water_Leaf      : return "Leaf";
    case water_Predicate : return "Predicate";
    case water_Event     : return "Event";
    case water_Begin     : return "Begin";
    case water_Tuple     : return "Tuple";
    case water_Select    : return "Select";
    case water_Sequence  : return "Sequence";
    case water_ZeroPlus  : return "ZeroPlus";
    case water_OnePlus   : return "OnePlus";
    case water_Maybe     : return "Maybe";
    case water_Range     : return "Range";
    case water_End       : return "End";
    default: break;
    }
    return "unknown";
}

extern bool h2o_WaterInit(Water, unsigned cacheSize);
extern bool h2o_Parse(Water, const char* rule, H2oUserNode tree);
extern bool h2o_RunQueue(Water);

extern unsigned h2o_global_debug;
extern void     h2o_debug(const char *filename, unsigned int linenum, const char *format, ...);
extern void     h2o_error(const char *filename, unsigned int linenum, const char *format, ...);
extern void     h2o_error_part(const char *format, ...);

static inline void h2o_noop() __attribute__((always_inline));
static inline void h2o_noop() { return; }

#if 1
#define H2O_DEBUG(level, args...) ({ typeof (level) hold__ = (level); if (hold__ <= h2o_global_debug) h2o_debug(__FILE__,  __LINE__, args); })
#else
#define H2O_DEBUG(level, args...) h2o_noop()
#endif

#if 1
#define H2O_ON_DEBUG(level, arg) ({ typeof (level) hold__ = (level); if (hold__ <= h2o_global_debug) arg; })
#else
#define H2O_DEBUG(level, args...) h2o_noop()
#endif

#if 1
#define H2O_ERROR(args...) h2o_error(__FILE__,  __LINE__, args)
#else
#define H2O_ERROR(args...) h2o_noop()
#endif

#if 1
#define H2O_ERROR_PART(args...) h2o_error_part(args)
#else
#define H2O_ERROR_PART(args...) h2o_noop()
#endif

#if 1
#define H2O_ERROR_AT(args...) h2o_error(args)
#else
#define H2O_ERROR_AT(args...) h2o_noop()
#endif

// used internally ONLY
extern bool h2o_AddName(Water, const char*, const H2oCode);
extern bool h2o_AddCache(Water, H2oCache);

/////////////////////
// end of file
/////////////////////
#endif
