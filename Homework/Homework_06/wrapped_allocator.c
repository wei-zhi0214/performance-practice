/*
 * wrapped_allocator.c - A cache-line-aligned bump allocator.
 *
 * Every allocation is aligned to a 64-byte cache-line boundary.
 * We achieve alignment by allocating (size + CACHE_LINE_SIZE - 1) extra
 * bytes and rounding the raw pointer up to the next cache-line boundary.
 *
 * Because the alignment offset is discarded, wrapped_free is a no-op
 * (we cannot recover the original pointer to "free" back to the heap).
 * This is fine for workloads that never free, or where the performance
 * measurement only cares about allocation throughput.
 */

#include "allocator_interface.h"
#include "memlib.h"

#include <stddef.h>
#include <stdint.h>

/*
 * unaligned_malloc - simple bump allocator that does not free.
 *
 * Extends the heap by exactly `size` bytes and returns a pointer to
 * the start of that region.
 */
void *unaligned_malloc(size_t size) {
    if (size == 0) return NULL;
    void *p = mem_sbrk((int)size);
    if (p == (void *)-1) return NULL;
    return p;
}

/*
 * wrapped_init - initialise the wrapped allocator.
 * Nothing to do because we rely on the simulated heap (memlib).
 */
int wrapped_init(void) {
    return 0;
}

/*
 * wrapped_malloc - allocate `size` bytes aligned to a cache-line boundary.
 *
 * Strategy:
 *   1. Allocate (size + CACHE_LINE_SIZE - 1) raw bytes so that there is
 *      always room for a full cache-line-aligned block of `size` bytes
 *      somewhere inside the raw region.
 *   2. Round the raw pointer UP to the next CACHE_LINE_SIZE boundary.
 *   3. Return that aligned pointer.
 *
 * The padding bytes before the aligned pointer are permanently wasted,
 * which is the price of alignment without storing metadata.
 */
void *wrapped_malloc(size_t size) {
    if (size == 0) return NULL;

    /* Allocate enough raw bytes to guarantee an aligned block of `size` */
    size_t total = size + CACHE_LINE_SIZE - 1;
    void *raw = unaligned_malloc(total);
    if (raw == NULL) return NULL;

    /* Round up to the next cache-line boundary */
    uintptr_t addr    = (uintptr_t)raw;
    uintptr_t aligned = ALIGN_CACHE(addr);
    return (void *)aligned;
}

/*
 * wrapped_free - no-op.
 *
 * We cannot reclaim the padding bytes in front of the aligned pointer
 * without storing metadata (which would cost extra memory and may not be
 * required by the spec). Freed blocks simply remain in the heap.
 */
void wrapped_free(void *ptr) {
    (void)ptr;  /* intentionally unused */
}
