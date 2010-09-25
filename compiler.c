/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **
 **
 **
 ***/
#include "compiler.h"

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

static inline bool cell_Create(H2oCell *target) {
    if (!target) return false;

    H2oCell result = malloc(sizeof(struct water_cell));

    if (!result) return false;

    memset(result, 0, sizeof(struct water_cell));

    *target = result;

    return true;
}

static bool stack_Init(unsigned count, H2oStack target) {
    if (!target) return false;

    memset(target, 0, sizeof(struct water_stack));

    if (count < 1) return true;

    H2oCell list = 0;
    H2oCell end  = 0;

    if (!cell_Create(&list)) return false;

    target->free_list = end = list;

    while (count--) {
        if (!cell_Create(&end->next)) return false;
    }

    return true;
}

static inline const char* type2name(H2oType type) {
    switch (type) {
    case water_define:    return "define";
    case water_identifer: return "identifer";
    case water_label:     return "label";
    case water_event:     return "event";
    case water_predicate: return "predicate";
    case water_not:       return "not";
    case water_assert:    return "assert";
    case water_zero_plus: return "zero_plus";
    case water_one_plus:  return "one_plus";
    case water_maybe:     return "maybe";
    case water_count:     return "count";
    case water_range:     return "range";
    case water_select:    return "select";
    case water_tuple:     return "tuple";
    case water_assign:    return "assign";
    case water_leaf:      return "leaf";
    case water_childern:  return "childern";
    case water_and:       return "and";
    case water_or:        return "or";
    case water_sequence:  return "sequence";
    case water_void: break;
    }
    return "unknown";
}

static inline void node_Print(FILE* output, H2oNode value) {
    if (!value.any) {
        fprintf(output, "nil");
        return;
    }

    H2oType type = value.any->type;

    switch (type) {
    case water_define: {
        fprintf(output, "define[");
        const char* text = value.define->name.start;
        unsigned  length = value.define->name.length;
        fprintf(output, "%*.*s", length, length, text);
        fprintf(output, "]");
        return;
    }
    case water_identifer: {
        const char* text = value.text->value.start;
        unsigned  length = value.text->value.length;
        fprintf(output, "%*.*s", length, length, text);
        return;
    }
    case water_label: {
        const char* text = value.text->value.start;
        unsigned  length = value.text->value.length;
        fprintf(output, "%*.*s:", length, length, text);
        return;
    }
    case water_event:  {
        const char* text = value.text->value.start;
        unsigned  length = value.text->value.length;
        fprintf(output, "@%*.*s", length, length, text);
        return;
    }
    case water_predicate:  {
        const char* text = value.text->value.start;
        unsigned  length = value.text->value.length;
        fprintf(output, "%%%*.*s", length, length, text);
        return;
    }
    case water_not:       {
        fprintf(output, "(");
        node_Print(output, value.operator->value);
        fprintf(output, ")!");
        return;
    }
    case water_assert:    {
        fprintf(output, "(");
        node_Print(output, value.operator->value);
        fprintf(output, ")&");
        return;
    }
    case water_zero_plus: {
        fprintf(output, "(");
        node_Print(output, value.operator->value);
        fprintf(output, ")*");
        return;
    }
    case water_one_plus:  {
        fprintf(output, "(");
        node_Print(output, value.operator->value);
        fprintf(output, ")+");
        return;
    }
    case water_maybe:     {
        fprintf(output, "(");
        node_Print(output, value.operator->value);
        fprintf(output, ")?");
        return;
    }
    case water_count:     {
        fprintf(output, "%u", value.count->count);
        return;
    }
    case water_range:     {
        fprintf(output, "(");
        node_Print(output, value.range->value);
        fprintf(output, "){");
        node_Print(output, value.range->min);
        fprintf(output, ",");
        node_Print(output, value.range->max);
        fprintf(output, "}");
        return;
    }
    case water_select:    {
        fprintf(output, "(");
        node_Print(output, value.branch->before);
        fprintf(output, "/");
        node_Print(output, value.branch->after);
        fprintf(output, ")");
        return;
    }
    case water_tuple:     {
        fprintf(output, "(");
        node_Print(output, value.branch->before);
        fprintf(output, ",");
        node_Print(output, value.branch->after);
        fprintf(output, ")");
        return;
    }
    case water_assign:    {
       node_Print(output, value.assign->label);
       fprintf(output, "->");
       node_Print(output, value.assign->event);
       return;
    }

    case water_childern:      {
        fprintf(output, "(");
        node_Print(output, value.operator->value);
        fprintf(output, ")childern");
        return;
    }

    case water_leaf:      {
        fprintf(output, "[]");
        return;
    }
    case water_and:  {
        fprintf(output, "(");
        node_Print(output, value.branch->before);
        fprintf(output, " ");
        node_Print(output, value.branch->after);
        fprintf(output, ")and");
        return;
    }
    case water_or:  {
        fprintf(output, "(");
        node_Print(output, value.branch->before);
        fprintf(output, " ");
        node_Print(output, value.branch->after);
        fprintf(output, ")or");
        return;
    }
    case water_sequence:  {
        fprintf(output, "(");
        node_Print(output, value.branch->before);
        fprintf(output, " ");
        node_Print(output, value.branch->after);
        fprintf(output, ")");
        return;
    }
    case water_void:
        break;
    }

    const char* name = type2name(type);
    fprintf(output, "%s(%x)", name, (unsigned) value.any);
}

static void stack_Print(FILE* output, H2oStack stack) {
    inline void print_list(H2oCell cell) {
        if (!cell) return;
        print_list(cell->next);
        fprintf(output, " ");
        node_Print(output, cell->value);
    }

    if (!stack) return;

    fprintf(output, "stack:");

    print_list(stack->top);

    return;
}

static inline bool node_Create(enum water_type type, H2oTarget target) {
    if (!target.any) return false;

    static unsigned next_id = 1;
    unsigned size = 0;

    switch (type) {
    case water_define:
        size = sizeof(struct water_define);
        break;

    case water_identifer:
        size = sizeof(struct water_text);
        break;

    case water_label:
        size = sizeof(struct water_text);
        break;

    case water_event:
        size = sizeof(struct water_text);
        break;

    case water_predicate:
        size = sizeof(struct water_text);
        break;

    case water_not:
        size = sizeof(struct water_operator);
        break;

    case water_assert:
        size = sizeof(struct water_operator);
        break;

    case water_zero_plus:
        size = sizeof(struct water_operator);
        break;

    case water_one_plus:
        size = sizeof(struct water_operator);
        break;

     case water_maybe:
        size = sizeof(struct water_operator);
        break;

    case water_count:
        size = sizeof(struct water_count);
        break;

    case water_range:
        size = sizeof(struct water_operator);
        break;

    case water_select:
        size = sizeof(struct water_branch);
        break;

    case water_tuple:
        size = sizeof(struct water_branch);
        break;

    case water_assign:
        size = sizeof(struct water_assign);
        break;

    case water_leaf:
        size = sizeof(struct water_any);
        break;

    case water_childern:
        size = sizeof(struct water_operator);
        break;

    case water_and:
        size = sizeof(struct water_branch);
        break;

    case water_or:
        size = sizeof(struct water_branch);
        break;

    case water_sequence:
        size = sizeof(struct water_branch);
        break;

    case water_void:
        return false;
    }

    H2oAny result = malloc(size);

    if (!result) return false;

    memset(result, 0, size);

    result->type = type;
    result->id   = next_id++;

    *target.any = result;

    return true;
}

static bool water_MoreData(PrsInput parser)
{
    H2oParser water = (H2oParser) parser;

    H2oBuffer tbuffer = &water->buffer;

    if (tbuffer->cursor >= tbuffer->read) {
        int read = getline(&tbuffer->line, &tbuffer->allocated, tbuffer->input);
        if (read < 0) return false;
        tbuffer->cursor = 0;
        tbuffer->read   = read;
    }

    if (0 < h2o_global_debug) {
        unsigned line_number = parser->cursor.line_number + 1;
        printf("%s[%d] %s", tbuffer->filename, line_number, tbuffer->line);
    }

    unsigned  count = tbuffer->read - tbuffer->cursor;
    const char *src = tbuffer->line + tbuffer->cursor;

    if (!cu_AppendData(parser, count, src)) return false;

    tbuffer->cursor += count;

    return true;
}


static bool water_FindNode(PrsInput parser, PrsName name, PrsNode* target) {
    H2oParser water = (H2oParser) parser;

    StaticValue result;

    if (!stable_Find(&water->node, name, &result)) return false;

    *target = (PrsNode) result;

    return true;
}

static bool water_FindPredicate(PrsInput parser, PrsName name, PrsPredicate* target) {
    return false;
}

static bool water_FindEvent(PrsInput parser, PrsName name, PrsEvent* target) {
    H2oParser water = (H2oParser) parser;

    StaticValue result;

    if (!stable_Find(&water->event, name, &result)) return false;

    *target = (PrsEvent) result;

    return true;
}

static bool water_AddName(PrsInput parser, PrsName name, PrsNode value) {
    H2oParser water = (H2oParser) parser;

    return stable_Replace(&water->node, name, (StaticValue) value);
}

static bool water_SetEvent(H2oParser water, PrsName name, PrsEvent value) {
    return stable_Replace(&water->event, name, (StaticValue) value);
}

static void print_State(H2oParser water, bool start, const char* format, ...) {
    if (3 > h2o_global_debug) return;

    H2oStack parse = &water->stack;

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
            printf("--------\n");
        }
    } else {
        printf(" parse: "); stack_Print(stdout, parse); printf("\n");
        printf("----------------------------------------------\n");
    }
}

static inline bool stack_Push(H2oStack stack, H2oNode value) {
    if (!stack)     return false;
    if (!value.any) return false;

    H2oCell cell = stack->free_list;

    if (cell) {
        stack->free_list = cell->next;
    } else {
        if (!cell_Create(&cell)) return false;
    }

    cell->next  = stack->top;
    cell->value = value;

    stack->top = cell;

    return true;
}

static inline bool stack_Pop(H2oStack stack, H2oTarget value) {
    if (!stack)      return false;
    if (!value.node) return false;
    if (!stack->top) return false;

    H2oCell cell = stack->top;

    stack->top = cell->next;

    cell->next = stack->free_list;
    stack->free_list = cell;

    *value.node = cell->value;

    return true;
}

static inline bool stack_Swap(H2oStack stack) {
    if (!stack)      return false;
    if (!stack->top) return false;

    H2oCell stk1 = stack->top;

    if (!stk1->next) return false;

    H2oCell stk2 = stk1->next;
    H2oCell stk3 = stk2->next;

    stk1->next = stk3;
    stk2->next = stk1;
    stack->top = stk2;

    return true;
}

/***********************************************************/

static inline bool make_Action(H2oParser water, H2oType type) {
    if (!water) return false;

    H2oText result = 0;

    if (!node_Create(type, &result)) return false;

    if (!stack_Push(&water->stack, result)) return false;

    return true;
}

static inline bool make_Text(H2oParser water, H2oType type) {
    if (!water) return false;

    H2oText result = 0;

    if (!node_Create(type, &result)) return false;

    if (!cu_MarkedText((PrsInput) water, &result->value)) return false;

    if (!stack_Push(&water->stack, result)) return false;

    return true;
}

static inline bool make_Operator(H2oParser water, H2oType type) {
    if (!water) return false;

    H2oOperator result = 0;

    if (!node_Create(type, &result)) return false;

    if (!stack_Pop(&water->stack, &result->value)) return false;

    if (!stack_Push(&water->stack, result)) return false;

    return true;
}

static inline bool make_Branch(H2oParser water, H2oType type) {
    if (!water) return false;

    H2oBranch result = 0;

    if (!node_Create(type, &result)) return false;

    if (!stack_Pop(&water->stack, &result->after)) return false;
    if (!stack_Pop(&water->stack, &result->before)) return false;

    if (!stack_Push(&water->stack, result)) return false;

    return true;
}

/***********************************************************/

static bool declare_event(PrsInput input, PrsCursor location) {
    const char *event_name = "declare";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    H2oDefine define = 0;

    if (!node_Create(water_define, &define)) return false;

    if (!cu_MarkedText((PrsInput) water, &define->name)) return false;

    define->next = water->rule;

    water->rule = define;

    if (!stack_Push(&water->stack, define)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool define_event(PrsInput input, PrsCursor location) {
    const char *event_name = "define";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    H2oDefine define = water->rule;

    if (!stack_Pop(&water->stack, &define->match)) return false;

    H2oDefine check;

    if (!stack_Pop(&water->stack, &check)) return false;

    if (check != define) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool identifier_event(PrsInput input, PrsCursor location) {
    const char *event_name = "identifier";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Text(water, water_identifer)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool label_event(PrsInput input, PrsCursor location) {
    const char *event_name = "label";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Text(water, water_label)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool event_event(PrsInput input, PrsCursor location) {
    const char *event_name = "event";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Text(water, water_event)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool predicate_event(PrsInput input, PrsCursor location) {
    const char *event_name = "predicate";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Text(water, water_predicate)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool not_event(PrsInput input, PrsCursor location) {
    const char *event_name = "not";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Operator(water, water_not)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool assert_event(PrsInput input, PrsCursor location) {
    const char *event_name = "assert";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Operator(water, water_assert)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool zero_plus_event(PrsInput input, PrsCursor location) {
    const char *event_name = "zero_plus";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Operator(water, water_zero_plus)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool one_plus_event(PrsInput input, PrsCursor location) {
    const char *event_name = "one_plus";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Operator(water, water_one_plus)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool maybe_event(PrsInput input, PrsCursor location) {
    const char *event_name = "maybe";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Operator(water, water_maybe)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool number_event(PrsInput input, PrsCursor location) {
    const char *event_name = "number";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    PrsData text;
    if (!cu_MarkedText(input, &text)) return false;

    unsigned long count = strtoul(text.start, 0, 10);

    if (UINT_MAX <= count) return false;

    H2oCount result;

    if (!node_Create(water_count, &result)) return false;

    result->count = count;

    if (!stack_Push(&water->stack, result)) return false;

    return true;
    (void) event_name;
}

static bool range_event(PrsInput input, PrsCursor location) {
    const char *event_name = "range";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    H2oRange result;

    if (!node_Create(water_range, &result)) return false;

    if (!stack_Pop(&water->stack, &result->max)) return false;
    if (!stack_Pop(&water->stack, &result->min)) return false;
    if (!stack_Pop(&water->stack, &result->value)) return false;

    if (!stack_Push(&water->stack, result)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool select_event(PrsInput input, PrsCursor location) {
    const char *event_name = "select";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Branch(water, water_select)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool tuple_event(PrsInput input, PrsCursor location) {
    const char *event_name = "tuple";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Branch(water, water_tuple)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool assign_event(PrsInput input, PrsCursor location) {
    const char *event_name = "tuple";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Branch(water, water_and)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool leaf_event(PrsInput input, PrsCursor location) {
    const char *event_name = "leaf";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Action(water, water_leaf)) return false;
    if (!make_Branch(water, water_and))  return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool tree_event(PrsInput input, PrsCursor location) {
    const char *event_name = "tree";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Operator(water, water_childern)) return false;
    if (!make_Branch(water, water_and))        return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool and_event(PrsInput input, PrsCursor location) {
    const char *event_name = "and";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Branch(water, water_and)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool or_event(PrsInput input, PrsCursor location) {
    const char *event_name = "or";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Branch(water, water_or)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

static bool sequence_event(PrsInput input, PrsCursor location) {
    const char *event_name = "sequence";
    H2oParser water = (H2oParser) input;
    print_State(water, true, "%s", event_name);

    if (!make_Branch(water, water_sequence)) return false;

    print_State(water, false, "%s", event_name);
    return true;
    (void) event_name;
}

/*------------------------------------------------------------*/

static bool write_Tree(H2oParser water, H2oNode match) {

    inline bool write_node(H2oNode value) {
        return write_Tree(water, value);
    }

    inline void lvalue(H2oNode value) {
        fprintf(water->output, "L%.6x", value.any->id);
    }

    inline void avalue(H2oNode value) {
        fprintf(water->output, "&L%.6x", value.any->id);
    }

    inline bool write_define() {
        H2oDefine   rule = match.define;
        const char* text = rule->name.start;
        unsigned  length = rule->name.length;
        fprintf(water->output, "\n// rule %*.*s\n", length, length, text);
        write_node(rule->match);
        fprintf(water->output, "static const H2oCode ");
        lvalue(rule);
        fprintf(water->output, " = ");
        avalue(rule->match);
        fprintf(water->output, "; // rule %*.*s\n", length, length, text);
        return true;
    }

    inline bool write_identifer() {
        H2oText text = match.text;
        fprintf(water->output, "static struct water_action ");
        lvalue(text);
        fprintf(water->output, " = { water_Apply, 0, \"%*.*s\" };\n",
                text->value.length,
                text->value.length,
                text->value.start);
        return true;
    }

    inline bool write_label() {
        H2oText text = match.text;
        fprintf(water->output, "static struct water_action ");
        lvalue(text);
        fprintf(water->output, " = { water_Root, 0, \"%*.*s\" };\n",
                text->value.length,
                text->value.length,
                text->value.start);
        return true;
    }

    inline bool write_event() {
        H2oText text = match.text;
        fprintf(water->output, "static struct water_action ");
        lvalue(text);
        fprintf(water->output, " = { water_Event, 0, \"%*.*s\" };\n",
                text->value.length,
                text->value.length,
                text->value.start);
        return true;
    }

    inline bool write_predicate() {
        H2oText text = match.text;
        fprintf(water->output, "static struct water_action ");

        lvalue(text);
        fprintf(water->output, " = { water_Predicate, 0, \"%*.*s\" };\n",
                text->value.length,
                text->value.length,
                text->value.start);
        return true;
    }

    inline bool write_not() {
        H2oOperator operator = match.operator;
        write_node(operator->value);
        fprintf(water->output, "static const struct water_function ");
        lvalue(match);
        fprintf(water->output, " = { water_Not, ");
        avalue(operator->value);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_assert() {
        H2oOperator operator = match.operator;
        write_node(operator->value);
        fprintf(water->output, "static const struct water_function ");
        lvalue(match);
        fprintf(water->output, " = { water_Assert, ");
        avalue(operator->value);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_zero_plus() {
        H2oOperator operator = match.operator;
        write_node(operator->value);
        fprintf(water->output, "static const struct water_function ");
        lvalue(match);
        fprintf(water->output, " = { water_ZeroPlus, ");
        avalue(operator->value);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_one_plus() {
        H2oOperator operator = match.operator;
        write_node(operator->value);
        fprintf(water->output, "static const struct water_function ");
        lvalue(match);
        fprintf(water->output, " = { water_OnePlus, ");
        avalue(operator->value);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_maybe() {
        H2oOperator operator = match.operator;
        write_node(operator->value);
        fprintf(water->output, "static const struct water_function ");
        lvalue(match);
        fprintf(water->output, " = { water_Maybe, ");
        avalue(operator->value);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_count() {
        H2oCount count = match.count;
        fprintf(water->output, "static unsigned ");
        lvalue(match);
        fprintf(water->output, " = %u", count->count);
        return true;
    }

    inline bool write_range() {
        H2oRange range = match.range;
        write_node(range->value);
        write_node(range->min);
        write_node(range->max);
        fprintf(water->output, "static const struct water_group ");
        lvalue(match);
        fprintf(water->output, " = { water_Range, ");
        avalue(range->value);
        fprintf(water->output, ", ");
        lvalue(range->min);
        fprintf(water->output, ", ");
        lvalue(range->max);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_select() {
        H2oBranch branch = match.branch;
        write_node(branch->before);
        write_node(branch->after);
        fprintf(water->output, "static const struct water_chain ");
        lvalue(match);
        fprintf(water->output, " = { water_Select, ");
        avalue(branch->before);
        fprintf(water->output, ", ");
        avalue(branch->after);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_tuple() {
        H2oBranch branch = match.branch;
        write_node(branch->before);
        write_node(branch->after);
        fprintf(water->output, "static const struct water_chain ");
        lvalue(match);
        fprintf(water->output, " = { water_Tuple, ");
        avalue(branch->before);
        fprintf(water->output, ", ");
        avalue(branch->after);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_assign() {
        H2oAssign assign = match.assign;
        write_node(assign->label);
        write_node(assign->event);
        fprintf(water->output, "static const struct water_chain ");
        lvalue(match);
        fprintf(water->output, " = { water_Event, ");
        avalue(assign->label);
        fprintf(water->output, ", ");
        avalue(assign->event);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_leaf() {
        fprintf(water->output, "static const struct water_code ");
        lvalue(match);
        fprintf(water->output, " = { water_Leaf };\n");
        return true;
    }

    inline bool write_childern() {
        H2oOperator operator = match.operator;
        write_node(operator->value);
        fprintf(water->output, "static const struct water_function ");
        lvalue(match);
        fprintf(water->output, " = { water_Childern, ");
        avalue(operator->value);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_and() {
        H2oBranch branch = match.branch;
        write_node(branch->before);
        write_node(branch->after);
        fprintf(water->output, "static const struct water_chain ");
        lvalue(match);
        fprintf(water->output, " = { water_And, ");
        avalue(branch->before);
        fprintf(water->output, ", ");
        avalue(branch->after);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_or() {
        H2oBranch branch = match.branch;
        write_node(branch->before);
        write_node(branch->after);
        fprintf(water->output, "static const struct water_chain ");
        lvalue(match);
        fprintf(water->output, " = { water_Or, ");
        avalue(branch->before);
        fprintf(water->output, ", ");
        avalue(branch->after);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool write_sequence() {
        H2oBranch branch = match.branch;
        write_node(branch->before);
        write_node(branch->after);
        fprintf(water->output, "static const struct water_chain ");
        lvalue(match);
        fprintf(water->output, " = { water_Sequence, ");
        avalue(branch->before);
        fprintf(water->output, ", ");
        avalue(branch->after);
        fprintf(water->output, " };\n");
        return true;
    }

    inline bool error() {
        return false;
    }

    if (!match.any) return false;

    H2oType type = match.any->type;

    switch (type) {
    case water_define:    return write_define();
    case water_identifer: return write_identifer();
    case water_label:     return write_label();
    case water_event:     return write_event();
    case water_predicate: return write_predicate();
    case water_not:       return write_not();
    case water_assert:    return write_assert();
    case water_zero_plus: return write_zero_plus();
    case water_one_plus:  return write_one_plus();
    case water_maybe:     return write_maybe();
    case water_count:     return write_count();
    case water_range:     return write_range();
    case water_select:    return write_select();
    case water_tuple:     return write_tuple();
    case water_assign:    return write_assign();
    case water_leaf:      return write_leaf();
    case water_childern:  return write_childern();
    case water_and:       return write_and();
    case water_or:        return write_or();
    case water_sequence:  return write_sequence();
    case water_void:      return error();
    }

    return false;
}

static bool write_Ccode(H2oParser water, H2oDefine rule) {
    return write_Tree(water, rule);
}

/*------------------------------------------------------------*/

/* from water.c */
extern bool water_graph(PrsInput input);

static bool water_Init(H2oParser water) {
    memset(water, 0, sizeof(struct water_parser));

    water->base.more      = water_MoreData;
    water->base.node      = water_FindNode;
    water->base.attach    = water_AddName;
    water->base.predicate = water_FindPredicate;
    water->base.event     = water_FindEvent;

    if (!cu_InputInit(&water->base, 1024)) return false;

    if (!stable_Init(1024, &water->node))      return false;
    if (!stable_Init(1024, &water->event))     return false;
    if (!stack_Init(100,   &water->stack))     return false;

    water_SetEvent(water, "declare",    declare_event);
    water_SetEvent(water, "define",     define_event);
    water_SetEvent(water, "tree",       tree_event);
    water_SetEvent(water, "leaf",       leaf_event);
    water_SetEvent(water, "tuple",      tuple_event);
    water_SetEvent(water, "select",     select_event);
    water_SetEvent(water, "assert",     assert_event);
    water_SetEvent(water, "not",        not_event);
    water_SetEvent(water, "assign",     assign_event);
    water_SetEvent(water, "zero_plus",  zero_plus_event);
    water_SetEvent(water, "one_plus",   one_plus_event);
    water_SetEvent(water, "maybe",      maybe_event);
    water_SetEvent(water, "range",      range_event);
    water_SetEvent(water, "identifier", identifier_event);
    water_SetEvent(water, "label",      label_event);
    water_SetEvent(water, "event",      event_event);
    water_SetEvent(water, "predicate",  predicate_event);
    water_SetEvent(water, "number",     number_event);
    water_SetEvent(water, "and",        and_event);
    water_SetEvent(water, "or",         or_event);
    water_SetEvent(water, "sequence",   sequence_event);

    water_graph((PrsInput) water);

    return true;
}

extern bool water_Create(const char* name, H2oParser *target) {
     H2oParser water = (H2oParser) malloc(sizeof(struct water_parser));

     if (!water) return false;

     if (!water_Init(water)) return false;

     *target = water;

     if (!name) {
         water->buffer.filename = "<stdin>";
         water->buffer.input    = stdin;
         return true;
     }

     if (!(water->buffer.input = fopen(name, "r"))) {
         perror(name);
         exit(1);
     }

     water->buffer.filename = name;

     return true;
}

extern void water_Parse(H2oParser water)
{
    if (!water) return;

    if (0 < h2o_global_debug) {
        printf("before parse\n");
    }

    if (!cu_Parse("file", (PrsInput)water)) {
        cu_SyntaxError(stderr,
                       (PrsInput) water,
                       water->buffer.filename);
        return;
    }

    if (0 < h2o_global_debug) {
        printf("before run\n");
    }

    if (!cu_RunQueue((PrsInput)water)) {
        printf("event error\n");
    }

    water->output = stdout;

    H2oDefine rule = water->rule;

    while (rule) {
        write_Ccode(water, rule);
        rule = rule->next;
    }
 }

extern bool water_Free(H2oParser water) {
    if (!water) return false;
    if (!water->buffer.filename) return false;

    if (stdin != water->buffer.input) {
        fclose(water->buffer.input);
    }

    water->buffer.filename = 0;
    water->buffer.input    = 0;

    return true;
}
