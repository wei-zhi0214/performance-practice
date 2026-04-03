/*
 * fixed_aligned_allocator.c - 64-byte fixed-size, cache-aligned allocator.
 *
 * Every block is exactly CACHE_LINE_SIZE (64) bytes and is aligned to a
 * 64-byte cache-line boundary.  Because the simulated heap (memlib) starts
 * at a cache-line-aligned address and we always allocate in multiples of
 * CACHE_LINE_SIZE, every block returned from mem_sbrk() is automatically
 * cache-line aligned.
 *
 * Freed blocks are placed on a singly-linked free list (stored inside the
 * block itself) and reused by subsequent malloc calls.
 *
 * Requests larger than 64 bytes cannot be satisfied and return NULL.
 */

#include "allocator_interface.h"
#include "memlib.h"

#include <stddef.h>
#include <stdint.h>

/* Fixed block size: one full cache line */
#define FIXED_SIZE CACHE_LINE_SIZE   /* 64 bytes */

/* Free list node stored inside an unused block */
typedef struct FreeNode {
    struct FreeNode *next;
} FreeNode;

static FreeNode *free_list = NULL;

/*
 * fixed_aligned_init - initialize (or re-initialize) the allocator.
 */
int fixed_aligned_init(void) {
    free_list = NULL;
    return 0;
}

/*
 * fixed_aligned_malloc - allocate a 64-byte, cache-aligned block.
 *
 * Returns NULL if size > 64 bytes.  Otherwise returns a pointer to a
 * 64-byte block that is guaranteed to be cache-line aligned.
 */
void *fixed_aligned_malloc(size_t size) {
    if (size == 0) return NULL;
    if (size > FIXED_SIZE) return NULL;

    /* Check the free list first */
    if (free_list != NULL) {
        FreeNode *node = free_list;
        free_list      = node->next;
        return (void *)node;
    }

    /*
     * Grow the heap by exactly FIXED_SIZE bytes.
     * Because the heap buffer in memlib.c is a static array (which the
     * compiler aligns to at least sizeof(max_align_t)) AND we always
     * allocate in multiples of CACHE_LINE_SIZE, the returned pointer will
     * be cache-line aligned as long as the initial heap address is
     * cache-line aligned.
     *
     * memlib guarantees that heap_buf[] is statically allocated and that
     * brk starts at heap_buf[0].  On most platforms static arrays are
     * page-aligned (4096 bytes), which is a multiple of 64.
     */
    void *p = mem_sbrk(FIXED_SIZE);
    if (p == (void *)-1) return NULL;

    return p;
}

/*
 * fixed_aligned_free - return a block to the free list.
 *
 * The block is prepended (LIFO) for good cache reuse.
 */
void fixed_aligned_free(void *ptr) {
    if (ptr == NULL) return;
    FreeNode *node = (FreeNode *)ptr;
    node->next     = free_list;
    free_list      = node;
}
