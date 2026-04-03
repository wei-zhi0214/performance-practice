/**
 * Copyright (c) 2015 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "./allocator_interface.h"
#include "./memlib.h"

// Don't call libc malloc!
#define malloc(...) (USE_MY_MALLOC)
#define free(...) (USE_MY_FREE)
#define realloc(...) (USE_MY_REALLOC)

#define chunk_size (1 << 16) 

typedef struct FreeNode {
    struct FreeNode* next;
} FreeNode;

FreeNode* bin[13]; // 13 bins for sizes 32, 64, 128, ..., 131072

// All blocks must have a specified minimum alignment.
// The alignment requirement (from config.h) is >= 8 bytes.
#ifndef ALIGNMENT
  #define ALIGNMENT 8
#endif

// Rounds up to the nearest multiple of ALIGNMENT.
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

// The smallest aligned size that will hold a size_t value.
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// check - This checks our invariant that the size_t header before every
// block points to either the beginning of the next block, or the end of the
// heap.
int my_check() {
  char* p;
  char* lo = (char*)mem_heap_lo();
  char* hi = (char*)mem_heap_hi() + 1;
  size_t size = 0;

  p = lo;
  while (lo <= p && p < hi) {
    size = ALIGN(*(size_t*)p + SIZE_T_SIZE);
    p += size;
  }

  if (p != hi) {
    printf("Bad headers did not end at heap_hi!\n");
    printf("heap_lo: %p, heap_hi: %p, size: %lu, p: %p\n", lo, hi, size, p);
    return -1;
  }

  return 0;
}

// init - Initialize the malloc package.  Called once before any other
// calls are made.  Since this is a very simple implementation, we just
// return success.
int my_init() {
  for (int i = 0; i < 13; i++) {
    bin[i] = NULL;
  }
  return 0;
}

static inline int size_to_bin(size_t size) {
    if (size <= 32) return 0;
    // 找最小的 n 使得 32 << n >= size
    // 等價於 ceil(log2(size)) - 5
    int msb = 64 - __builtin_clzll(size - 1);  // ceil(log2(size))
    int b = msb - 5;
    if (b < 0) b = 0;
    if (b > 12) b = 12;
    return b;
}

//  malloc - Allocate a block using bin free lists with size header.
void* my_malloc(size_t size) {
  if (size == 0) return NULL;

  // block size = header + data, rounded up to bin size
  size_t needed = ALIGN(size + SIZE_T_SIZE);
  int req_order = size_to_bin(needed);
  size_t block_size = (size_t)32 << req_order;  // actual bin size

  if (bin[req_order] != NULL) {
    // Reuse a free block; header is at the block start
    char* block = (char*)bin[req_order];
    bin[req_order] = bin[req_order]->next;
    *(size_t*)block = block_size;
    return block + SIZE_T_SIZE;
  }

  // Grow heap: carve chunk into blocks of block_size
  int num_blocks = (chunk_size + block_size - 1) / block_size;
  size_t req_chunk_size = num_blocks * block_size;

  void* p = mem_sbrk(req_chunk_size);
  if (p == (void*)-1) return NULL;

  char* blk = (char*)p;
  for (int i = 0; i < num_blocks; i++) {
    FreeNode* node = (FreeNode*)blk;
    node->next = bin[req_order];
    bin[req_order] = node;
    blk += block_size;
  }

  // Pop one block off and return it
  char* block = (char*)bin[req_order];
  bin[req_order] = bin[req_order]->next;
  *(size_t*)block = block_size;
  return block + SIZE_T_SIZE;
}

// free - Return block to its bin using the header to find bin index.
void my_free(void* ptr) {
  if (ptr == NULL) return;

  // Header is SIZE_T_SIZE bytes before the user pointer
  char* block = (char*)ptr - SIZE_T_SIZE;
  size_t block_size = *(size_t*)block;
  int order = size_to_bin(block_size);

  FreeNode* node = (FreeNode*)block;
  node->next = bin[order];
  bin[order] = node;
}

// realloc - Implemented simply in terms of malloc and free
void* my_realloc(void* ptr, size_t size) {
  void* newptr;
  size_t copy_size;

  // Allocate a new chunk of memory, and fail if that allocation fails.
  newptr = my_malloc(size);
  if (NULL == newptr) {
    return NULL;
  }

  // Get the size of the old block of memory.  Take a peek at my_malloc(),
  // where we stashed this in the SIZE_T_SIZE bytes directly before the
  // address we returned.  Now we can back up by that many bytes and read
  // the size.
  copy_size = *(size_t*)((uint8_t*)ptr - SIZE_T_SIZE);

  // If the new block is smaller than the old one, we have to stop copying
  // early so that we don't write off the end of the new block of memory.
  if (size < copy_size) {
    copy_size = size;
  }

  // This is a standard library call that performs a simple memory copy.
  memcpy(newptr, ptr, copy_size);

  // Release the old block.
  my_free(ptr);

  // Return a pointer to the new block.
  return newptr;
}
