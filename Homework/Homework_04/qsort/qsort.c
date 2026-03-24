// qsort.c -- Correct parallel quicksort (Checkoff Item 5)
//
// This version has NO determinacy races: the two recursive calls
// always operate on disjoint subarrays.
//
// Build with Cilkscale to measure work/span/parallelism:
//   make CILKSCALE=1
//   ./qsort <n>

#include <cilk/cilk.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#ifdef CILKSCALE
#include <cilk/cilkscale.h>
// Helper: captures the start span at program entry, printed at exit.
static wsp_t __wsp_start;
static inline void print_total(void) {
  wsp_t end = wsp_getworkspan();
  wsp_dump(wsp_sub(end, __wsp_start), "total");
}
#else
static inline void print_total(void) {}
#endif

typedef uint32_t data_t;

static void swap(data_t* a, data_t* b) {
  data_t t = *a;
  *a = *b;
  *b = t;
}

// Serial Lomuto partition -- no races possible here.
static int partition(data_t* arr, int lo, int hi) {
  data_t pivot = arr[hi];
  int i = lo - 1;
  for (int j = lo; j <= hi - 1; j++) {
    if (arr[j] <= pivot) {
      ++i;
      swap(&arr[i], &arr[j]);
    }
  }
  swap(&arr[i + 1], &arr[hi]);
  return i + 1;
}

void sample_qsort(data_t* arr, int lo, int hi) {
  if (lo < hi) {
    int p = partition(arr, lo, hi);
    // arr[lo..p-1] and arr[p+1..hi] are disjoint -- no race.
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
  int n    = 100000;
  int seed = 42;
  if (argc > 1) n    = atoi(argv[1]);
  if (argc > 2) seed = atoi(argv[2]);

#ifdef CILKSCALE
  __wsp_start = wsp_getworkspan();
#endif

  srand(seed);
  data_t* arr = (data_t*)malloc(n * sizeof(data_t));
  for (int i = 0; i < n; i++)
    arr[i] = rand() % (n * 10);

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  sample_qsort(arr, 0, n - 1);

  clock_gettime(CLOCK_MONOTONIC, &end);
  double elapsed = (end.tv_sec - start.tv_sec) +
                   (end.tv_nsec - start.tv_nsec) / 1e9;

  if (verify_sorted(arr, n))
    printf("Sorted correctly (%d elements) in %.4f seconds\n", n, elapsed);
  else
    printf("SORT FAILED!\n");

#ifdef CILKSCALE
  print_total();
#endif

  free(arr);
  return 0;
}
