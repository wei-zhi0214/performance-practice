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
 #include <cilk/cilk.h>

// TODO (Sec 3.3): cilk/reducer.h is Intel Cilk Plus only -- not in OpenCilk 2.1
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
  if (list2->head == NULL) return;
  if (list1->tail == NULL) {
    list1->head = list2->head;
  } else {
    list1->tail->next = list2->head;
  }
  list1->tail  = list2->tail;
  list1->size += list2->size;
  list2->head = list2->tail = NULL;
  list2->size = 0;
}

// ---------------------------------------------------------------------------
// Serial solver (no cilk_spawn) -- used as coarsening base case.
// ---------------------------------------------------------------------------
static void queens_serial(uint8_t down, uint8_t left, uint8_t right,
                          Board board, int row, BoardList* board_list) {
  if (down == FULL) {
    add_board(board_list, board);
    return;
  }
  uint8_t poss = ~(down | left | right) & FULL;
  while (poss) {
    uint8_t place = poss & (uint8_t)(-(int8_t)poss);
    int     col   = __builtin_ctz(place);
    queens_serial(down  | place,
                  (uint8_t)((left  | place) << 1) & FULL,
                  (uint8_t)((right | place) >> 1),
                  board | ((Board)1 << (row * N + col)),
                  row + 1,
                  board_list);
    poss &= ~place;
  }
}

// ---------------------------------------------------------------------------
// Core solver -- Write-up 1 explains the algorithm.
// Coarsening: spawn only for the first COARSEN_DEPTH rows; below that use
// queens_serial to avoid spawn overhead on fine-grained subtasks.
// ---------------------------------------------------------------------------
#define COARSEN_DEPTH 2

void queens(uint8_t down, uint8_t left, uint8_t right,
            Board board, int row, BoardList* board_list) {
  if (down == FULL) {
    add_board(board_list, board);
    return;
  }

  // Base case for coarsening: below threshold, run serially.
  if (row >= COARSEN_DEPTH) {
    queens_serial(down, left, right, board, row, board_list);
    return;
  }

  // Candidate columns: not attacked by any placed queen.
  uint8_t poss = ~(down | left | right) & FULL;
  BoardList temp_lists[N];
  int n = 0;
  while (poss) {
    uint8_t place = poss & (uint8_t)(-(int8_t)poss);   // isolate LSB
    int     col   = __builtin_ctz(place);

    temp_lists[n] = (BoardList){ .head = NULL, .tail = NULL, .size = 0 };
    cilk_spawn queens(down  | place,
           (uint8_t)((left  | place) << 1) & FULL,
           (uint8_t)((right | place) >> 1),
           board | ((Board)1 << (row * N + col)),
           row + 1,
           &temp_lists[n]);
    n++;
    poss &= ~place;
  }
  cilk_sync;
  for (int i = 0; i < n; i++) {
    merge_lists(board_list, &temp_lists[i]);
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
// Sec 3.3: Reducer implementation -- NOTE: OpenCilk 2.1 does NOT ship
// cilk/reducer.h (Intel Cilk Plus API).  The three monoid functions below
// are correct and would plug straight into CILK_C_INIT_REDUCER if the header
// were available.  On OpenCilk we keep using the Section-3.2 temp-list
// approach, which is functionally equivalent.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Reducer stubs (Sec 3.3)
// ---------------------------------------------------------------------------

// TODO (Sec 3.3, step 2): implement these three functions.

void board_list_reduce(void* key, void* left, void* right) {
   // Cast to BoardList* and call merge_lists.
   merge_lists((BoardList*)left, (BoardList*)right);

}

void board_list_identity(void* key, void* value) {
   // Set *value to the identity BoardList {NULL, NULL, 0}.
   *(BoardList*)value = (BoardList){ .head = NULL, .tail = NULL, .size = 0 };
}

void board_list_destroy(void* key, void* value) {
   // Free nodes; hint: call delete_nodes.
    delete_nodes((BoardList*)value);
}

// TODO (Sec 3.3, step 3): instantiate the reducer type
// typedef CILK_C_DECLARE_REDUCER(BoardList) BoardListReducer;

// TODO (Sec 3.3, step 4): declare a global reducer (comment in when ready)
// BoardListReducer board_reducer =
//    CILK_C_INIT_REDUCER(BoardList,
//                         board_list_reduce,
//                         board_list_identity,
//                         board_list_destroy,
//                         (BoardList){ .head = NULL, .tail = NULL, .size = 0 });

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
