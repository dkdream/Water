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

extern bool let_wtree(Water water);
extern bool tree_ctree(Copper input);

struct static_table my_symbols;

extern size_t symbol_Make(CuData name) {
    if (!name.start) return 0;
    if (1 > name.length) return 0;

    StaticValue result;

    if (!stable_NFind(&my_symbols, name.start, name.length, &result)) {
        result = (StaticValue) strndup(name.start, name.length);
        stable_Replace(&my_symbols, (const char*) result, result);
    }

    return (size_t) result;
}

static bool findType(Water        water __attribute__ ((unused)),
                     H2oUserName  name,
                     H2oUserType* result)
{
    if (!name)  return false;

    printf("looking for %s\n", name);

    CuData cname;

    cname.start  = name;
    cname.length = strlen(name);

    *result = (H2oUserType) symbol_Make(cname);

    return true;
}

struct static_table my_codes;

static bool findCode(Water       water __attribute__ ((unused)),
                     H2oUserName name,
                     H2oCode*    target)
{
    StaticValue result;

    if (!stable_Find(&my_codes, name, &result)) return false;

    *target = (H2oCode) result;

    return true;
}

static bool setCode(Water       water __attribute__ ((unused)),
                    H2oUserName name,
                    H2oCode     value)
{
    return stable_Replace(&my_codes, name, (StaticValue) value);
}

struct static_table my_water_events;

static bool findWaterEvent(Water       water __attribute__ ((unused)),
                           H2oUserName name,
                           H2oEvent*   target)
{
    StaticValue result;

    if (!stable_Find(&my_water_events, name, &result)) return false;

    *target = (H2oEvent) result;

    return true;
}

static bool setWaterEvent(H2oUserName name,
                          H2oEvent    value)
{
    return stable_Replace(&my_water_events, name, (StaticValue) value);
}


struct static_table my_nodes;

static bool findNode(Copper  copper __attribute__ ((unused)),
                     CuName  name,
                     CuNode* target)
{
    StaticValue result;

    if (!stable_Find(&my_nodes, name, &result)) return false;

    *target = (CuNode) result;

    return true;
}

static bool setNode(Copper copper __attribute__ ((unused)),
                    CuName name,
                    CuNode value)
{
    return stable_Replace(&my_nodes, name, (StaticValue) value);
}

struct static_table my_copper_events;

static bool findCopperEvent(Copper   copper __attribute__ ((unused)),
                            CuName   name,
                            CuEvent* target)
{
    StaticValue result;

    if (!stable_Find(&my_copper_events, name, &result)) return false;

    *target = (CuEvent) result;

    return true;
}

static bool setCopperEvent(CuName  name,
                           CuEvent value)
{
    return stable_Replace(&my_copper_events, name, (StaticValue) value);
}

static unsigned index_level = 0;
static void indent() {
    if (index_level < 1) return;

    unsigned inx = index_level;

    for (; inx-- ;) {
        printf("  ");
    }
}

static bool begin_event(Water       water __attribute__ ((unused)),
                        H2oUserNode value)
{
    Node_test node = (Node_test) value;
    const char* type = (const char*) node->type;

    indent();
    printf("begin event %s %p\n", type, value);
    index_level += 1;
    return true;
}
static bool end_event(Water       water __attribute__ ((unused)),
                      H2oUserNode value)
{
    Node_test node = (Node_test) value;
    const char* type = (const char*) node->type;

    index_level -= 1;
    indent();
    printf("end event %s %p\n", type, value);
    return true;
}
static bool lets_event(Water       water __attribute__ ((unused)),
                       H2oUserNode value)
{
    indent();
    printf("lets event %p\n", value);
    return true;
}
static bool value_event(Water       water __attribute__ ((unused)),
                        H2oUserNode value)
{
    indent();
    printf("value event %p\n", value);
    return true;
}
static bool assign_event(Water       water __attribute__ ((unused)),
                         H2oUserNode value)
{
    indent();
    printf("assign event %p\n", value);
    return true;
}
static bool symbol_event(Water       water __attribute__ ((unused)),
                         H2oUserNode value)
{
    indent();
    printf("symbol event %p\n", value);
    return true;
}

static bool statement_event(Water       water __attribute__ ((unused)),
                            H2oUserNode value)
{
    Node_test node = (Node_test) value;
    const char* type = (const char*) node->type;

    indent();
    printf("statement event %s %p\n", type, value);
    return true;
}

static struct water the_walker;

static bool setup_walker() {
    memset(&the_walker, 0, sizeof(struct water));

    the_walker.first     = GetFirst_test;
    the_walker.next      = GetNext_test;
    the_walker.match     = MatchNode_test;
    the_walker.type      = findType;
    the_walker.code      = findCode;
    the_walker.attach    = setCode;
    the_walker.event     = findWaterEvent;

    stable_Init(1024, &my_codes);
    stable_Init(1024, &my_water_events);

    h2o_WaterInit(&the_walker, 1024);

    setWaterEvent("begin",     begin_event);
    setWaterEvent("end",       end_event);
    setWaterEvent("lets",      lets_event);
    setWaterEvent("value",     value_event);
    setWaterEvent("assign",    assign_event);
    setWaterEvent("symbol",    symbol_event);
    setWaterEvent("statement", statement_event);

    let_wtree(&the_walker);

    return true;
}

static bool run_walker(Node_test value) {

    node_Print(1, value);

    fprintf(stderr, "test node %x type %Zu\n", (unsigned) value, value->type);

    h2o_global_debug = 4;

    if (!h2o_Parse(&the_walker, "Start", value)) {
        fprintf(stderr, "unable to parse");
        return false;
    }

    if (!h2o_RunQueue(&the_walker)) {
        fprintf(stderr, "unable to run");
        return false;
    }

    return true;
}

static char buffer[4096];
static const char* convert(CuData name) {
    strncpy(buffer, name.start, name.length);
    buffer[name.length] = 0;
    return buffer;
}

static struct test_stack the_marks;
static struct test_stack the_trees;

static bool push_tree(const char *name, unsigned childern) {
    CuData    cname;
    Node_test value;

    cname.start  = name;
    cname.length = strlen(name);

    if (!node_Create(&value, cname, childern, &the_trees)) return false;

    return stack_PushNode(&the_trees, value);
}

static bool begin_childern_event(Copper   file __attribute__ ((unused)),
                                 CuCursor at   __attribute__ ((unused)))
{
    size_t depth = 0;

    stack_Depth(&the_trees, &depth);
    stack_PushMark(&the_marks, depth);

    return true;
}
static bool end_childern_event(Copper   file __attribute__ ((unused)),
                               CuCursor at   __attribute__ ((unused)))
{
    size_t bottom  = 0;
    size_t current = 0;
    size_t delta   = 0;

    stack_PopMark(&the_marks, &bottom);
    stack_Depth(&the_trees, &current);

    if (bottom > current) return false;

    delta = current - bottom;

    return push_tree("Block", current - bottom);
}
static bool push_tree_event(Copper   file __attribute__ ((unused)),
                            CuCursor at   __attribute__ ((unused)))
{
    Node_test block;
    Node_test symbol;

    stack_PopNode(&the_trees, &block);
    stack_PopNode(&the_trees, &symbol);

    block->type = symbol->type;

    free(symbol);

    return stack_PushNode(&the_trees, block);
}
static bool push_symbol_event(Copper   file,
                              CuCursor at   __attribute__ ((unused)))
{
    CuData    cname;
    Node_test value;

    if (!cu_MarkedText(file, &cname)) return false;

    if (!node_Create(&value, cname, 0, &the_trees)) return false;

    return stack_PushNode(&the_trees, value);
}
static bool push_string_event(Copper file, CuCursor at)
{
    push_symbol_event(file, at);

    return push_tree("String", 1);
}
static bool push_number_event(Copper file, CuCursor at)
{
    push_symbol_event(file, at);

    return push_tree("Number", 1);
}

static struct copper the_parser;

static bool setup_parser() {
    memset(&the_parser, 0, sizeof(struct copper));

    the_parser.node   = findNode;
    the_parser.attach = setNode;
    the_parser.event  = findCopperEvent;

    stable_Init(1024, &my_nodes);
    stable_Init(1024, &my_copper_events);

    stack_Init(100, &the_marks);
    stack_Init(100, &the_trees);

    cu_InputInit(&the_parser, 1024);

    /* add events and predicates */
    setCopperEvent("begin_childern", begin_childern_event);
    setCopperEvent("end_childern",   end_childern_event);
    setCopperEvent("push_tree",      push_tree_event);
    setCopperEvent("push_symbol",    push_symbol_event);
    setCopperEvent("push_string",    push_string_event);
    setCopperEvent("push_number",    push_number_event);

    /* add nodes */
    tree_ctree(&the_parser);

    /* fill meta data */
    cu_FillMetadata(&the_parser);

    /* set start node */
    cu_Start("file", &the_parser);

    return true;
}

static bool run_parser(FILE *input) {
    CuData data      = { 0, 0 };
    char  *buffer    = 0;
    size_t allocated = 0;

    for ( ; ; ) {
        switch(cu_Event(&the_parser, data)) {
        case cu_NeedData:
            data.length = getline(&buffer, &allocated, input);
            if (data.length >= 0) {
                data.start = buffer;
            } else {
                data.length = -1;
                data.start  = 0;
                if (ferror(input)) {
                    fprintf(stderr, "no more data\n");
                    break;
                }
            }
            continue;

        case cu_FoundPath:
            if (!cu_RunQueue(&the_parser)) {
                fprintf(stderr, "event error\n");
            }
            break;

        case cu_NoPath:
            fprintf(stderr, "syntax error\n");
            //cu_SyntaxError(stderr, the_parser, infile);
            break;

        case cu_Error:
            fprintf(stderr, "hard error\n");
            break;
        }

        break;
    }

    return true;
}

int main(int    argc,
         char **argv)
{
    FILE *input = 0;

    stable_Init(1024, &my_symbols);

    setup_parser();
    setup_walker();

    if (argc > 1) {
        printf("input = %s\n", argv[1]);
        input = fopen(argv[1], "r");
    }

    if (input) {
        run_parser(input);
    } else {
        push_tree("Value", 0);
        push_tree("Symbol", 0);
        push_tree("ParameterName", 1);
        push_tree("LetAssign", 2);
        stack_Dup(&the_trees);
        push_tree("Statement", 0);
        push_tree("Let", 3);
        stack_Dup(&the_trees);
        push_tree("Block", 2);
    }

    if (!the_trees.top) return 0;

    Node_test value = the_trees.top->value;

    node_Print(0, value);

    if (!run_walker(value)) return 1;

    return 0;
}

/*****************
 ** end of file **
 *****************/

