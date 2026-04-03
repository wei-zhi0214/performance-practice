#ifndef ALLOCATOR_INTERFACE_H
#define ALLOCATOR_INTERFACE_H

#include <stddef.h>
#include <stdint.h>

// Cache line size
#define CACHE_LINE_SIZE 64

// Align x up to the nearest multiple of align (must be power of 2)
#define ALIGN(x, align) (((x) + (align) - 1) & ~((align) - 1))

// Align to 8 bytes
#define ALIGN8(x) ALIGN(x, 8)

// Align to cache line
#define ALIGN_CACHE(x) ALIGN(x, CACHE_LINE_SIZE)

// simple allocator
int   simple_init(void);
void *simple_malloc(size_t size);
void  simple_free(void *ptr);

// wrapped allocator
int   wrapped_init(void);
void *wrapped_malloc(size_t size);
void  wrapped_free(void *ptr);

// helper used internally by wrapped allocator
void *unaligned_malloc(size_t size);

// packed allocator
int   packed_init(void);
void *packed_malloc(size_t size);
void  packed_free(void *ptr);

// fixed aligned allocator
int   fixed_aligned_init(void);
void *fixed_aligned_malloc(size_t size);
void  fixed_aligned_free(void *ptr);

// smart allocator
int   smart_init(void);
void *smart_malloc(size_t size);
void  smart_free(void *ptr);

// SMART_PTR: given pointer returned by smart_malloc, get actual data pointer
// We store size info in the low bit of the aligned pointer:
// if low bit of original pointer is 1 => small (32-byte), else large (64-byte)
// SMART_PTR clears low bits to get the usable pointer
#define SMART_PTR(p)  ((void *)((uintptr_t)(p) & ~(uintptr_t)1))
// IS_SMALL: check if the allocation is small (32-byte)
#define IS_SMALL(p)   ((uintptr_t)(p) & 1)

#endif // ALLOCATOR_INTERFACE_H
