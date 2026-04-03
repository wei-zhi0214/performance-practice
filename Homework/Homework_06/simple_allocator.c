/*
 * simple_allocator.c - A fixed-size block allocator.
 *
 * All allocations are rounded up to BLOCK_SIZE (default 1024 bytes).
 * Freed blocks are kept on a singly-linked free list and reused by
 * subsequent malloc calls.
 *
 * Because every block is the same size, the allocator achieves ~100%
 * utilization on workloads whose requests never exceed BLOCK_SIZE.
 */

#include "allocator_interface.h"
#include "memlib.h"

#include <stddef.h>

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 1024
#endif

/* Node stored inside a free block */
typedef struct FreeNode {
    struct FreeNode *next;
} FreeNode;

static FreeNode *free_list_head = NULL;

/*
 * simple_init - initialize (or re-initialize) the allocator.
 * Called once before a sequence of malloc/free operations.
 */
int simple_init(void) {
    free_list_head = NULL;
    return 0;
}

/*
 * simple_malloc - allocate a block of at least `size` bytes.
 *
 * If size > BLOCK_SIZE, returns NULL (cannot satisfy request).
 * Otherwise every allocation is exactly BLOCK_SIZE bytes.
 * Checks the free list first; if empty, extends the heap.
 */
void *simple_malloc(size_t size) {
    if (size == 0) return NULL;
    if (size > (size_t)BLOCK_SIZE) return NULL;

    /* Round up to BLOCK_SIZE */
    size = BLOCK_SIZE;

    if (free_list_head != NULL) {
        /* Reuse a previously freed block */
        FreeNode *node  = free_list_head;
        free_list_head  = node->next;
        return (void *)node;
    }

    /* Grow the heap */
    void *p = mem_sbrk(BLOCK_SIZE);
    if (p == (void *)-1) return NULL;
    return p;
}

/*
 * simple_free - return a block to the free list.
 *
 * The block is prepended to the free list so that the next malloc can
 * reclaim it immediately (LIFO order gives good cache behaviour).
 */
void simple_free(void *ptr) {
    if (ptr == NULL) return;
    FreeNode *node  = (FreeNode *)ptr;
    node->next      = free_list_head;
    free_list_head  = node;
}
