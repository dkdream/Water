
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
    unsigned curser;
    ssize_t  read;
    size_t   allocated;
    char    *line;
} TextBuffer;

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
} *ParserState;

typedef struct water {
    struct prs_input    base;

    struct static_table node;
    struct static_table predicate;
    struct static_table event;
    struct parse_state  state;

    bool done;

    TextBuffer buffer;
} *Water;

static bool parser_state_Scanner(const Extended node) {
    if (isNil(node)) return true;

    ParserState state = (ParserState) node;

    if (!node_Live(state->mark))      return false;
    if (!node_Live(state->parse))     return false;

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

    printf("read line %s", tbuffer->line);

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

static bool parse_done(PrsInput parser, PrsCursor at) {
    Water water = (Water) parser;
    water->done = true;
    return true;
}

/***********************************************************/

/***********************************************************/

static bool noop_event(PrsInput input, PrsCursor at) {
    return true;
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
    water->done = false;

    struct thread thread;

    thread.at = 0;
    stack_Create(&thread.stack);

    while (cu_Parse("rule", (PrsInput)water)) {
        if (!cu_RunQueue((PrsInput)water)) {
            printf("event error\n");
        }

        if (water->done) return;
    }

    printf("syntax error\n");
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

    printf("calling cu_InputInit\n");
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

    water_SetEvent(water, "define", noop_event);
    water_SetEvent(water, "done", parse_done);
    water_SetEvent(water, "end", noop_event);
    water_SetEvent(water, "identifier", noop_event);
    water_SetEvent(water, "label", noop_event);
    water_SetEvent(water, "leaf", noop_event);
    water_SetEvent(water, "maybe", noop_event);
    water_SetEvent(water, "min_max", noop_event);
    water_SetEvent(water, "min_max", noop_event);
    water_SetEvent(water, "number", noop_event);
    water_SetEvent(water, "one_plus", noop_event);
    water_SetEvent(water, "range", noop_event);
    water_SetEvent(water, "reference", noop_event);
    water_SetEvent(water, "select", noop_event);
    water_SetEvent(water, "start", noop_event);
    water_SetEvent(water, "tree", noop_event);
    water_SetEvent(water, "tuble", noop_event);
    water_SetEvent(water, "zero_plus", noop_event);

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
