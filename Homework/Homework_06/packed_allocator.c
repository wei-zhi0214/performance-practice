/*
 * packed_allocator.c - A tightly-packed bump allocator with per-object headers.
 *
 * Each allocation is laid out as:
 *
 *   [ size_t header (8 bytes) | user data (ALIGN8(size) bytes) ]
 *
 * The bump pointer advances by (sizeof(size_t) + ALIGN8(size)) bytes for
 * each malloc call, ensuring that the user data region is always 8-byte
 * aligned.  The header stores the original requested size so that
 * packed_free can (in principle) identify the block.
 *
 * Because this is a bump allocator, packed_free is effectively a no-op:
 * we cannot reclaim interior blocks without a compacting GC.  This gives
 * maximum packing density (minimum heap size) at the cost of no recycling.
 */

#include "allocator_interface.h"
#include "memlib.h"

#include <stddef.h>
#include <stdint.h>

/*
 * packed_init - reset the bump pointer.
 * memlib owns the heap; we just reset its break pointer.
 */
int packed_init(void) {
    mem_reset_brk();
    return 0;
}

/*
 * packed_malloc - allocate `size` bytes with an 8-byte-aligned layout.
 *
 * Layout:
 *   [  size_t header  ][  ALIGN8(size) bytes of user data  ]
 *
 * Returns a pointer to the user-data region (just past the header).
 */
void *packed_malloc(size_t size) {
    if (size == 0) return NULL;

    size_t data_size  = ALIGN8(size);
    size_t total_size = sizeof(size_t) + data_size;

    /* Extend the heap */
    void *raw = mem_sbrk((int)total_size);
    if (raw == (void *)-1) return NULL;

    /* Store the original requested size in the header */
    size_t *header = (size_t *)raw;
    *header = size;

    /* Return pointer to the user data region */
    return (void *)(header + 1);
}

/*
 * packed_free - mark a block as freed.
 *
 * In a bump allocator we cannot actually reclaim the memory, but we can
 * read back the header to verify the pointer is valid (debugging aid).
 * In production this is a no-op.
 */
void packed_free(void *ptr) {
    if (ptr == NULL) return;

    /* The header sits immediately before the user pointer */
    /* (void *)ptr - sizeof(size_t) would be UB through void* arithmetic
       in standard C, so we cast to char* first) */
    /* size_t *header = (size_t *)((char *)ptr - sizeof(size_t)); */
    /* size_t stored_size = *header; */
    /* Nothing we can do with it in a bump allocator - just discard. */
    (void)ptr;
}
