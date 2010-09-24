#if !defined(_topaz_h_)
#define _topaz_h_
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

typedef void                  *UserNode;
typedef void                  *UserMark;
typedef void                  *UserType;
typedef const char            *UserName;
typedef struct water_location *H2oLocation;
typedef struct water_code     *H2oCode;
typedef struct water_thread   *H2oThread;
typedef struct water          *Water;

/* fetch the first child of this node (if any)*/
typedef bool (*GetFirst)(Water, H2oLocation);
/* fetch the next child of this node (if any) */
typedef bool (*GetNext)(Water, H2oLocation);
/* fetch the match root type*/
typedef bool (*MatchNode)(Water, UserType, UserNode);

/* user defined predicate */
/* these are ONLY call while traversing the tree */
typedef bool (*H2oPredicate)(Water, UserNode);

/* user defined event actions */
/* these are ONLY call after a sucessful traversal */
typedef bool (*H2oEvent)(Water, UserNode);

typedef bool (*FindType) (Water, UserName, UserType*);
typedef bool (*FindCode) (Water, UserName, H2oCode*);          // find the H2oCode by name
typedef bool (*AddCode)  (Water, UserName, H2oCode);           // add the  H2oCode to name
typedef bool (*FindEvent)(Water, UserName, H2oEvent*);         // find the H2oEvent by name
typedef bool (*FindPredicate)(Water, UserName, H2oPredicate*); // find the H2oPredicate by name

struct water_location {
    UserNode    root;    // the current root           (if any)
    UserMark    offset;  // the mark for this location (if any)
    UserNode    current; // the current node
};

struct water {
    /* call-backs */
    GetFirst      first;
    GetNext       next;
    MatchNode     match;
    FindType      type;
    FindCode      code;
    AddCode       attach;
    FindPredicate predicate;
    FindEvent     event;

    /* data */
    struct water_location cursor;
    H2oThread begin;
    H2oThread end;
    H2oThread free_list;
};

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

typedef void                  *H2oIndex;
typedef struct water_chain    *H2oChain;
typedef struct water_function *H2oFunction;
typedef struct water_action   *H2oAction;
typedef struct water_group    *H2oGroup;

// used by
// - water_Any
// - water_Begin
// - water_End
struct water_code {
    H2oOperation oper;
};

// used by
// - water_And
// - water_Or
// - water_Tuple
// - water_Select
// - water_Sequence
struct water_chain {
    H2oOperation oper;
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
    H2oCode      argument;
};

// used by
// - water_Apply
// - water_Root
// - water_Predicate
// - water_Event
struct water_action {
    H2oOperation oper;
    H2oIndex     index;
};

// used by
// - water_Range
struct water_group {
    H2oOperation oper;
    H2oCode      argument;
    unsigned     minimum; // minimum < maximum unless maximum == zero
    unsigned     maximum; // zero means until GetNext returns false
};


/////////////////////
// end of file
/////////////////////
#endif
