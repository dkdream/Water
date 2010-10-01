/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "nodes.h"

#include <static_table.h>

#include <stdio.h>

extern bool let_graph(Water water);

typedef struct name_value {
    const char *name;
    unsigned    value;
} NVPair;

static const NVPair let       = { "Let",           1 };
static const NVPair assign    = { "LetAssign",     2 };
static const NVPair value     = { "Value",         3 };
static const NVPair parameter = { "ParameterName", 4 };
static const NVPair symbol    = { "Symbol",        5 };
static const NVPair statement = { "Statement",     6 };
static const NVPair block     = { "Block",     7 };

static const NVPair* type_map[] = { &let, &assign, &value, &parameter, &symbol, &statement, &block, 0 };

static const char* type2name(unsigned type) {
    unsigned index = 0;

    for ( ; type_map[index] ; ++index) {
        if (type == type_map[index]->value) {
            return type_map[index]->name;
        }
    }

    return 0;
}

static bool findType(Water water, H2oUserName name, H2oUserType* result) {
    if (!water) return false;
    if (!name)  return false;

    unsigned index = 0;
    for ( ; type_map[index] ; ++index) {
        if (strcmp(type_map[index]->name, name)) continue;
        *result = (H2oUserType) type_map[index]->value;
        return true;
    }

    return false;
}

struct static_table my_codes;

static bool findCode(Water water, H2oUserName name, H2oCode* target) {
    StaticValue result;

    if (!stable_Find(&my_codes, name, &result)) return false;

    *target = (H2oCode) result;

    return true;
}

static bool setCode(Water water, H2oUserName name, H2oCode value) {
    return stable_Replace(&my_codes, name, (StaticValue) value);
}

struct static_table my_events;

static bool findEvent(Water water, H2oUserName name, H2oEvent* target) {
    StaticValue result;

    if (!stable_Find(&my_events, name, &result)) return false;

    *target = (H2oEvent) result;

    return true;
}

static bool setEvent(Water water, H2oUserName name, H2oEvent value) {
    return stable_Replace(&my_events, name, (StaticValue) value);
}

struct static_table my_predicates;

static bool findPredicate(Water water, H2oUserName name, H2oPredicate* target) {
    StaticValue result;

    if (!stable_Find(&my_predicates, name, &result)) return false;

    *target = (H2oPredicate) result;

    return true;
}

static bool begin_event(Water water, H2oUserNode value) {
    printf("begin\n");
    return true;
}
static bool end_event(Water water, H2oUserNode value) {
    printf("end\n");
    return true;
}
static bool lets_event(Water water, H2oUserNode value) {
    printf("lets\n");
    return true;
}
static bool value_event(Water water, H2oUserNode value) {
    printf("value\n");
    return true;
}
static bool assign_event(Water water, H2oUserNode value) {
    printf("assign\n");
    return true;
}
static bool symbol_event(Water water, H2oUserNode value) {
    printf("symbol\n");
    return true;
}

static bool statement_event(Water water, H2oUserNode value) {
    printf("statement\n");
    return true;
}

struct water the_walker;

static bool setup_walker() {
    the_walker.first     = GetFirst_test;
    the_walker.next      = GetNext_test;
    the_walker.match     = MatchNode_test;
    the_walker.type      = findType;
    the_walker.code      = findCode;
    the_walker.attach    = setCode;
    the_walker.predicate = findPredicate;
    the_walker.event     = findEvent;

    stable_Init(1024, &my_codes);
    stable_Init(1024, &my_events);
    stable_Init(1024, &my_predicates);

    h2o_WaterInit(&the_walker, 1024);

    setEvent(&the_walker, "begin",  begin_event);
    setEvent(&the_walker, "end",    end_event);
    setEvent(&the_walker, "lets",   lets_event);
    setEvent(&the_walker, "value",  value_event);
    setEvent(&the_walker, "assign", assign_event);
    setEvent(&the_walker, "symbol", symbol_event);
    setEvent(&the_walker, "statement", statement_event);

    let_graph(&the_walker);

    return true;
}

static struct test_stack the_stack;

static bool push_tree(const char *name, unsigned childern) {
    H2oUserType type;

    if (!findType(&the_walker, name, &type)) return false;

    fprintf(stderr, "constructing node %s %u\n", name, (unsigned) type);

    Node_test value;

    if (!node_Create(&value, (unsigned) type, childern, &the_stack)) return false;

    return stack_Push(&the_stack, value);
}

int main(int argc, char **argv)
{
    setup_walker();

    stack_Init(100, &the_stack);

    push_tree("Value", 0);
    push_tree("Symbol", 0);
    push_tree("ParameterName", 1);
    push_tree("LetAssign", 2);
    stack_Dup(&the_stack);
    push_tree("Statement", 0);
    push_tree("Let", 3);
    stack_Dup(&the_stack);
    push_tree("Block", 2);

    Node_test value;

    stack_Pop(&the_stack, &value);

    node_Print(1, type2name, value);

    fprintf(stderr, "test node %x type %u\n", (unsigned) value, value->type);

    h2o_global_debug = 4;

    if (!h2o_Parse(&the_walker, "Start", value)) {
        fprintf(stderr, "unable to parse");
        return 1;
    }

    if (!h2o_RunQueue(&the_walker)) {
        fprintf(stderr, "unable to run");
        return 1;
    }

    return 0;
}

/*****************
 ** end of file **
 *****************/

