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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>


typedef struct test_node  *Node_test;
typedef struct test_cell  *Cell_test;
typedef struct test_stack *Stack_test;

struct test_node {
    unsigned type;
    unsigned size;
    Node_test childern[];
};

struct test_cell {
    Node_test value;
    Cell_test next;
};

struct test_stack {
    Cell_test top;
    Cell_test free_list;
};

static bool GetFirst_test(Water water, H2oLocation location) {
    if (!water)    return false;
    if (!location) return false;

    struct test_node *node = location->root;

    if (!node) return false;

    // if (!hasChildern(node->type)) return false;

    if (0 >= node->size) return false;

    location->offset  = (H2oUserMark) 0;
    location->current = (H2oUserNode) node->childern[0];

    return true;
}

static bool GetNext_test(Water water, H2oLocation location) {
    if (!water)    return false;
    if (!location) return false;

    struct test_node *node = location->root;

    if (!node) return false;

    // if (!hasChildern(node->type)) return false;

    unsigned index = (unsigned) location->offset;

    index += 1;

    if (index >= node->size) return false;

    location->offset  = (H2oUserMark) index;
    location->current = (H2oUserNode) node->childern[index];

    return true;
}

static bool MatchNode_test(Water water,  H2oUserType utype, H2oUserNode unode) {
    if (!water) return false;
    if (!unode)  return false;

    unsigned          type = (unsigned) utype;
    struct test_node *node = unode;

    return type == node->type;
}

static bool cell_Create(Cell_test *target) {
    if (!target) return false;

    Cell_test result = malloc(sizeof(struct test_cell));

    if (!result) return false;

    memset(result, 0, sizeof(struct test_cell));

    *target = result;

    return true;
}

static bool stack_Init(unsigned count, Stack_test target) {
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

static bool stack_Push(Stack_test stack, Node_test value) {
    if (!stack)     return false;
    if (!value.any) return false;

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

static bool stack_Pop(Stack_test stack, Node_test *value) {
    if (!stack)      return false;
    if (!value.node) return false;
    if (!stack->top) return false;

    Cell_test cell = stack->top;

    stack->top = cell->next;

    cell->next = stack->free_list;
    stack->free_list = cell;

    *value = cell->value;

    return true;
}

static bool stack_Swap(Stack_test stack) {
    if (!stack)      return false;
    if (!stack->top) return false;

    Cell_test stk1 = stack->top;

    if (!stk1->next) return false;

    Cell_test stk2 = stk1->next;
    Cell_test stk3 = stk2->next;

    stk1->next = stk3;
    stk2->next = stk1;
    stack->top = stk2;

    return true;
}

static bool node_Create(Node_test *target, unsigned type, unsigned size, Stack_test stack) {
    if (!target) return false;

    unsigned fullsize = sizeof(struct test_node) + (sizeof(Node_test) * size);

    Node_test result = malloc(fullsize);

    if (!result) return false;

    result->type = type;
    result->size = size;

    if (0 >= size) {
        *target = result;
        return true;
    }

    for ( ; size-- ; ) {
        if (!stack_Pop(stack, &result->childern[size])) return false;
    }

    return true;
}

#endif
