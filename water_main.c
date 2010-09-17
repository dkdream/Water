
#include "water.h"
#include <copper.h>
#include <hg_extended.h>
#include <static_table.h>
#include <hg_symbol.h>
#include <hg_stack.h>
#include <hg_count.h>
#include <hg_syntax.h>
#include <hg_treadmill.h>

/* */
#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>

/* from water.c */
extern bool water_graph(PrsInput input);

/* */
unsigned int h2o_global_debug = 0;

/* */
static char*  program_name = 0;
static Symbol parser_state_type = 0;

typedef struct getline_buffer {
    const char *filename;
    FILE       *input;
    unsigned    curser;
    ssize_t     read;
    size_t      allocated;
    char       *line;
} TextBuffer;

#define PARSER_MAX_ROOTS 20

typedef struct parse_state {
    struct user_extended header;
    /**
     * there are five stack
     * o. the parse stack hold all the current parse nodes
     * o. the mark stack hold the current marks use to
     *    construct tree nodes for the parse stack
     */
    Stack    mark;
    Stack    parse;

    // list of gc visiable temps for use in parser actions
    unsigned live;
    Node     temp[PARSER_MAX_ROOTS];
} *ParserState;

typedef struct water {
    struct prs_input    base;
    struct static_table node;
    struct static_table predicate;
    struct static_table event;
    struct parse_state  state;
    TextBuffer buffer;
} *Water;

static bool parser_state_Scanner(const Extended node) {
    if (isNil(node)) return true;

    ParserState state = (ParserState) node;

    if (!node_Live(state->mark))  return false;
    if (!node_Live(state->parse)) return false;

    unsigned inx = state->live;
    for ( ; inx ; ) {
        if (!node_Live(state->temp[--inx]))  return false;
    }

    return true;
}

static Symbol register_ParserState() {
     if (parser_state_type) return parser_state_type;

     Symbol hold;

     if (!symbol_Make("water.ParserState", &hold)) {
         VM_ERROR("unable to create water.ParserState name");
         return false;
     }
     if (!extended_Register(hold,
                            parser_state_Scanner,
                            sizeof(struct parse_state))) {
         VM_ERROR("unable to register water.ParserState");
         return (Symbol)0;
     }

     parser_state_type = hold;

     return  parser_state_type;
}

static bool parser_state_ExternalInit(const ParserState state) {

    if (!extended_ExternalInit(register_ParserState(), (const Extended)state)) {
        VM_ERROR("unable initialize water.ParserState header");
        return false;
    }

    if (!stack_Create(&state->mark)) {
        VM_ERROR("unable to create water.ParserState mark stack");
        return false;
    }
    if (!stack_Create(&state->parse)) {
        VM_ERROR("unable to create water.ParserState parse stack");
        return false;
    }

    return true;
}

static bool parser_Live(Water water, unsigned int count, Node** local) {
    if (!water) return false;
    if (!local) return false;

    ParserState state = &water->state;

    if (count > PARSER_MAX_ROOTS) {
        state->live = 0;
        return false;
    }

    int inx = 0;
    for ( ; inx < count ; ++inx) {
        state->temp[inx] = NIL;
    }

    state->live = count;
    *local = state->temp;

    return true;
}

static void print_State(Water water, bool start, const char* format, ...) {
    if (3 > h2o_global_debug) return;

    Stack parse = water->state.parse;
    Stack mark  = water->state.mark;

    va_list arguments;
    va_start(arguments, format);

    if (start) {
        printf("----------------------------------------------");
        vfprintf(stdout, format, arguments);
        printf("%s", "\n");
    }

    va_end(arguments);

    if (start) {
        if (5 < h2o_global_debug) {
            printf(" parse: "); stack_Print(stdout, parse); printf("\n");
            printf(" mark:  "); stack_Print(stdout, mark); printf("\n");
            printf("--------\n");
        }
    } else {
        printf(" parse: "); stack_Print(stdout, parse); printf("\n");
        printf(" mark:  "); stack_Print(stdout, mark); printf("\n");
        printf("----------------------------------------------\n");
    }
}


/***********************************************************/
/* *** parsing call-back functions *** */
/***********************************************************/

static bool water_MoreData(PrsInput parser)
{
    Water water = (Water) parser;

    TextBuffer *tbuffer = &water->buffer;

    if (tbuffer->curser >= tbuffer->read) {
        int read = getline(&tbuffer->line, &tbuffer->allocated, tbuffer->input);
        if (read < 0) return false;
        tbuffer->curser = 0;
        tbuffer->read   = read;
    }

    if (0 < h2o_global_debug) {
        unsigned line_number = parser->cursor.line_number + 1;
        printf("%s[%d] %s", tbuffer->filename, line_number, tbuffer->line);
    }

    unsigned  count = tbuffer->read - tbuffer->curser;
    const char *src = tbuffer->line + tbuffer->curser;

    if (!cu_AppendData(parser, count, src)) return false;

    tbuffer->curser += count;

    return true;
}


static bool water_FindNode(PrsInput parser, PrsName name, PrsNode* target) {
    Water water = (Water) parser;

    StaticValue result;

    if (!stable_Find(&water->node, name, &result)) return false;

    *target = (PrsNode) result;

    return true;
}

static bool water_FindPredicate(PrsInput parser, PrsName name, PrsPredicate* target) {
    Water water = (Water) parser;

    StaticValue result;

    if (!stable_Find(&water->predicate, name, &result)) return false;

    *target = (PrsPredicate) result;

    return true;
}

static bool water_FindEvent(PrsInput parser, PrsName name, PrsEvent* target) {
    Water water = (Water) parser;

    StaticValue result;

    if (!stable_Find(&water->event, name, &result)) return false;

    *target = (PrsEvent) result;

    return true;
}

static bool water_AddName(PrsInput parser, PrsName name, PrsNode value) {
    Water water = (Water) parser;

    return stable_Replace(&water->node, name, (StaticValue) value);
}

static bool water_SetEvent(Water water, PrsName name, PrsEvent value) {
    return stable_Replace(&water->event, name, (StaticValue) value);
}

/***********************************************************/

static bool tag_node(Water water, const char* type) {
    Symbol symbol = 0;

    if (!symbol_Make(type, &symbol)) return false;

    Stack parse = water->state.parse;
    Node    *at = 0;

    parser_Live(water, 2, &at);

    if (!stack_Pop(parse, at)) return false;

    if (!syntax_Create(symbol, 1, at, &at[1].syntax)) return false;

    if (!stack_Push(parse, at[1])) return false;

    return true;
}

static bool tag_collection(Water water, unsigned count, const char* type) {
    Symbol symbol = 0;

    if (!symbol_Make(type, &symbol)) return false;

    Stack parse = water->state.parse;
    Node    *at = 0;

    parser_Live(water, 2, &at);

    if (!stack_Collect(parse, count, &at[0].list)) {
        VM_ERROR("error collecting %s", type);
        return false;
    }

    if (!syntax_Create(symbol, 1, at, &at[1].syntax)) return false;

    if (!stack_Push(parse, at[1])) return false;

    return true;
}

/***********************************************************/

static bool define_event(PrsInput input, PrsCursor location) {
    const char *event_name = "define";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    if (!tag_collection(water, 2, "define")) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool end_event(PrsInput input, PrsCursor location) {
    const char *event_name = "end";
    const char *rule_name  = "unknown";

    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    Stack     mark = water->state.mark;
    Stack    parse = water->state.parse;
    Node       *at = 0;

    parser_Live(water, 1, &at);

    if (!stack_Pop(mark, at)) {
        VM_ERROR("no start to collect from");
        return false;
    }

    unsigned depth = at[0].count->value;

    if (!stack_Contract(parse, depth, &at[0].list)) {
        VM_ERROR("error collecting list");
        return false;
    }

    stack_Push(parse, at[0]);

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool identifier_event(PrsInput input, PrsCursor location) {
    const char *event_name = "identifier";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    PrsData text;

    if (!cu_MarkedText(input, &text)) return false;

    Stack parse = water->state.parse;
    Node *at    = 0;

    parser_Live(water, 1, &at);

    symbol_Create(text, &at[0].symbol);

    stack_Push(parse, at[0]);

    tag_node(water, "identifier");

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool label_event(PrsInput input, PrsCursor location) {
    const char *event_name = "label";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    PrsData text;

    if (!cu_MarkedText(input, &text)) return false;

    Stack parse = water->state.parse;
    Node *at    = 0;

    parser_Live(water, 1, &at);

    symbol_Create(text, &at[0].symbol);

    stack_Push(parse, at[0]);

    tag_node(water, "label");

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool leaf_event(PrsInput input, PrsCursor location) {
    const char *event_name = "leaf";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    tag_node(water, "leaf");

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool maybe_event(PrsInput input, PrsCursor location) {
    const char *event_name = "maybe";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    tag_node(water, "maybe");

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool min_max_event(PrsInput input, PrsCursor location) {
    const char *event_name = "min_max";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    if (!tag_collection(water, 2, "min:max")) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool number_event(PrsInput input, PrsCursor location) {
    const char *event_name = "number";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    PrsData text;
    if (!cu_MarkedText(input, &text)) return false;

    Stack parse = water->state.parse;
    Node *at    = 0;

    parser_Live(water, 1, &at);

    unsigned long value = strtoul(text.start, 0, 10);

    if (UINT_MAX <= value) return false;

    if (!count_Create((unsigned) value, &at[0].count)) return false;

    stack_Push(parse, at[0]);

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool one_plus_event(PrsInput input, PrsCursor location) {
    const char *event_name = "one_plus";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    if (!tag_node(water, "one_plus")) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool range_event(PrsInput input, PrsCursor location) {
    const char *event_name = "range";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    if (!tag_collection(water, 2, "range")) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool reference_event(PrsInput input, PrsCursor location) {
    const char *event_name = "reference";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    if (!tag_node(water, "reference")) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool select_event(PrsInput input, PrsCursor location) {
    const char *event_name = "select";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    if (!tag_node(water, "select")) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool start_event(PrsInput input, PrsCursor location) {
    const char *event_name = "start";
    const char *rule_name  = "unknown";
    Water water = (Water) input;

    print_State(water, true, "%s", event_name);

    Stack     mark = water->state.mark;
    Stack    parse = water->state.parse;
    Node       *at = 0;
    unsigned depth = 0;

    parser_Live(water, 1, &at);

    stack_Depth(parse,  &depth);
    count_Create(depth, &at[0].count);
    stack_Push(mark,    at[0]);

    print_State(water, false, "%s", event_name);

    return true;
    (void) event_name;
    (void) rule_name;
}

static bool tree_event(PrsInput input, PrsCursor location) {
    const char *event_name = "tree";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    if (!tag_collection(water, 2, "tuple")) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool tuble_event(PrsInput input, PrsCursor location) {
    const char *event_name = "tuble";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);

    if (!tag_node(water, "tuple")) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}

static bool zero_plus_event(PrsInput input, PrsCursor location) {
    const char *event_name = "zero_plus";
    const char *rule_name  = "unknown";
    Water water = (Water) input;
    print_State(water, true, "%s", event_name);
    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
    (void) rule_name;
}



/***********************************************************/

static void usage(char *name)
{
    fprintf(stderr, "usage: water  [--verbose]+ --name c_func_name [--output outfile] [--file infile]\n");
    fprintf(stderr, "water [-v]+ -n c_func_name [-o outfile] [-f infile]\n");
    fprintf(stderr, "water [--help]\n");
    fprintf(stderr, "water [-h]\n");
    fprintf(stderr, "where <option> can be\n");
    fprintf(stderr, "  -h|--help    print this help information\n");
    fprintf(stderr, "  -v|--verbose be verbose\n");
    fprintf(stderr, "if no <infile> is given, input is read from stdin\n");
    fprintf(stderr, "if no <oufile> is given, output is written to stdou\n");
    exit(1);
}

static void doOpen(Water water, const char* name)
{
    water->buffer.filename = name;
    if (!(water->buffer.input = fopen(name, "r"))) {
        perror(name);
        exit(1);
    }
}

static void doParse(Water water)
{
    if (0 < h2o_global_debug) {
        printf("before parse\n");
    }

    if (!cu_Parse("file", (PrsInput)water)) {
        printf("syntax error\n");
        return;
    }

    if (0 < h2o_global_debug) {
        printf("before run\n");
    }

    if (!cu_RunQueue((PrsInput)water)) {
        printf("event error\n");
    }
 }

static void doClose(Water water)
{
    fclose(water->buffer.input);
    water->buffer.filename = 0;
    water->buffer.input    = 0;
}

int main(int argc, char **argv)
{
    program_name = basename(argv[0]);

    const char* infile   = 0;
    const char* outfile  = 0;
    const char* funcname = 0;

    unsigned do_trace  = 0;
    unsigned do_debug  = 0;

    static struct option long_options[] = {
        {"trace",   0, 0, 't'},
        {"verbose", 0, 0, 'v'},
        {"help",    0, 0, 'h'},
        {"file",    1, 0, 'f'},
        {"output",  1, 0, 'o'},
        {"name",    1, 0, 'n'},
        {0, 0, 0, 0}
    };

    int chr;
    int option_index = 0;

    while (-1 != ( chr = getopt_long(argc, argv,
                                     "vthf:n:o:",
                                     long_options,
                                     &option_index)))
        {
            switch (chr)
                {
                case 'h':
                    usage(argv[0]);
                    break;

                case 'v':
                    ++do_debug;
                    break;

                case 't':
                    ++do_trace;
                    break;

                case 'f':
                    infile = optarg;
                    break;

                case 'o':
                    outfile = optarg;
                    break;

                case 'n':
                    funcname = optarg;
                    break;

                default:
                    printf("invalid option %c\n", chr);
                    exit(1);
                }
        }

    argc -= optind;
    argv += optind;

    cu_global_debug  = do_trace;
    hg_global_debug  = do_debug;
    h2o_global_debug = do_debug;

    startMercuryLibrary();

    Water water = (Water) malloc(sizeof(struct water));

    memset(water, 0, sizeof(struct water));

    water->base.more      = water_MoreData;
    water->base.node      = water_FindNode;
    water->base.attach    = water_AddName;
    water->base.predicate = water_FindPredicate;
    water->base.event     = water_FindEvent;

    assert(water->base.more      == water_MoreData);
    assert(water->base.node      == water_FindNode);
    assert(water->base.attach    == water_AddName);
    assert(water->base.predicate == water_FindPredicate);
    assert(water->base.event     == water_FindEvent);

    cu_InputInit(&water->base, 1024);

    assert(water->base.cache->size > 0);

    assert(water->base.more      == water_MoreData);
    assert(water->base.node      == water_FindNode);
    assert(water->base.attach    == water_AddName);
    assert(water->base.predicate == water_FindPredicate);
    assert(water->base.event     == water_FindEvent);


    stable_Init(1024, &water->node);

    assert(water->base.cache->size > 0);

    stable_Init(1024, &water->predicate);

    assert(water->base.cache->size > 0);

    stable_Init(1024, &water->event);

    assert(water->base.cache->size > 0);

    water_SetEvent(water, "define", define_event);
    water_SetEvent(water, "end", end_event);
    water_SetEvent(water, "identifier", identifier_event);
    water_SetEvent(water, "label", label_event);
    water_SetEvent(water, "leaf", leaf_event);
    water_SetEvent(water, "maybe", maybe_event);
    water_SetEvent(water, "min_max", min_max_event);
    water_SetEvent(water, "number", number_event);
    water_SetEvent(water, "one_plus", one_plus_event);
    water_SetEvent(water, "range", range_event);
    water_SetEvent(water, "reference", reference_event);
    water_SetEvent(water, "select", select_event);
    water_SetEvent(water, "start", start_event);
    water_SetEvent(water, "tree", tree_event);
    water_SetEvent(water, "tuble", tuble_event);
    water_SetEvent(water, "zero_plus", zero_plus_event);

    water_graph((PrsInput) water);

    if (!parser_state_ExternalInit(&water->state)) {
        VM_ERROR("unable to init parser state\n");
    }

    if (!space_AddRoot(_zero_space, (Extended)(&water->state))) {
        VM_ERROR("unable to add parser state to root set\n");
    }

    if (infile) {
         doOpen(water, infile);
    } else {
        water->buffer.filename = "<stdin>";
        water->buffer.input    = stdin;
    }

    doParse(water);

    if (infile) {
        doClose(water);
    }

    return 0;
}
