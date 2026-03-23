/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
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


#include "./util.h"

// Function prototypes
void isort(data_t* begin, data_t* end);
static void sort_f_helper(data_t* A, int p, int r, data_t* temp);
static __attribute__((always_inline)) void merge_f(data_t* A, int p, int q, int r, data_t* temp);
static __attribute__((always_inline)) void copy_f(data_t* source, data_t* dest, int n);
#define THRESHOLD 32

// Public entry point: allocates temp ONCE, sorts, then frees.
// temp needs at most ceil(n/2) + 1 elements (smaller half + sentinel).
void sort_f(data_t* A, int p, int r) {
  assert(A);
  if (p >= r) return;
  int n = r - p + 1;
  data_t* temp = NULL;
  mem_alloc(&temp, n / 2 + 1);
  if (temp == NULL) return;
  sort_f_helper(A, p, r, temp);
  mem_free(&temp);
}

// Internal recursive helper — reuses the already-allocated temp buffer.
static __attribute__((always_inline)) void sort_f_helper(data_t* A, int p, int r, data_t* temp) {
  if (p < r) {
    if (r - p < THRESHOLD) {
      isort(A + p, A + r);
      return;
    }
    int q = (p + r) / 2;
    sort_f_helper(A, p, q, temp);
    sort_f_helper(A, q + 1, r, temp);
    merge_f(A, p, q, r, temp);
  }
}

// Merge using pre-allocated temp — no malloc/free here.
// Same smaller-half strategy as sort_m.
static __attribute__((always_inline)) void merge_f(data_t* A, int p, int q, int r, data_t* temp) {
  assert(A);
  assert(p <= q);
  assert((q + 1) <= r);
  int n1 = q - p + 1;
  int n2 = r - q;

  if (n1 <= n2) {
    // Case 1: copy left half, merge forward
    copy_f(A + p, temp, n1);
    temp[n1] = UINT_MAX;

    int i = 0;
    int j = q + 1;
    for (int k = p; k <= r; k++) {
      if (j > r || temp[i] <= A[j]) {
        A[k] = temp[i++];
      } else {
        A[k] = A[j++];
      }
    }
  } else {
    // Case 2: copy right half, merge backward
    copy_f(A + q + 1, temp, n2);
    temp[n2] = UINT_MAX;

    int i = q;
    int j = n2 - 1;
    for (int k = r; k >= p; k--) {
      if (i < p || temp[j] >= A[i]) {
        A[k] = temp[j--];
      } else {
        A[k] = A[i--];
      }
    }
  }
}

static __attribute__((always_inline)) void copy_f(data_t* source, data_t* dest, int n) {
  assert(dest);
  assert(source);
  for (int i = 0; i < n; i++) {
    dest[i] = source[i];
  }
}
