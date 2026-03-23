// fib.c -- Fibonacci using Cilk parallelism
// Checkoff Item 1: parallelize with cilk_spawn / cilk_sync
// Checkoff Item 2: add coarsening (grain size) for real speedup

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cilk/cilk.h>

// Grain size for coarsening (Checkoff 2).
// If n <= GRAIN, fall back to serial fib.
#define GRAIN 20

int fib(int n) {
  if (n < 2) return n;

  int x = cilk_spawn fib(n - 1);
  int y = fib(n - 2);
  cilk_sync;
  return x + y;
}

int main(int argc, char* argv[]) {
  int n = 45;
  if (argc > 1) n = atoi(argv[1]);

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  int result = fib(n);

  clock_gettime(CLOCK_MONOTONIC, &end);
  double elapsed = (end.tv_sec - start.tv_sec) +
                   (end.tv_nsec - start.tv_nsec) / 1e9;

  printf("fib(%d) = %d\n", n, result);
  printf("Time: %.4f seconds\n", elapsed);
  return 0;
}
