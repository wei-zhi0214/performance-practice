// qsort-race.c -- Quicksort with a determinacy race (Checkoff Item 4)
//
// This program was "parallelized" by naively adding cilk_spawn/cilk_sync
// to the inner loop of partition.  It has a DATA RACE.
//
// Your tasks:
//   1. Run with Cilksan (make CILKSAN=1) to detect the race.
//   2. Identify the exact read/write lines involved.
//   3. Fix the race and verify with Cilksan again.

#include <cilk/cilk.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef uint32_t data_t;

static void swap(data_t* a, data_t* b) {
  data_t t = *a;
  *a = *b;
  *b = t;
}

static int partition(data_t* arr, int lo, int hi) {
  data_t pivot = arr[hi];
  int i = lo - 1;

  for (int j = lo; j <= hi - 1; j++) {
    if (arr[j] <= pivot) {
      i++;
      swap(&arr[i], &arr[j]);
    }
  }

  swap(&arr[i + 1], &arr[hi]);
  return i + 1;
}

void sample_qsort(data_t* arr, int lo, int hi) {
  if (lo < hi) {
    int p = partition(arr, lo, hi);
    cilk_spawn sample_qsort(arr, lo, p - 1);
    sample_qsort(arr, p + 1, hi);
    cilk_sync;
  }
}

static int verify_sorted(data_t* arr, int n) {
  for (int i = 0; i < n - 1; i++)
    if (arr[i] > arr[i + 1]) return 0;
  return 1;
}

int main(int argc, char* argv[]) {
  int n    = 10000;
  int seed = 42;
  if (argc > 1) n    = atoi(argv[1]);
  if (argc > 2) seed = atoi(argv[2]);

  srand(seed);
  data_t* arr = (data_t*)malloc(n * sizeof(data_t));
  for (int i = 0; i < n; i++)
    arr[i] = rand() % (n * 10);

  sample_qsort(arr, 0, n - 1);

  if (verify_sorted(arr, n))
    printf("Sorted correctly (%d elements)\n", n);
  else
    printf("SORT FAILED! (%d elements)\n", n);

  free(arr);
  return 0;
}
