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
#if 0
typedef void                  *UserNode;
typedef struct water_list     *H2oList;
typedef struct water_label    *H2oLabel;
typedef struct water_thread   *H2oThread;

struct water_list {
    H2oList  before;
    UserNode value;
};

/* user define event actions */
/* these are ONLY call after a sucessful parse completes */
typedef bool (*H2oEvent)(H2oInput, UserNode);

struct water_label {
    H2oEvent    function;
    const char* name; // debugging
};

struct water_thread {
    H2oThread next;
    H2oLabel  label;
    UserNode  cursor;
};

struct water_variable {
    const char* name;
    H2oList     value;
};
#endif

/////////////////////
// end of file
/////////////////////
#endif
