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
#if 0
#include "water.h"

/* */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <stdarg.h>
#include <assert.h>

/* */

static bool make_Thread(const char* rule,
                        H2oCursor at,
                        H2oLabel  label,
                        H2oThread *target)
{
    struct water_thread *result = malloc(sizeof(struct water_thread));

    result->next  = 0;
    result->rule  = rule;
    result->at    = at;
    result->label = label;

    *target = result;

    return true;
}

static bool make_Queue(H2oQueue *target) {
    struct water_queue *result = malloc(sizeof(struct water_queue));

    result->free_list = 0;
    result->begin     = 0;
    result->end       = 0;

    *target = result;

    return true;
}

static bool queue_Event(H2oQueue    queue,
                        const char* rule,
                        H2oCursor   at,
                        H2oLabel    label)
{
    if (!queue) return true;

    struct water_thread *result = queue->free_list;

    if (!result) {
        make_Thread(rule, at, label, &result);
    } else {
        queue->free_list = result->next;
        result->next     = 0;
        result->rule     = rule;
        result->at       = at;
        result->label    = label;
    }

    if (!queue->begin) {
        queue->begin = queue->end = result;
        return true;
    }

    queue->end->next = result;
    queue->end = result;

    return true;
}

static bool queue_Run(H2oQueue queue,
                      H2oInput input)
{
    if (!queue)        return true;
    if (!queue->begin) return true;
    if (!input)        return false;

    H2oThread current = queue->begin;

    queue->begin = 0;

    unsigned index = 0;

    for ( ; current ; ) {
        H2oLabel label = current->label;

        if (!label.function(input, 0)) {
            queue->begin = current;
            return false;
        }

        index += 1;

        H2oThread next   = current->next;
        current->next    = queue->free_list;
        queue->free_list = current;
        current = next;
    }

    return true;
}

static bool queue_FreeList(H2oQueue queue,
                           H2oThread list)
{
    if (!list) return true;

    if (!queue) {
        for ( ; list ; ) {
            H2oThread next = list->next;
            free(list);
            list = next;
        }
        return true;
    }

    for ( ; list ; ) {
        H2oThread next = list->next;
        list->next = queue->free_list;
        queue->free_list = list;
        list = next;
    }

    return true;
}

static bool queue_TrimTo(H2oQueue  queue,
                         H2oThread to)
{
    if (!queue) return false;
    if (!to) {
        to = queue->begin;
        queue->begin = 0;
        queue->end   = 0;
        return  queue_FreeList(queue, to);
    }

    if (!queue->begin) return false;

    if (to == queue->end) return true;

    H2oThread current = queue->begin;

    for ( ; current ; current = current->next ) {
        if (current == to) break;

    }

    if (!current) return false;

    current = to->next;

    to->next   = 0;
    queue->end = to;

    return queue_FreeList(queue, current);

}

static bool queue_Clear(H2oQueue queue) {
    if (!queue) return true;

    H2oThread node = queue->begin;

    queue->begin = 0;
    queue->end   = 0;

    return queue_FreeList(queue, node);
}

static bool mark_begin(H2oInput input, H2oCursor at) {
    if (!input) return false;
    input->context.begin = at;
    return true;
}

static struct water_label begin_label = { &mark_begin, "set.begin" };

static bool mark_end(H2oInput input, H2oCursor at) {
    if (!input) return false;
    input->context.end = at;
    return true;
}

static struct water_label end_label = { &mark_end, "set.end" };

/* this is use only in copper_vm for debugging (SINGLE THEADED ALERT) */
static char *char2string(unsigned char value)
{
    static unsigned index = 0;
    static char buffer[60];
    char *text = buffer + (index * 6);

    // allow up to 10 call before reusing the same buffer section
    index = (index + 1) % 10;

    switch (value) {
    case '\a': return "\\a"; /* bel */
    case '\b': return "\\b"; /* bs */
    case '\e': return "\\e"; /* esc */
    case '\f': return "\\f"; /* ff */
    case '\n': return "\\n"; /* nl */
    case '\r': return "\\r"; /* cr */
    case '\t': return "\\t"; /* ht */
    case '\v': return "\\v"; /* vt */
    case '\'': return "\\'"; /* ' */
    case '\"': return "\\\"";
    case '\\': return "\\\\";
    default:
        if (value < 32) {
            int len = sprintf(text, "\\%03o", value);
            text[len] = 0;
            return text;
        }
        if (126 < value) {
            int len = sprintf(text, "\\%03o", value);
            text[len] = 0;
            return text;
        }
        break;
    }

    text[0] = (char) value;
    text[1] = 0;
    return text;
}

static bool make_Point(H2oPoint *target)
{
    unsigned fullsize = sizeof(struct water_point);

    struct water_point *result = malloc(fullsize);
    memset(result, 0, fullsize);

    *target = result;

    return true;
}

static bool cache_MorePoints(H2oCache value, unsigned count) {
    if (!value) return false;

    H2oPoint list = value->free_list;

    for ( ; count ; --count) {
        H2oPoint head;

        if (!make_Point(&head)) return false;

        head->next = list;
        value->free_list = head;
        list = head;
    }

    return true;
}


static bool make_Cache(unsigned size, H2oCache *target) {
    unsigned fullsize = (sizeof(struct water_cache) + (size * sizeof(H2oPoint)));

    struct water_cache *result = malloc(fullsize);
    memset(result, 0, fullsize);

    result->size = size;

    if (!cache_MorePoints(result, size)) return false;

    *target = result;

    return true;
}

static bool cache_Point(H2oCache cache, H2oNode node, unsigned offset) {
    if (!cache) return false;

    unsigned code  = offset;
    unsigned index = code  % cache->size;
    H2oPoint list  = cache->table[index];

    for ( ; list ; list = list->next) {
        if (node   != list->node)   continue;
        if (offset != list->offset) continue;
        return true;
    }

    if (!cache->free_list) {
        if (!cache_MorePoints(cache, cache->size)) return false;
    }

    H2oPoint head = cache->free_list;

    head->node   = node;
    head->offset = offset;

    cache->free_list = head->next;

    head->next = cache->table[index];

    cache->table[index] = head;

    return true;
}

static bool cache_Find(H2oCache cache, H2oNode node, unsigned offset) {
    if (!cache) return false;

    unsigned code  = offset;
    unsigned index = code  % cache->size;
    H2oPoint list  = cache->table[index];

    for ( ; list ; list = list->next) {
        if (node   != list->node)   continue;
        if (offset != list->offset) continue;
        return true;
    }

    return false;
}

static bool cache_Remove(H2oCache cache, H2oNode node, unsigned offset) {
    if (!cache) return false;

    unsigned code  = offset;
    unsigned index = code  % cache->size;
    H2oPoint list  = cache->table[index];

    if (node   == list->node) {
        if (offset == list->offset) {
            cache->table[index] = list->next;
            list->next = cache->free_list;
            cache->free_list = list;
            return true;
        }
    }

    H2oPoint prev = list;

    for ( ; list->next ; ) {
        prev = list;
        list = list->next;
        if (node   != list->node)   continue;
        if (offset != list->offset) continue;

        prev->next = list->next;
        list->next = cache->free_list;
        cache->free_list = list;

        return true;
    }

    return true;
}

static bool cache_Clear(H2oCache cache) {
    if (!cache) return false;

    unsigned index   = 0;
    unsigned columns = cache->size;

    for ( ; index < columns ; ++index ) {
        H2oPoint list = cache->table[index];
        cache->table[index] = 0;
        for ( ; list ; ) {
            H2oPoint next = list->next;
            list->next = cache->free_list;
            cache->free_list = list;
            list = next;
        }
    }

    return true;
}


static bool water_vm(const char* rulename,
                     H2oNode start,
                     unsigned level,
                     H2oInput input)
{
    assert(0 != input);
    assert(0 != start);

    H2oQueue queue = input->queue;
    assert(0 != queue);

    H2oThread mark = 0;
    H2oCursor at;

    inline const char* node_label(H2oNode node) {
        if (!node)        return "--unknown--";
        if (!node->label) return "--unknown--";
        return node->label;
    }

    inline void hold() {
        mark = queue->end;
        at   = input->cursor;
        if (at.text_inx > input->reach.text_inx) {
            input->reach = at;
        }
    }

    inline bool reset()   {
        if (!mark) {
            if (!queue_Clear(queue)) return false;
        } else {
            if (!queue_TrimTo(queue, mark)) return false;
        }
        input->cursor = at;
        return true;
    }

    inline bool more() {
        return input->more(input);
    }

    inline bool next() {
        unsigned point = input->cursor.text_inx;

        if ('\n' == input->data.buffer[point]) {
            input->cursor.line_number += 1;
            input->cursor.char_offset  = 0;
        } else {
            input->cursor.char_offset += 1;
        }

        input->cursor.text_inx += 1;

        return true;
    }

    inline bool current(H2oChar *target) {
        unsigned point = input->cursor.text_inx;
        unsigned limit = input->data.limit;

        if (point >= limit) {
            if (!more()) return false;
        }

        *target = input->data.buffer[point];

        return true;
    }

    inline bool node(H2oName name, H2oNode* target) {
        if (!input->node(input, name, target)) {
            CU_ERROR("node %s not found\n", name);
            return false;
        }
        return true;
    }

    inline bool predicate(H2oName name) {
        H2oPredicate test = 0;
        if (!input->predicate(input, name, &test)) {
            CU_ERROR("predicate %s not found\n", name);
            return false;
        }
        return test(input);
    }

    inline bool set_cache(H2oNode cnode) {
        return cache_Point(input->cache, cnode, at.text_inx);
    }

    inline bool check_cache(H2oNode cnode) {
        if (cache_Find(input->cache, cnode, at.text_inx)) return true;
        if (water_MatchName != cnode->oper) return false;
        const char *name = cnode->arg.name;
        H2oNode value;
        if (!node(name, &value)) return false;
        if (!check_cache(value)) return false;
        set_cache(value);
        return true;
    }

    inline void indent(unsigned debug) {
        CU_ON_DEBUG(debug,
                    { unsigned inx = level;
                        for ( ; inx ; --inx) {
                            CU_DEBUG(debug, " |");
                        }
                    });
    }

    inline void debug_charclass(unsigned debug, const unsigned char *bits) {
        CU_ON_DEBUG(debug,
                    { unsigned inx = 0;
                        for ( ; inx < 32;  ++inx) {
                            CU_DEBUG(debug, "\\%03o", bits[inx]);
                        }
                    });
    }

    inline bool add_event(H2oLabel label) {
        indent(2); CU_DEBUG(2, "event %s(%x) at (%u,%u)\n",
                           label.name,
                           (unsigned) label.function,
                           at.line_number + 1,
                           at.char_offset);

        return queue_Event(queue,
                           rulename,
                           at,
                           label);
    }

    inline bool checkFirstSet(H2oNode cnode, bool *target) {
        if (!target) return false;

        if (prs_MatchName == cnode->oper) {
            const char *name = cnode->arg.name;
            H2oNode value;
            if (!node(name, &value)) return false;
            if (!checkFirstSet(value, target)) return false;
            if (!*target) set_cache(value);
            return true;
        }

        if (pft_opaque == cnode->type) return false;
        if (pft_event  == cnode->type) return false;

        H2oFirstSet  first = cnode->first;
        H2oFirstList list  = cnode->start;

        if (!first) return false;

        if (list) {
            if (0 < list->count) {
                indent(2); CU_DEBUG(2, "check (%s) at (%u,%u) first (bypass call nodes)\n",
                                    node_label(cnode),
                                    at.line_number + 1,
                                    at.char_offset);
                return false;
            }
        }

        H2oChar chr = 0;
        if (!current(&chr)) return false;

        unsigned             binx   = chr;
        const unsigned char *bits   = first->bitfield;

        if (bits[binx >> 3] & (1 << (binx & 7))) {
            indent(4); CU_DEBUG(4, "check (%s) to cursor(\'%s\') at (%u,%u) first %s",
                                node_label(cnode),
                                char2string(chr),
                                at.line_number + 1,
                                at.char_offset,
                                "continue");
            CU_DEBUG(4, " (");
            debug_charclass(4, bits);
            CU_DEBUG(4, ")\n");
            return false;
        }

        bool result;

        if (pft_transparent == cnode->type) {
            *target = result = true;
        } else {
            *target = result = false;
            cache_Point(input->cache, cnode, at.text_inx);
        }

        indent(2); CU_DEBUG(1, "check (%s) to cursor(\'%s\') at (%u,%u) first %s result %s",
                            node_label(cnode),
                            char2string(chr),
                            at.line_number + 1,
                            at.char_offset,
                            "skip",
                            (result ? "passed" : "failed"));
        CU_DEBUG(4, " (");
        debug_charclass(4, bits);
        CU_DEBUG(4, ")");
        CU_DEBUG(1, "\n");
        return true;
    }

    inline bool checkMetadata(H2oNode cnode, bool *target) {
        if (!target) return false;

        H2oMetaFirst meta  = cnode->metadata;

        if (!meta) return false;

        if (pft_opaque == meta->type) {
            indent(4); CU_DEBUG(4, "check (%s) at (%u,%u) meta (bypass opaque)\n",
                                node_label(cnode),
                                at.line_number + 1,
                                at.char_offset);
            return false;
        }

        if (pft_event == meta->type) {
            indent(4); CU_DEBUG(4, "check (%s) at (%u,%u) meta (bypass event)\n",
                                node_label(cnode),
                                at.line_number + 1,
                                at.char_offset);
            return false;
        }

        if (prs_MatchName == cnode->oper) {
            const char *name = cnode->arg.name;
            H2oNode value;
            if (!node(name, &value)) return false;
            if (!checkMetadata(value, target)) return false;
            if (!*target) set_cache(value);
            return true;
        }

        if (!meta->done) {
            indent(2); CU_DEBUG(1, "check (%s) using unfinish meta data\n",
                                node_label(cnode));
        }

        H2oMetaSet first = meta->first;

        if (!first) {
            indent(4); CU_DEBUG(4, "check (%s) at (%u,%u) meta (bypass no first)\n",
                                node_label(cnode),
                                at.line_number + 1,
                                at.char_offset);
            return false;
        }

        H2oChar chr = 0;
        if (!current(&chr)) return false;

        unsigned             binx = chr;
        const unsigned char *bits = first->bitfield;

        if (bits[binx >> 3] & (1 << (binx & 7))) {
            indent(4); CU_DEBUG(4, "check (%s) to cursor(\'%s\') at (%u,%u) meta %s (",
                                node_label(cnode),
                                char2string(chr),
                                at.line_number + 1,
                                at.char_offset,
                                "continue");
            debug_charclass(4, bits);
            CU_DEBUG(4, ")\n");
            return false;
        }


        bool result;

        if (pft_transparent == meta->type) {
            *target = result = true;
        } else {
            *target = result = false;
            cache_Point(input->cache, cnode, at.text_inx);
        }

        indent(2); CU_DEBUG(1, "check (%s) to cursor(\'%s\') at (%u,%u) %s meta %s result %s",
                            node_label(cnode),
                            char2string(chr),
                            at.line_number + 1,
                            at.char_offset,
                            oper2name(cnode->oper),
                            "skip",
                            (result ? "passed" : "failed"));
        CU_DEBUG(4, " (");
        debug_charclass(4, bits);
        CU_DEBUG(4, ")");
        CU_DEBUG(1, "\n");

        return true;
    }

    inline bool prs_and() {
        if (!copper_vm(rulename, start->arg.pair->left, level, input))  {
            reset();
            return false;
        }
        if (!copper_vm(rulename, start->arg.pair->right, level, input)) {
            reset();
            return false;
        }

        return true;
    }

    inline bool prs_choice() {
        if (copper_vm(rulename, start->arg.pair->left, level, input)) {
            return true;
        }
        reset();
        if (copper_vm(rulename, start->arg.pair->right, level, input)) {
            return true;
        }
        reset();
        return false;
    }

    inline bool prs_zero_plus() {
        while (copper_vm(rulename, start->arg.node, level, input)) {
            hold();
        }
        reset();
        return true;
    }

    inline bool prs_one_plus() {
        bool result = false;
        while (copper_vm(rulename, start->arg.node, level, input)) {
            result = true;
            hold();
        }
        reset();
        return result;
    }

    inline bool prs_optional() {
        if (copper_vm(rulename, start->arg.node, level, input)) {
            return true;
        }
        reset();
        return true;
    }

    inline bool prs_assert_true() {
        if (copper_vm(rulename, start->arg.node, level, input)) {
            reset();
            return true;
        }
        reset();
        return false;
    }

    inline bool prs_assert_false() {
        if (copper_vm(rulename, start->arg.node, level, input)) {
            reset();
            return false;
        }
        reset();
        return true;
    }

    inline bool prs_char() {
        H2oChar chr = 0;
        if (!current(&chr)) {
            indent(2); CU_DEBUG(2, "match(\'%s\') to end-of-file at (%u,%u)\n",
                               char2string(start->arg.letter),
                               at.line_number + 1,
                               at.char_offset);
            return false;
        }

        indent(2); CU_DEBUG(2, "match(\'%s\') to cursor(\'%s\') at (%u,%u)\n",
                           char2string(start->arg.letter),
                           char2string(chr),
                           at.line_number + 1,
                           at.char_offset);

        if (chr != start->arg.letter) {
            return false;
        }
        next();
        return true;
    }

    inline bool prs_between() {
        H2oChar chr = 0;
        if (!current(&chr)) {
            indent(2); CU_DEBUG(2, "between(\'%s\',\'%s\') to end-of-file at (%u,%u)\n",
                               char2string(start->arg.range->begin),
                               char2string(start->arg.range->end),
                               at.line_number + 1,
                               at.char_offset);
            return false;
        }

        indent(2); CU_DEBUG(2, "between(\'%s\',\'%s\') to cursor(\'%s\') at (%u,%u)\n",
                           char2string(start->arg.range->begin),
                           char2string(start->arg.range->end),
                           char2string(chr),
                           at.line_number + 1,
                           at.char_offset);

        if (chr < start->arg.range->begin) {
            return false;
        }
        if (chr > start->arg.range->end) {
            return false;
        }
        next();
        return true;
    }

    inline bool prs_in() {
        H2oChar chr = 0;

        if (!current(&chr)) {
            indent(2); CU_DEBUG(2, "set(%s) to end-of-file at (%u,%u)\n",
                               start->arg.set->label,
                               at.line_number + 1,
                               at.char_offset);
            return false;
        }

        indent(2); CU_DEBUG(2, "set(%s) to cursor(\'%s\') at (%u,%u)\n",
                           start->arg.set->label,
                           char2string(chr),
                           at.line_number + 1,
                           at.char_offset);

        unsigned             binx = chr;
        const unsigned char *bits = start->arg.set->bitfield;

        if (bits[binx >> 3] & (1 << (binx & 7))) {
            next();
            return true;
        }

        return false;
    }

    inline bool prs_name() {
        const char *name = start->arg.name;
        H2oNode     value;

        if (!node(name, &value)) return false;

        if (cache_Find(input->cache, value, at.text_inx)) {
            indent(2); CU_DEBUG(1, "rule \"%s\" at (%u,%u) (cached result failed\n",
                                name,
                                at.line_number + 1,
                                at.char_offset);
            return false;
        }

        bool result;

        if (checkFirstSet(value, &result)) {
            return result;
        }

        if (checkMetadata(value, &result)) {
            return result;
        }

        indent(2); CU_DEBUG(2, "rule \"%s\" at (%u,%u)\n",
                           name,
                           at.line_number + 1,
                           at.char_offset);

        // note the same name maybe call from two or more uncached nodes
        result = copper_vm(name, value, level+1, input);

        indent(2); CU_DEBUG(1, "rule \"%s\" at (%u,%u) to (%u,%u) result %s\n",
                            name,
                            at.line_number + 1,
                            at.char_offset,
                            input->cursor.line_number + 1,
                            input->cursor.char_offset,
                            (result ? "passed" : "failed"));

        return result;
    }

    inline bool prs_dot() {
        H2oChar chr;
        if (!current(&chr)) {
            return false;
        }
        next();
        return true;
    }

    inline bool prs_begin() {
        return add_event(begin_label);
    }

    inline bool prs_end() {
        return add_event(end_label);
    }

    inline bool prs_predicate() {
        if (!predicate(start->arg.name)) {
            reset();
            return false;
        }
        return true;
    }

    inline bool prs_apply() {
        const char *name  = start->arg.name;
        H2oLabel    label = { 0, name };
        H2oEvent    event = 0;

        indent(2); CU_DEBUG(2, "fetching event %s at (%u,%u)\n",
                           name,
                           at.line_number + 1,
                           at.char_offset);

        if (!input->event(input, name, &event)) {
            CU_ERROR("event %s not found\n", name);
            return false;
        }

        label.function = event;

        add_event(label);

        return true;
    }
    inline bool prs_thunk() {
        return add_event(*(start->arg.label));
    }

    inline bool prs_text() {
        const H2oChar *text = (const H2oChar *) start->arg.name;

        for ( ; 0 != *text ; ++text) {
            H2oChar chr = 0;

            if (!current(&chr)) {
                indent(2); CU_DEBUG(2, "match(\"%s\") to end-of-file at (%u,%u)\n",
                                   char2string(*text),
                                   at.line_number + 1,
                                   at.char_offset);
                reset();
                return false;
            }

            indent(2); CU_DEBUG(2, "match(\"%s\") to cursor(\'%s\') at (%u,%u)\n",
                               char2string(*text),
                               char2string(chr),
                               at.line_number + 1,
                               at.char_offset);

            if (chr != *text) {
                reset();
                return false;
            }

            next();
        }

        return true;
    }

    inline bool run_node() {
        bool result;

        if (checkFirstSet(start, &result)) {
            return result;
        }

        if (checkMetadata(start, &result)) {
            return result;
        }

        switch (start->oper) {
        case prs_Apply:       return prs_apply();
        case prs_AssertFalse: return prs_assert_false();
        case prs_AssertTrue:  return prs_assert_true();
        case prs_Begin:       return prs_begin();
        case prs_Choice:      return prs_choice();
        case prs_End:         return prs_end();
        case prs_MatchChar:   return prs_char();
        case prs_MatchDot:    return prs_dot();
        case prs_MatchName:   return prs_name();
        case prs_MatchRange:  return prs_between();
        case prs_MatchSet:    return prs_in();
        case prs_MatchText:   return prs_text();
        case prs_OneOrMore:   return prs_one_plus();
        case prs_Predicate:   return prs_predicate();
        case prs_Sequence:    return prs_and();
        case prs_Thunk:       return prs_thunk();
        case prs_ZeroOrMore:  return prs_zero_plus();
        case prs_ZeroOrOne:   return prs_optional();
        case prs_Void:        return false;
        }
        return false;
    }

    hold();

    CU_ON_DEBUG(3, {
            indent(3); CU_DEBUG(3, "check (%s) %s",
                                node_label(start),
                                oper2name(start->oper));

            if (prs_MatchName == start->oper) {
                const char *name = start->arg.name;
                CU_DEBUG(3, " %s", name);
            }
            CU_DEBUG(3, " at (%u,%u)\n",
                     at.line_number + 1,
                     at.char_offset);
        });

    if (check_cache(start)) return false;

    bool result = run_node();

    if (!result) {
        set_cache(start);
    }

    return result;

    (void)cache_Remove;
}

/*************************************************************************************
 *************************************************************************************
 *************************************************************************************
 *************************************************************************************/

extern bool h2o_RunQueue(H2oInput input) {
    if (!input) return false;
    return queue_Run(input->queue, input);
}

unsigned h2o_global_debug = 0;

extern void h2o_debug(const char *filename,
                     unsigned int linenum,
                     const char *format,
                     ...)
{
    va_list ap; va_start (ap, format);

    //    printf("file %s line %u :: ", filename, linenum);
    vprintf(format, ap);
    fflush(stdout);
}

extern void h2o_error(const char *filename,
                      unsigned int linenum,
                      const char *format,
                      ...)
{
    va_list ap; va_start (ap, format);

    printf("file %s line %u :: ", filename, linenum);
    vprintf(format, ap);
    exit(1);
}

extern void h2o_error_part(const char *format, ...)
{
    va_list ap; va_start (ap, format);
    vprintf(format, ap);
}
#endif


