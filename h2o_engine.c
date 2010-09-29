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
#include "water.h"

/* */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <stdarg.h>
#include <assert.h>

/* */
struct water_thread {
    H2oThread next;
    H2oEvent  event;
    H2oUserNode  node;
};

static bool make_Thread(H2oEvent event,
                        H2oUserNode node,
                        H2oThread *target)
{
    struct water_thread *result = malloc(sizeof(struct water_thread));

    result->next  = 0;
    result->event = event;
    result->node  = node;

    *target = result;

    return true;
}

static bool water_vm(Water water, H2oCode start)
{
    assert(0 != water);
    assert(0 != start);

    struct {
        struct water_location location;
        H2oThread             end;
    } marker;

    // mark the queue and the location
    inline bool mark()  {
        marker.location = water->cursor;
        marker.end      = water->end;
        return true;
    };

    inline bool next_node() {
        if (!water->cursor.root) return false;
        struct water_location check = water->cursor;
        if (!(water->next)(water, &check)) return false;
        water->cursor = check;
        return true;
    }

    // reset the queue to mark and the location
    inline bool reset() {
        water->cursor = marker.location;
        water->end    = marker.end;
        H2oThread current = marker.end->next;
        marker.end->next  = 0;
        while (current) {
            H2oThread next = current->next;
            current->next = water->free_list;
            water->free_list = current;
            current = next;
        }
        return true;
    }

    inline bool call_with(H2oCode code) {
        return water_vm(water, code);
    }

    inline void* fetch_code(H2oAction action) {
        H2oCache cache  = action->cache;
        return cache->values[action->index];
    }

    inline bool apply_code(H2oAction action) {
        H2oCode code = fetch_code(action);
        if (!(water->code)(water, (H2oUserName) index, &code)) return false;
        return call_with(code);
    }

    inline bool add_event(H2oAction action) {
        H2oEvent event = fetch_code(action);
        if (!(water->event)(water, (H2oUserName) index, &event)) return false;
        H2oThread value = water->free_list;
        if (!value) {
            if (!make_Thread(event, water->cursor.current, &value)) return false;
        } else {
            water->free_list = value->next;
            value->next = 0;
            value->event = event;
            value->node  = water->cursor.current;
        }

        if (!water->begin) {
            water->begin = water->end = value;
        } else {
            water->end->next = value;
            water->end = value;
        }

        return true;
    }
    inline bool apply_predicate(H2oAction action) {
        H2oPredicate predicate = fetch_code(action);
        if (!(water->predicate)(water, (H2oUserName) index, &predicate)) return false;
        return predicate(water, water->cursor.current);
    }
    inline bool match_root(H2oAction action) {
        H2oUserType type = fetch_code(action);
        if (!(water->type)(water, (H2oUserName) index, &type))   return false;
        if (!(water->match)(water, type, water->cursor.current)) return false;
        return true;
    }

    inline bool water_any() { return 0 != water->cursor.current; }

    inline bool water_and() {
        H2oChain chain = (H2oChain) start;
        if (!mark()) return false;
        if (!call_with(chain->before)) return false;
        if (call_with(chain->before))  return true;
        reset();
        return false;
    }

    inline bool water_or() {
        H2oChain chain = (H2oChain) start;
        if (call_with(chain->before)) return true;
        if (call_with(chain->before)) return true;
        return false;
    }

    inline bool water_not() {
        H2oFunction function = (H2oFunction) start;
        if (!mark()) return false;
        bool test = call_with(function->argument);
        if (!reset()) return false;
        return (test ? false : true);
    }

    inline bool water_assert() {
        H2oFunction function = (H2oFunction) start;
        if (!mark()) return false;
        bool test = call_with(function->argument);
        if (!reset()) return false;
        return test;
    }

    inline bool water_apply() {
        H2oAction action = (H2oAction) start;
        return apply_code(action);
    }

    inline bool water_root() {
        H2oAction action = (H2oAction) start;
        return match_root(action);
    }

    inline bool water_childern() {
        H2oFunction function = (H2oFunction) start;
        struct water_location holding = water->cursor;
        water->cursor.root    = holding.current;
        water->cursor.offset  = 0;
        water->cursor.current = 0;
        bool result = call_with(function->argument);
        water->cursor = holding;
        return result;
    }

    inline bool water_event() {
        H2oAction action = (H2oAction) start;
        return add_event(action);
    }

    inline bool water_predicate() {
        H2oAction action = (H2oAction) start;
        return apply_predicate(action);
    }

    inline bool water_begin() {
        struct water_location check = { water->cursor.current, 0, 0 };
        if (!(water->first)(water, &check)) return false;
        water->cursor = check;
        return true;
    }

    inline bool water_tuple() {
        H2oChain chain = (H2oChain) start;
        if (!mark()) return false;
        if (!call_with(chain->before)) return false;
        if (next_node()) {
            if (call_with(chain->before)) return true;
        }
        reset();
        return false;
    }

    inline bool water_select() {
        H2oChain chain = (H2oChain) start;
        if (call_with(chain->before)) return true;
        if (call_with(chain->before)) return true;
        return false;
    }

    inline bool water_sequence() {
        H2oChain chain = (H2oChain) start;
        if (!call_with(chain->before)) return false;
        if (call_with(chain->before))  return true;
        return false;
    }

    inline bool water_zero_plus() {
        H2oFunction function = (H2oFunction) start;
        while (call_with(function->argument)) {
            if (!next_node()) return true;
        }
        return true;
    }

    inline bool water_one_plus() {
        H2oFunction function = (H2oFunction) start;
        if (!call_with(function->argument)) return false;
        if (!next_node()) return true;
        while (call_with(function->argument)) {
            if (!next_node()) return true;
        }
        return true;
    }

    inline bool water_maybe() {
        H2oFunction function = (H2oFunction) start;
        call_with(function->argument);
        return true;
    }

    inline bool water_range() {
        H2oGroup group = (H2oGroup) start;
        unsigned index = group->minimum;

        if (0 < index) {
            if (!mark()) return false;
            if (!call_with(group->argument)) return false;
            for ( ; --index ; ) {
                if (next_node()) {
                    if (call_with(group->argument)) continue;
                }
                reset();
                return false;
            }
        }

        index = group->maximum;

        if (0 == index) {
            if (!next_node()) return true;
            while (call_with(group->argument)) {
                if (!next_node()) break;
            }
        } else {
            if (!next_node()) return true;
            for ( ; index-- ; ) {
                if (!call_with(group->argument)) return true;
                if (!next_node()) break;
            }
        }
        return true;
    }

    inline bool water_end() {
        struct water_location check = water->cursor;
        return !(water->next)(water, &check);
    }

    switch (start->oper) {
    case water_Any:       return water_any();       // match any root
    case water_And:       return water_and();       // is the root A and B
    case water_Or:        return water_or();        // is the root A or  B
    case water_Not:       return water_not();       // assert that the root is not A (dont add to the queue)
    case water_Assert:    return water_assert();    // assert that the root is A     (dont add to the queue)
    case water_Apply:     return water_apply();     // apply a named code to root
    case water_Root:      return water_root();      // match root
    case water_Childern:  return water_childern();  // match childern
    case water_Event:     return water_event();     // add a event for this node to the event queue
    case water_Predicate: return water_predicate(); // apply a named predicate to root
    case water_Begin:     return water_begin();     // check for a first child
    case water_Tuple:     return water_tuple();     // match all
    case water_Select:    return water_select();    // match one
    case water_Sequence:  return water_sequence();  // check all
    case water_ZeroPlus:  return water_zero_plus(); // match zero+
    case water_OnePlus:   return water_one_plus();  // match one+
    case water_Maybe:     return water_maybe();     // check then match
    case water_Range:     return water_range();     // match range
    case water_End:       return water_end();       // check for no child
    default: break;
    }

    return false;
}

/*************************************************************************************
 *************************************************************************************
 *************************************************************************************
 *************************************************************************************/

extern bool h2o_WaterInit(Water water, unsigned cacheSize) {
    return true;
}

extern bool h2o_Parse(Water water, const char* rule, H2oUserNode tree) {

}


extern bool h2o_RunQueue(Water water) {
    if (!water) return false;
    if (!water->begin) return true;

    H2oThread current = water->begin;

    for ( ; current ; ) {
        H2oEvent function = current->event;

        if (!function(water, current->node)) {
            water->begin = current;
            return false;
        }

        H2oThread next   = current->next;
        current->next    = water->free_list;
        water->free_list = current;
        current = next;
    }

    return true;
}

unsigned h2o_global_debug = 0;

extern void h2o_debug(const char *filename,
                     unsigned int linenum,
                     const char *format,
                     ...)
{
    va_list ap; va_start (ap, format);

    fprintf(stderr, "file %s line %u :: ", filename, linenum);
    vfprintf(stderr, format, ap);
    fflush(stderr);
}

extern void h2o_error(const char *filename,
                      unsigned int linenum,
                      const char *format,
                      ...)
{
    va_list ap; va_start (ap, format);

    fprintf(stderr, "file %s line %u :: ", filename, linenum);
    vfprintf(stderr, format, ap);
    exit(1);
}

extern void h2o_error_part(const char *format, ...)
{
    va_list ap; va_start (ap, format);
    vfprintf(stderr, format, ap);
}

extern bool h2o_AddName(Water water, const char* name, const H2oCode code) {
    if (!water)         return false;
    if (!water->attach) return false;

    return water->attach(water, name, code);
}
extern bool h2o_AddCache(Water water, H2oCache cache) {
    if (!water) return false;
    if (!cache) return false;
    if (cache->next) return false;

    switch (cache->type) {
    case rule_cache:
        cache->next = water->rules;
        water->rules = cache;
        break;
    case root_cache:
        cache->next = water->roots;
        water->roots = cache;
        break;
    case event_cache:
        cache->next = water->events;
        water->events = cache;
        break;
    case predicate_cache:
        cache->next = water->predicates;
        water->predicates = cache;
        break;
    case cache_void:
        return false;
    }

    return true;
}






