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
/***************************
 **
 ** Project: Water
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

typedef struct water_marker *H2oMaker;

struct water_marker {
    struct water_location location;
    H2oThread             end;
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

static bool water_vm(Water water, unsigned level, H2oCode start)
{
    inline void indent() {
        if (2 > h2o_global_debug) return;
        int xxx = 0;
        for ( ; xxx < level ; ++xxx) {
            fprintf(stderr, ". ");
        }
        return;
    }

    // mark the queue and the location
    inline bool mark(H2oMaker marker)  {
        if (!marker) return false;
        indent();
        H2O_DEBUG(2, "setting marker %x %x\n",
                  (unsigned) water->cursor.current,
                  (unsigned) water->end);
        marker->location = water->cursor;
        marker->end      = water->end;
        return true;
    };

    // reset the queue to mark and the location
    inline bool reset(H2oMaker marker) {
        if (!marker) return false;

        water->cursor = marker->location;

        H2oThread current = 0 ;

        if (!marker->end) {
            current      = water->begin;
            water->end   = 0;
            water->begin = 0;
        } else {
            water->end = marker->end;
            current    = marker->end->next;
            marker->end->next  = 0;
        }

        while (current) {
            H2oThread next = current->next;
            current->next = water->free_list;
            water->free_list = current;
            current = next;
        }

        indent();
        H2O_DEBUG(2, "resetting marker %x %x\n",
                  (unsigned) water->cursor.current,
                  (unsigned) water->end);

        return true;
    }

    inline bool fetch_first(H2oLocation location) {
        if (!location->root) return false;
        return water->first(water, location);
    }

    inline bool fetch_next(H2oLocation location) {
        if (!location->root) return false;
        return water->next(water, location);
    }

    inline bool next_node() {
        struct water_location check = water->cursor;
        indent();
        H2O_DEBUG(2, "check next node %x[%u] -> %x\n",
                  (unsigned) check.root,
                  (unsigned) check.offset,
                  (unsigned) check.current);

        if (!fetch_next(&check)) return false;
        water->cursor = check;
        indent();
        H2O_DEBUG(2, "next node %x[%u] -> %x\n",
                  (unsigned) check.root,
                  (unsigned) check.offset,
                  (unsigned) check.current);
        return true;
    }

    inline bool as_root(H2oLocation location) {
        if (!water->cursor.current) return false;
        location->root    = water->cursor.current;
        location->offset  = 0;
        location->current = 0;
        return true;
    }

    inline bool swap_location(H2oLocation location) {
        struct water_location holding = water->cursor;
        water->cursor = *location;
        *location = holding;
        return true;
    }

    inline bool call_with(H2oCode code) {
        return water_vm(water, level + 1, code);
    }

    inline void* fetch_code(H2oAction action) {
        H2oCache cache = action->cache;
        return cache->values[action->index];
    }

    inline bool apply_code(H2oAction action) {
        H2oCode code = fetch_code(action);
        if (!code) return false;
        indent(); H2O_DEBUG(2, "calling rule %s\n", action->name);
        bool result =  call_with(code);
        indent(); H2O_DEBUG(2, "result of rule %s - %s\n", action->name, (result ? "true" : "false"));
        return result;
    }

    inline bool add_event(H2oAction action) {
        H2oEvent event = fetch_code(action);
        if (!event) return false;

        indent(); H2O_DEBUG(2, "adding event --  %s %x\n", action->name, (unsigned) water->cursor.current);

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
        if (!predicate) return false;
        indent(); H2O_DEBUG(2, "apply predicate %s\n", action->name);
        return predicate(water, water->cursor.current);
    }

    inline bool match_root(H2oAction action) {
        H2oUserType type = fetch_code(action);
        if (!type) return false;
        indent(); H2O_DEBUG(2, "testing root %s\n", action->name);
        if (!(water->match)(water, type, water->cursor.current)) {
            indent(); H2O_DEBUG(2, "testing root %s - false\n", action->name);
            return false;
        }
        return true;
    }

    inline bool water_any() { return 0 != water->cursor.current; }

    inline bool water_and() {
        H2oChain chain = (H2oChain) start;
        struct water_marker marker;
        if (!mark(&marker)) return false;
        if (!call_with(chain->before)) return false;
        if (call_with(chain->after))   return true;
        reset(&marker);
        return false;
    }

    inline bool water_or() {
        H2oChain chain = (H2oChain) start;
        if (call_with(chain->before)) return true;
        if (call_with(chain->after))  return true;
        return false;
    }

    inline bool water_not() {
        H2oFunction function = (H2oFunction) start;
        struct water_marker marker;
        if (!mark(&marker)) return false;
        bool test = call_with(function->argument);
        if (!reset(&marker)) return false;
        return (test ? false : true);
    }

    inline bool water_assert() {
        H2oFunction function = (H2oFunction) start;
        struct water_marker marker;
        if (!mark(&marker)) return false;
        bool test = call_with(function->argument);
        if (!reset(&marker)) return false;
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
        struct water_location here;

        if (!as_root(&here))       return false;
        if (!fetch_first(&here))   return false;
        if (!swap_location(&here)) return false;

        indent();
        H2O_DEBUG(2, "first node %x[%u] -> %x\n",
                  (unsigned) water->cursor.root,
                  (unsigned) water->cursor.offset,
                  (unsigned) water->cursor.current);

        bool result = call_with(function->argument);

        water->cursor = here;
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
        struct water_marker marker;
        if (!mark(&marker)) return false;
        if (!call_with(chain->before)) return false;
        if (next_node()) {
            if (call_with(chain->after)) return true;
        }
        reset(&marker);
        return false;
    }

    inline bool water_sequence() {
        H2oChain chain = (H2oChain) start;
        struct water_marker marker;
        if (!mark(&marker)) return false;
        if (!call_with(chain->before)) return false;
        if (call_with(chain->after))   return true;
        reset(&marker);
        return false;
    }

    inline bool water_select() {
        H2oChain chain = (H2oChain) start;
        if (call_with(chain->before)) return true;
        if (call_with(chain->after))  return true;
        return false;
    }

    inline bool water_zero_plus() {
        H2oFunction function = (H2oFunction) start;
        if (!call_with(function->argument)) {
            return true;
        }
        struct water_location last = water->cursor;
        if (next_node()) {
            while (call_with(function->argument)) {
                last = water->cursor;
                if (!next_node()) break;
            }
        }
        water->cursor = last;
        return true;
    }

    inline bool water_one_plus() {
        H2oFunction function = (H2oFunction) start;
        if (!call_with(function->argument)) {
            return false;
        }
        struct water_location last = water->cursor;
        if (next_node()) {
            while (call_with(function->argument)) {
                last = water->cursor;
                if (!next_node()) break;
            }
        }
        water->cursor = last;
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
            struct water_marker marker;
            if (!mark(&marker)) return false;
            if (!call_with(group->argument)) return false;
            for ( ; --index ; ) {
                if (next_node()) {
                    if (call_with(group->argument)) continue;
                }
                reset(&marker);
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

    inline bool water_leaf() {
        struct water_location check;
        check.root    = water->cursor.current;
        check.offset  = 0;
        check.current = 0;
        return !(water->first)(water, &check);
    }

    inline bool run_code() {
        switch (start->oper) {
        case water_And:       return water_and();       // is the root A and B
        case water_Any:       return water_any();       // match any root
        case water_Apply:     return water_apply();     // apply a named code to root
        case water_Assert:    return water_assert();    // assert that the root is A     (dont add to the queue)
        case water_Begin:     return water_begin();     // check for a first child
        case water_Childern:  return water_childern();  // match childern
        case water_End:       return water_end();       // check for no more siblings
        case water_Event:     return water_event();     // add a event for this node to the event queue
        case water_Leaf:      return water_leaf();      // check for no childern
        case water_Maybe:     return water_maybe();     // check then match
        case water_Not:       return water_not();       // assert that the root is not A (dont add to the queue)
        case water_OnePlus:   return water_one_plus();  // match one+
        case water_Or:        return water_or();        // is the root A or  B
        case water_Predicate: return water_predicate(); // apply a named predicate to root
        case water_Range:     return water_range();     // match range
        case water_Root:      return water_root();      // match root
        case water_Select:    return water_or();        // match one
        case water_Sequence:  return water_and();       // check all
        case water_Tuple:     return water_tuple();     // match all
        case water_ZeroPlus:  return water_zero_plus(); // match zero+
        default: break;
        }

        return false;
    }

    assert(0 != water);
    assert(0 != start);

    indent(); H2O_DEBUG(2, "operation %s %s on %x\n", oper2text(start->oper), start->label, (unsigned) water->cursor.current);

    bool result = run_code();

    indent(); H2O_DEBUG(2, "operation %s %s on %x - %s\n", oper2text(start->oper), start->label, (unsigned) water->cursor.current,
                        (result ? "true" : "false"));

    return result;
}


typedef void  *H2o_Value;
typedef bool (*H2o_FetchValue)(Water, H2oUserName, H2o_Value*);

static bool reload_Cache(Water water, H2oCache cache) {
    if (!water) return false;
    if (!cache) return true;

    if (0 >= cache->count) {
        return reload_Cache(water, cache->next);
    }

    unsigned size = sizeof(void *) * cache->count;

    if (!cache->values) {
        cache->values = malloc(size);
        if (!cache->values) return false;
    }

    const char **names  = cache->names;
    void       **values = cache->values;

    H2o_FetchValue fetch;

    inline bool fetch_value(unsigned index) {
        const char *name = names[index];
        if (!fetch(water, name, &values[index])) {
            fprintf(stderr, "unable to find %s\n", name);
            return false;
        }
        return true;
    }

    switch (cache->type) {
    case rule_cache:
        fetch = (H2o_FetchValue) water->code;
        break;

    case root_cache:
        fetch = (H2o_FetchValue) water->type;
        break;

    case event_cache:
        fetch = (H2o_FetchValue) water->event;
        break;

    case predicate_cache:
        fetch = (H2o_FetchValue) water->predicate;
        break;

    case cache_void:
        return false;
    }

    memset(values, 0, size);

    unsigned index = 0;
    for ( ; index < cache->count; ++index) {
        if (!fetch_value(index)) return false;
    }

    return reload_Cache(water, cache->next);
}

/*************************************************************************************
 *************************************************************************************
 *************************************************************************************
 *************************************************************************************/

extern bool h2o_WaterInit(Water water, unsigned cacheSize) {
    if (!water) return false;

    return true;
}

extern bool h2o_Parse(Water water, const char* rule, H2oUserNode tree) {
    if (!water) return false;

    if (!reload_Cache(water, water->cache)) return false;

    H2oCode code;

    if (!water->code(water, rule, &code)) return false;

    water->cursor.root    = 0;
    water->cursor.offset  = 0;
    water->cursor.current = tree;

    return water_vm(water, 0, code);
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
    vfprintf(stderr, format, ap);
    fflush(stderr);
}

extern void h2o_error(const char *filename,
                      unsigned int linenum,
                      const char *format,
                      ...)
{
    va_list ap; va_start (ap, format);
    vfprintf(stderr, format, ap);
    exit(1);
}

extern void h2o_error_part(const char *format, ...) {
    va_list ap; va_start (ap, format);
    vfprintf(stderr, format, ap);
}

extern bool h2o_AddName(Water water, const char* name, const H2oCode code) {
    if (!water)         return false;
    if (!water->attach) return false;

    return water->attach(water, name, code);
}

extern bool h2o_AddCache(Water water, H2oCache cache) {
    if (!water)                    return false;
    if (!cache)                    return false;
    if (cache->next)               return false;
    if (cache_void == cache->type) return false;

    cache->next = water->cache;
    water->cache = cache;

    return true;
}






