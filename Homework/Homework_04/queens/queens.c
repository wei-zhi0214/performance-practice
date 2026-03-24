// queens.c -- N-Queens solver using Cilk (MIT 6.172 HW4, Section 3)
//
// The board is 8x8 (N=8).  Bit i of a uint64_t represents square i,
// laid out row-major: square (row,col) = bit (row*N + col).
//
// The solver uses Tony Lezard's compact bit-vector trick (1991):
//   down  -- columns already occupied (N bits)
//   left  -- left-diagonal hazards shifted into current row (N bits)
//   right -- right-diagonal hazards shifted into current row (N bits)
// All three vectors fit in N bits because diagonals that exit the board
// on either side can be discarded.
//
// Write-up 1 (Bonus): explain why N bits suffice for each vector.
//
// Tasks summary:
//   Sec 3.1  -- Write-up 2,3  : add cilk_spawn/sync, observe race, Cilksan
//   Sec 3.2  -- Write-up 4,5,6: implement merge_lists, fix race manually
//   Sec 3.3  -- Write-up 7,8  : implement BoardListReducer

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// TODO (Sec 3.1): uncomment when parallelizing
// #include <cilk/cilk.h>

// TODO (Sec 3.3): uncomment when implementing reducer
// #include <cilk/reducer.h>

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

#define N    8
#define FULL ((uint8_t)((1 << N) - 1))   // 0xFF: all 8 columns occupied
#define I    10                            // timing iterations in main()

// A Board records a complete solution: bit (row*N+col) set when queen placed.
typedef uint64_t Board;

typedef struct BoardNode {
  Board            board;
  struct BoardNode* next;
} BoardNode;

typedef struct {
  BoardNode* head;
  BoardNode* tail;
  int        size;
} BoardList;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static BoardNode* new_node(Board board) {
  BoardNode* node = (BoardNode*)malloc(sizeof(BoardNode));
  node->board = board;
  node->next  = NULL;
  return node;
}

static void add_board(BoardList* list, Board board) {
  BoardNode* node = new_node(board);
  if (list->tail == NULL) {
    list->head = list->tail = node;
  } else {
    list->tail->next = node;
    list->tail       = node;
  }
  list->size++;
}

// Free every node; reset list to empty.
static void delete_nodes(BoardList* list) {
  BoardNode* curr = list->head;
  while (curr) {
    BoardNode* next = curr->next;
    free(curr);
    curr = next;
  }
  list->head = list->tail = NULL;
  list->size = 0;
}

// ---------------------------------------------------------------------------
// Write-up 4 (Sec 3.2): implement merge_lists
//   Merge list2 into list1 in O(1), then reset list2 to empty.
//   Hint: connect list1->tail to list2->head; update tail and size.
// ---------------------------------------------------------------------------
void merge_lists(BoardList* list1, BoardList* list2) {
  // TODO: implement
  (void)list1;
  (void)list2;
}

// ---------------------------------------------------------------------------
// Core solver (12 lines) -- Write-up 1 explains the algorithm.
//
//   down  : columns occupied so far
//   left  : left-diagonal hazards in current row (shifted up each row)
//   right : right-diagonal hazards in current row (shifted down each row)
//   board : accumulated board bits for this branch
//   row   : current row index (0-based)
//   board_list: output list
//
// TODO (Sec 3.1): Add cilk_spawn before the recursive call below, and
//                 cilk_sync after the while-loop.  Then observe the race
//                 with Cilksan.
//
// TODO (Sec 3.2): After removing cilk_spawn/sync, re-parallelize using
//                 temporary BoardLists merged after cilk_sync.
//
// TODO (Sec 3.3): Re-parallelize using a BoardListReducer instead.
// ---------------------------------------------------------------------------
void queens(uint8_t down, uint8_t left, uint8_t right,
            Board board, int row, BoardList* board_list) {
  if (down == FULL) {
    add_board(board_list, board);
    return;
  }

  // Candidate columns: not attacked by any placed queen.
  uint8_t poss = ~(down | left | right) & FULL;
  while (poss) {
    uint8_t place = poss & (uint8_t)(-(int8_t)poss);   // isolate LSB
    int     col   = __builtin_ctz(place);

    queens(down  | place,
           (uint8_t)((left  | place) << 1) & FULL,
           (uint8_t)((right | place) >> 1),
           board | ((Board)1 << (row * N + col)),
           row + 1,
           board_list);

    poss &= ~place;
  }
}

// ---------------------------------------------------------------------------
// run_queens: serial wrapper used for timing and Cilkscale
// ---------------------------------------------------------------------------
BoardList run_queens(void) {
  BoardList list = { .head = NULL, .tail = NULL, .size = 0 };
  queens(0, 0, 0, 0ULL, 0, &list);
  return list;
}

// ---------------------------------------------------------------------------
// Reducer stubs (Sec 3.3)
// ---------------------------------------------------------------------------

// TODO (Sec 3.3, step 2): implement these three functions.

// void board_list_reduce(void* key, void* left, void* right) {
//   // Cast to BoardList* and call merge_lists.
// }

// void board_list_identity(void* key, void* value) {
//   // Set *value to the identity BoardList {NULL, NULL, 0}.
// }

// void board_list_destroy(void* key, void* value) {
//   // Free nodes; hint: call delete_nodes.
// }

// TODO (Sec 3.3, step 3): instantiate the reducer type
// typedef CILK_C_DECLARE_REDUCER(BoardList) BoardListReducer;

// TODO (Sec 3.3, step 4): declare a global reducer (comment in when ready)
// BoardListReducer board_reducer =
//   CILK_C_INIT_REDUCER(BoardList,
//                        board_list_reduce,
//                        board_list_identity,
//                        board_list_destroy,
//                        (BoardList){ .head = NULL, .tail = NULL, .size = 0 });

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(void) {
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  BoardList result = { .head = NULL, .tail = NULL, .size = 0 };

  // NOTE (Cilksan): comment out this loop (keep only one run_queens call)
  //                 when running with CILKSAN=1 to keep runtime manageable.
  //                 Uncomment before recording performance numbers.
  for (int i = 0; i < I; i++) {
    delete_nodes(&result);
    result = run_queens();
  }

  clock_gettime(CLOCK_MONOTONIC, &end);
  double elapsed = (end.tv_sec - start.tv_sec) +
                   (end.tv_nsec - start.tv_nsec) / 1e9;

  printf("Found %d solutions\n", result.size);
  printf("Average time: %.6f seconds (over %d runs)\n", elapsed / I, I);

  delete_nodes(&result);
  return 0;
}
