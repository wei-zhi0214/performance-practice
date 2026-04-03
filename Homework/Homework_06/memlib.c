/*
 * memlib.c - A simulated heap using a large static buffer.
 *
 * The heap is backed by a 20 MB static array. mem_sbrk() grows the heap
 * and returns a pointer to the newly allocated region (like the real sbrk).
 */

#include "memlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 20 MB heap */
#define MAX_HEAP (20 * (1 << 20))

/* The simulated heap storage */
static char   heap_buf[MAX_HEAP];
static char  *heap_lo  = NULL;   /* pointer to first byte of heap */
static char  *heap_hi  = NULL;   /* pointer to last byte of heap (NULL when empty) */
static char  *brk_ptr  = NULL;   /* current break (one past last allocated) */
static size_t heap_size = 0;     /* current heap size in bytes */

/*
 * mem_init - initialize the memory system model
 */
void mem_init(void) {
    heap_lo   = heap_buf;
    brk_ptr   = heap_buf;
    heap_hi   = NULL;   /* empty heap */
    heap_size = 0;
}

/*
 * mem_deinit - free any resources used by the memory system model
 */
void mem_deinit(void) {
    /* nothing to free for a static buffer */
}

/*
 * mem_reset_brk - reset the simulated brk pointer to the start of the heap
 */
void mem_reset_brk(void) {
    brk_ptr   = heap_lo;
    heap_hi   = NULL;   /* empty heap */
    heap_size = 0;
}

/*
 * mem_sbrk - simple model of the sbrk function.
 *
 *   Extends the heap by incr bytes and returns a pointer to the start of
 *   the new area. In keeping with the usual sbrk semantics, the returned
 *   pointer is the OLD brk value (i.e., the start of the newly allocated
 *   region). Returns (void *)-1 on error.
 */
void *mem_sbrk(int incr) {
    char *old_brk = brk_ptr;

    if (incr < 0) {
        fprintf(stderr, "mem_sbrk: negative increment not supported\n");
        return (void *)-1;
    }

    if ((brk_ptr + incr) > (heap_lo + MAX_HEAP)) {
        fprintf(stderr, "mem_sbrk: heap overflow (requested %d bytes, "
                        "only %zu available)\n",
                incr, (size_t)(MAX_HEAP - heap_size));
        return (void *)-1;
    }

    brk_ptr   += incr;
    heap_hi    = (incr > 0) ? brk_ptr - 1 : heap_hi;
    heap_size += (size_t)incr;
    return (void *)old_brk;
}

/*
 * mem_heap_lo - return pointer to the first byte of the heap
 */
void *mem_heap_lo(void) {
    return (void *)heap_lo;
}

/*
 * mem_heap_hi - return pointer to the last byte of the heap
 */
void *mem_heap_hi(void) {
    return (void *)heap_hi;
}

/*
 * mem_heapsize - returns the current heap size in bytes
 */
size_t mem_heapsize(void) {
    return heap_size;
}

/*
 * mem_pagesize - returns the system page size (simulated as 4096)
 */
size_t mem_pagesize(void) {
    return 4096;
}
