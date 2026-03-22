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
static __attribute__((always_inline)) void merge_m(data_t* A, int p, int q, int r);
static __attribute__((always_inline)) void copy_m(data_t* source, data_t* dest, int n);
#define THRESHOLD 32

// A basic merge sort routine that sorts the subarray A[p..r]
void sort_m(data_t* A, int p, int r) {
  assert(A);
  if (p < r) {
    if (r - p < THRESHOLD) {
      isort(A + p, A + r);
      return;
    }
    int q = (p + r) / 2;
    sort_m(A, p, q);
    sort_m(A, q + 1, r);
    merge_m(A, p, q, r);
  }
}

// A merge routine. Merges the sub-arrays A[p..q] and A[q+1..r].
// Copies only the SMALLER half to minimize allocation.
//
// Case 1: left half smaller (n1 <= n2)
//   Copy left to temp, merge temp+A[q+1..r] forward into A[p..r].
//   Write pointer k never overtakes right read pointer j (k <= j always).
//
// Case 2: right half smaller (n2 < n1)
//   Copy right to temp, merge A[p..q]+temp backward into A[p..r].
//   Write pointer k never overtakes left read pointer i (k >= i always).
static __attribute__((always_inline)) void merge_m(data_t* A, int p, int q, int r) {
  assert(A);
  assert(p <= q);
  assert((q + 1) <= r);
  int n1 = q - p + 1;
  int n2 = r - q;
  int nsmall = (n1 <= n2) ? n1 : n2;

  data_t* temp = NULL;
  mem_alloc(&temp, nsmall + 1);
  if (temp == NULL) {
    mem_free(&temp);
    return;
  }

  if (n1 <= n2) {
    // Case 1: copy left half, merge forward
    copy_m(A + p, temp, n1);
    temp[n1] = UINT_MAX;  // sentinel

    int i = 0;      // index into temp (left half copy)
    int j = q + 1;  // index into right half in A

    for (int k = p; k <= r; k++) {
      if (j > r || temp[i] <= A[j]) {
        A[k] = temp[i++];
      } else {
        A[k] = A[j++];
      }
    }
  } else {
    // Case 2: copy right half, merge backward
    copy_m(A + q + 1, temp, n2);
    temp[n2] = UINT_MAX;  // sentinel (used as lower bound guard below)

    int i = q;      // index into left half in A (right end)
    int j = n2 - 1; // index into temp (right half copy, right end)

    for (int k = r; k >= p; k--) {
      if (i < p || temp[j] >= A[i]) {
        A[k] = temp[j--];
      } else {
        A[k] = A[i--];
      }
    }
  }

  mem_free(&temp);
}

static __attribute__((always_inline)) void copy_m(data_t* source, data_t* dest, int n) {
  assert(dest);
  assert(source);

  for (int i = 0; i < n; i++) {
    dest[i] = source[i];
  }
}
