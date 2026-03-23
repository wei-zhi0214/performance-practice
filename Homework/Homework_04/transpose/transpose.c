// transpose.c -- In-place matrix transpose
// Checkoff Item 3: replace for loops with cilk_for to parallelize

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cilk/cilk.h>

// Transpose n x n matrix A in-place.
void transpose(int* A, int n) {
  cilk_for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      int tmp       = A[i * n + j];
      A[i * n + j]  = A[j * n + i];
      A[j * n + i]  = tmp;
    }
  }
}

int main(int argc, char* argv[]) {
  int n = 1000;
  if (argc > 1) n = atoi(argv[1]);

  int* A = (int*)malloc((size_t)n * n * sizeof(int));
  if (!A) { perror("malloc"); return 1; }

  // Initialize
  for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
      A[i * n + j] = i * n + j;

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  transpose(A, n);

  clock_gettime(CLOCK_MONOTONIC, &end);
  double elapsed = (end.tv_sec - start.tv_sec) +
                   (end.tv_nsec - start.tv_nsec) / 1e9;

  // Verify: A[i][j] should now be j*n+i
  int ok = 1;
  for (int i = 0; i < n && ok; i++)
    for (int j = 0; j < n && ok; j++)
      if (A[i * n + j] != j * n + i) ok = 0;

  printf("Transpose %dx%d: %.4f seconds -- %s\n",
         n, n, elapsed, ok ? "CORRECT" : "WRONG");

  free(A);
  return 0;
}
