#if !defined(_nodes_h_)
#define _nodes_h_
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
#include <water.h>
#include <copper.h>

/* */
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

//extern size_t      symbol_Make(const char* name);
extern size_t      symbol_Make(const CuData name);
extern const char* symbol_Name(size_t symbol);

typedef struct test_node  *Node_test;
typedef struct test_cell  *Cell_test;
typedef struct test_stack *Stack_test;

struct test_node {
    size_t    type; // sizeof(void*)
    unsigned  size;
    Node_test childern[];
};

struct test_cell {
    union {
        Node_test value;
        size_t    mark;
    };
    Cell_test next;
};

struct test_stack {
    Cell_test top;
    Cell_test free_list;
};

static bool GetFirst_test(Water       water __attribute__ ((unused)),
                          H2oLocation location)
{
    if (!location) return false;

    struct test_node *node = location->root;

    if (!node) return false;

    if (0 >= node->size) return false;

    location->offset  = (H2oUserMark) 0;
    location->current = (H2oUserNode) node->childern[0];

    return true;
}

static bool GetNext_test(Water       water __attribute__ ((unused)),
                         H2oLocation location)
{
    if (!location) return false;

    struct test_node *node = location->root;

    if (!node) return false;

    unsigned index = (unsigned) location->offset;

    index += 1;

    if (index >= node->size) return false;

    location->offset  = (H2oUserMark) index;
    location->current = (H2oUserNode) node->childern[index];

    return true;
}

static bool MatchNode_test(Water       water __attribute__ ((unused)),
                           H2oUserType utype,
                           H2oUserNode unode)
{
    if (!unode) return false;

    size_t            type = (size_t) utype;
    struct test_node *node = unode;

    return type == node->type;
}

static inline bool node_Print(unsigned count,
                              Node_test value)
{
    inline void indent() {
        unsigned at = count;
        while ( 0 < at--) {
            printf(". ");
        }
    }

    indent();
    if (!value) {
        printf("nil\n");
        return true;
    }

    const char* type = (const char*) value->type;

    if (type) {
        printf("%s %p\n", type, value);
    } else {
        printf("type[%Zu] %p\n", value->type, value);
    }

    unsigned index = 0;
    for ( ; index < value->size ; ++index) {
        node_Print(count + 1, value->childern[index]);
    }

    return true;
}

static inline bool cell_Create(Cell_test *target) {
    if (!target) return false;

    Cell_test result = malloc(sizeof(struct test_cell));

    if (!result) return false;

    memset(result, 0, sizeof(struct test_cell));

    *target = result;

    return true;
}

static inline bool stack_Init(unsigned count, Stack_test target) {
    if (!target) return false;

    memset(target, 0, sizeof(struct test_stack));

    if (count < 1) return true;

    Cell_test list = 0;
    Cell_test end  = 0;

    if (!cell_Create(&list)) return false;

    target->free_list = end = list;

    while (count--) {
        if (!cell_Create(&end->next)) return false;
    }

    return true;
}

static inline bool stack_Depth(Stack_test stack, size_t *count) {
    if (!stack) return false;
    if (!count) return false;

    size_t size = 0;

    Cell_test cell = stack->top;

    for(; cell ;) {
        size += 1;
        cell = cell->next;
    }

    *count = size;

    return true;
}

static inline bool stack_PushNode(Stack_test stack, Node_test value) {
    if (!stack) return false;
    if (!value) return false;

    Cell_test cell = stack->free_list;

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

static inline bool stack_PopNode(Stack_test stack, Node_test *value) {
    if (!stack)      return false;
    if (!value)      return false;
    if (!stack->top) return false;

    Cell_test cell = stack->top;

    stack->top = cell->next;

    cell->next = stack->free_list;
    stack->free_list = cell;

    *value = cell->value;

    return true;
}

static inline bool stack_PushMark(Stack_test stack, size_t mark) {
    if (!stack) return false;

    Cell_test cell = stack->free_list;

    if (cell) {
        stack->free_list = cell->next;
    } else {
        if (!cell_Create(&cell)) return false;
    }

    cell->next = stack->top;
    cell->mark = mark;

    stack->top = cell;

    return true;
}

static inline bool stack_PopMark(Stack_test stack, size_t *mark) {
    if (!stack)      return false;
    if (!mark)       return false;
    if (!stack->top) return false;

    Cell_test cell = stack->top;

    stack->top = cell->next;

    cell->next = stack->free_list;
    stack->free_list = cell;

    *mark = cell->mark;

    return true;
}

static inline bool stack_Dup(Stack_test stack) {
    if (!stack)      return false;
    if (!stack->top) return false;

    Cell_test top  = stack->top;
    Cell_test cell = stack->free_list;

    if (cell) {
        stack->free_list = cell->next;
    } else {
        if (!cell_Create(&cell)) return false;
    }

    cell->next = stack->top;
    stack->top = cell;

    cell->value = top->value;

    return true;
}

static inline bool node_Create(Node_test *target, const CuData name, unsigned size, Stack_test stack) {
    if (!target) return false;

    unsigned fullsize = sizeof(struct test_node) + (sizeof(Node_test) * size);

    Node_test result = malloc(fullsize);

    if (!result) return false;

    result->type = symbol_Make(name);
    result->size = size;

    if (0 >= size) {
        *target = result;
        return true;
    }

    for ( ; size-- ; ) {
        if (!stack_PopNode(stack, &result->childern[size])) return false;
    }

    *target = result;

    return true;
}

#endif
