/*
 * smart_allocator.c - A two-size-class allocator for 32- and 64-byte blocks.
 *
 * Size classes:
 *   Small : <= 32 bytes  ->  32-byte block
 *   Large : <= 64 bytes  ->  64-byte block
 *   > 64 bytes           ->  NULL (unsupported)
 *
 * The tag encoding used in the returned pointer:
 *   - Small allocation: returned value = real_ptr | 1  (low bit set)
 *   - Large allocation: returned value = real_ptr      (low bit clear)
 *
 * Because allocations are at least 8-byte aligned, the low bit of a real
 * data pointer is always 0, so encoding one metadata bit there is safe.
 *
 * The caller retrieves the actual usable pointer with:
 *   SMART_PTR(p)   ->  clears the low bit
 *   IS_SMALL(p)    ->  tests the low bit
 *
 * Both free lists are singly-linked and store the next pointer inside the
 * free block itself.
 */

#include "allocator_interface.h"
#include "memlib.h"

#include <stddef.h>
#include <stdint.h>

#define SMALL_SIZE  32   /* bytes per small block */
#define LARGE_SIZE  64   /* bytes per large block */

/* Free list node (stored inside an idle block) */
typedef struct FreeNode {
    struct FreeNode *next;
} FreeNode;

static FreeNode *small_free_list = NULL;
static FreeNode *large_free_list = NULL;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/*
 * alloc_small - obtain a 32-byte block (from free list or heap).
 */
static void *alloc_small(void) {
    if (small_free_list != NULL) {
        FreeNode *node   = small_free_list;
        small_free_list  = node->next;
        return (void *)node;
    }
    void *p = mem_sbrk(SMALL_SIZE);
    if (p == (void *)-1) return NULL;
    return p;
}

/*
 * alloc_large - obtain a 64-byte block (from free list or heap).
 */
static void *alloc_large(void) {
    if (large_free_list != NULL) {
        FreeNode *node   = large_free_list;
        large_free_list  = node->next;
        return (void *)node;
    }
    void *p = mem_sbrk(LARGE_SIZE);
    if (p == (void *)-1) return NULL;
    return p;
}

/* -------------------------------------------------------------------------
 * Public interface
 * ---------------------------------------------------------------------- */

/*
 * smart_init - initialize both free lists.
 */
int smart_init(void) {
    small_free_list = NULL;
    large_free_list = NULL;
    return 0;
}

/*
 * smart_malloc - allocate a block for the requested size.
 *
 * Returns a tagged pointer:
 *   small allocation -> real_ptr | 1
 *   large allocation -> real_ptr (tag = 0)
 *   failure          -> NULL
 */
void *smart_malloc(size_t size) {
    if (size == 0) return NULL;

    if (size <= SMALL_SIZE) {
        void *p = alloc_small();
        if (p == NULL) return NULL;
        /* Tag the low bit to indicate a small block */
        return (void *)((uintptr_t)p | (uintptr_t)1);
    }

    if (size <= LARGE_SIZE) {
        void *p = alloc_large();
        /* Low bit is 0 => large block (no tagging needed) */
        return p;
    }

    /* Unsupported size */
    return NULL;
}

/*
 * smart_free - return a block to the appropriate free list.
 *
 * Uses the tag macros SMART_PTR() and IS_SMALL() defined in
 * allocator_interface.h to decode the pointer.
 */
void smart_free(void *ptr) {
    if (ptr == NULL) return;

    int small = IS_SMALL(ptr);
    void *real = SMART_PTR(ptr);

    FreeNode *node = (FreeNode *)real;

    if (small) {
        node->next      = small_free_list;
        small_free_list = node;
    } else {
        node->next      = large_free_list;
        large_free_list = node;
    }
}
