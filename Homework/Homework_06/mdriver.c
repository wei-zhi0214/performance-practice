/*
 * mdriver.c - Trace-driven testing framework for custom memory allocators.
 *
 * Usage:
 *   mdriver [-B] [-W] [-P] [-F] [-S] [-l] [-v] [-g] [-f <tracefile>]
 *
 * Flags:
 *   -B  test simple_allocator
 *   -W  test wrapped_allocator
 *   -P  test packed_allocator
 *   -F  test fixed_aligned_allocator
 *   -S  test smart_allocator
 *   -l  also test libc malloc/free
 *   -v  verbose output
 *   -g  autograder-style output
 *   -f  specify a single trace file (may be repeated)
 *
 * Trace file format:
 *   Line 1:   <number-of-operations>
 *   Following lines (one per operation):
 *     a <id> <size>   -- allocate <size> bytes; remember pointer at slot <id>
 *     f <id>          -- free the pointer stored in slot <id>
 *
 * Metrics:
 *   Utilization U = peak_allocated / max(heap_size_at_peak, 40 KB)
 *   (clamped so U <= 1.0)
 *
 * The framework resets the simulated heap (mem_reset_brk) before each
 * allocator/trace combination and calls the allocator's init function.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#include "memlib.h"
#include "allocator_interface.h"

/* -------------------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------------- */

#define MAX_TRACE_OPS   100000
#define MAX_ID          100000
#define TRACES_DIR      "traces"
#define MIN_HEAP        (40 * 1024)   /* 40 KB denominator floor */

/* -------------------------------------------------------------------------
 * Allocator descriptor
 * ---------------------------------------------------------------------- */

typedef int   (*init_fn)(void);
typedef void *(*malloc_fn)(size_t);
typedef void  (*free_fn)(void *);

typedef struct {
    const char *name;
    init_fn     init;
    malloc_fn   malloc;
    free_fn     free;
    int         enabled;
} AllocatorDesc;

/* libc shim */
static int   libc_init(void)             { return 0; }
static void *libc_malloc_wrap(size_t sz) { return malloc(sz); }
static void  libc_free_wrap(void *p)     { free(p); }

/* Table of all allocators */
static AllocatorDesc allocators[] = {
    { "simple",        simple_init,        simple_malloc,        simple_free,        0 },
    { "wrapped",       wrapped_init,       wrapped_malloc,       wrapped_free,       0 },
    { "packed",        packed_init,        packed_malloc,        packed_free,        0 },
    { "fixed_aligned", fixed_aligned_init, fixed_aligned_malloc, fixed_aligned_free, 0 },
    { "smart",         smart_init,         smart_malloc,         smart_free,         0 },
    { "libc",          libc_init,          libc_malloc_wrap,     libc_free_wrap,     0 },
};
#define NUM_ALLOCATORS ((int)(sizeof(allocators) / sizeof(allocators[0])))

/* -------------------------------------------------------------------------
 * Trace structures
 * ---------------------------------------------------------------------- */

typedef enum { OP_ALLOC, OP_FREE } OpKind;

typedef struct {
    OpKind kind;
    int    id;
    size_t size;   /* only meaningful for OP_ALLOC */
} TraceOp;

typedef struct {
    char     path[512];
    int      num_ops;
    TraceOp *ops;
} Trace;

/* -------------------------------------------------------------------------
 * Globals
 * ---------------------------------------------------------------------- */

static int verbose   = 0;
static int autograder = 0;

/* User-supplied trace files (-f flag) */
static char *user_traces[256];
static int   num_user_traces = 0;

/* -------------------------------------------------------------------------
 * Trace loading
 * ---------------------------------------------------------------------- */

static Trace *load_trace(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Cannot open trace file: %s\n", path);
        return NULL;
    }

    Trace *t = (Trace *)calloc(1, sizeof(Trace));
    if (!t) { fclose(fp); return NULL; }
    strncpy(t->path, path, sizeof(t->path) - 1);

    if (fscanf(fp, "%d", &t->num_ops) != 1) {
        fprintf(stderr, "Bad trace header in %s\n", path);
        free(t);
        fclose(fp);
        return NULL;
    }

    t->ops = (TraceOp *)calloc((size_t)t->num_ops, sizeof(TraceOp));
    if (!t->ops) { free(t); fclose(fp); return NULL; }

    for (int i = 0; i < t->num_ops; i++) {
        char op;
        if (fscanf(fp, " %c", &op) != 1) {
            fprintf(stderr, "Unexpected EOF in trace %s at op %d\n", path, i);
            free(t->ops); free(t); fclose(fp);
            return NULL;
        }
        if (op == 'a') {
            int id; size_t sz;
            if (fscanf(fp, "%d %zu", &id, &sz) != 2) {
                fprintf(stderr, "Bad alloc op in %s\n", path);
                free(t->ops); free(t); fclose(fp);
                return NULL;
            }
            t->ops[i].kind = OP_ALLOC;
            t->ops[i].id   = id;
            t->ops[i].size = sz;
        } else if (op == 'f') {
            int id;
            if (fscanf(fp, "%d", &id) != 1) {
                fprintf(stderr, "Bad free op in %s\n", path);
                free(t->ops); free(t); fclose(fp);
                return NULL;
            }
            t->ops[i].kind = OP_FREE;
            t->ops[i].id   = id;
            t->ops[i].size = 0;
        } else {
            fprintf(stderr, "Unknown op '%c' in %s\n", op, path);
            free(t->ops); free(t); fclose(fp);
            return NULL;
        }
    }

    fclose(fp);
    return t;
}

static void free_trace(Trace *t) {
    if (!t) return;
    free(t->ops);
    free(t);
}

/* -------------------------------------------------------------------------
 * Run one allocator against one trace
 * ---------------------------------------------------------------------- */

typedef struct {
    double utilization;   /* peak_allocated / max(heap_at_peak, 40 KB) */
    int    success;       /* 1 if no errors */
    size_t num_ops;
} RunResult;

static RunResult run_trace(AllocatorDesc *alloc, Trace *trace, int is_libc) {
    RunResult result = {0.0, 0, 0};

    /* Reset simulated heap (not needed for libc, but harmless) */
    if (!is_libc) {
        mem_reset_brk();
    }

    /* Initialise the allocator */
    if (alloc->init() != 0) {
        fprintf(stderr, "[%s] init failed\n", alloc->name);
        return result;
    }

    /* Pointer table indexed by allocation id */
    void **ptrs = (void **)calloc(MAX_ID, sizeof(void *));
    if (!ptrs) {
        fprintf(stderr, "Out of memory for pointer table\n");
        return result;
    }

    size_t current_allocated = 0;
    size_t peak_allocated    = 0;
    size_t heap_at_peak      = 0;

    result.success = 1;

    for (int i = 0; i < trace->num_ops; i++) {
        TraceOp *op = &trace->ops[i];

        if (op->kind == OP_ALLOC) {
            if (op->id < 0 || op->id >= MAX_ID) {
                fprintf(stderr, "[%s] id %d out of range\n", alloc->name, op->id);
                result.success = 0;
                break;
            }
            if (ptrs[op->id] != NULL) {
                fprintf(stderr, "[%s] double alloc for id %d\n", alloc->name, op->id);
                result.success = 0;
                break;
            }

            void *p = alloc->malloc(op->size);

            /* For smart_allocator the returned pointer may be tagged;
               store the raw tagged value so free can decode it. */
            if (p == NULL && op->size > 0) {
                if (verbose) {
                    fprintf(stderr, "[%s] malloc(%zu) returned NULL at op %d\n",
                            alloc->name, op->size, i);
                }
                /* Not necessarily fatal -- just record NULL */
            }
            ptrs[op->id]        = p;
            current_allocated  += op->size;

            if (current_allocated > peak_allocated) {
                peak_allocated = current_allocated;
                heap_at_peak   = is_libc ? peak_allocated : mem_heapsize();
            }

        } else { /* OP_FREE */
            if (op->id < 0 || op->id >= MAX_ID) {
                fprintf(stderr, "[%s] free id %d out of range\n", alloc->name, op->id);
                result.success = 0;
                break;
            }
            void *p = ptrs[op->id];
            if (p == NULL) {
                /* Nothing to free (may have been a NULL malloc or already freed) */
                if (verbose) {
                    fprintf(stderr, "[%s] free of NULL at id %d (op %d) -- skipping\n",
                            alloc->name, op->id, i);
                }
            } else {
                alloc->free(p);
                /* Find the size for this id by scanning back */
                for (int j = i - 1; j >= 0; j--) {
                    if (trace->ops[j].kind == OP_ALLOC && trace->ops[j].id == op->id) {
                        if (current_allocated >= trace->ops[j].size)
                            current_allocated -= trace->ops[j].size;
                        break;
                    }
                }
                ptrs[op->id] = NULL;
            }
        }
    }

    free(ptrs);

    /* Compute utilization */
    size_t denom = (heap_at_peak > MIN_HEAP) ? heap_at_peak : MIN_HEAP;
    size_t numer = (peak_allocated > MIN_HEAP) ? peak_allocated : MIN_HEAP;
    result.utilization = (denom > 0) ? (double)numer / (double)denom : 0.0;
    if (result.utilization > 1.0) result.utilization = 1.0;
    result.num_ops = (size_t)trace->num_ops;

    return result;
}

/* -------------------------------------------------------------------------
 * Discover trace files in the traces/ directory
 * ---------------------------------------------------------------------- */

static int collect_traces(char ***out_paths) {
    DIR *dir = opendir(TRACES_DIR);
    if (!dir) {
        if (verbose)
            fprintf(stderr, "Cannot open directory '%s'\n", TRACES_DIR);
        *out_paths = NULL;
        return 0;
    }

    /* Count entries first */
    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        count++;
    }
    rewinddir(dir);

    char **paths = (char **)calloc((size_t)(count + 1), sizeof(char *));
    if (!paths) { closedir(dir); *out_paths = NULL; return 0; }

    int idx = 0;
    while ((ent = readdir(dir)) != NULL && idx < count) {
        if (ent->d_name[0] == '.') continue;
        char buf[600];
        snprintf(buf, sizeof(buf), "%s/%s", TRACES_DIR, ent->d_name);
        paths[idx++] = strdup(buf);
    }
    closedir(dir);

    *out_paths = paths;
    return idx;
}

/* -------------------------------------------------------------------------
 * Print a results table
 * ---------------------------------------------------------------------- */

static void print_result(const char *alloc_name, const char *trace_path,
                         RunResult *r, int autograder_mode) {
    const char *trace_name = strrchr(trace_path, '/');
    trace_name = trace_name ? trace_name + 1 : trace_path;
#ifdef _WIN32
    const char *trace_name2 = strrchr(trace_path, '\\');
    if (trace_name2 && trace_name2 > trace_name) trace_name = trace_name2 + 1;
#endif

    if (autograder_mode) {
        printf("%-20s %-30s util=%.1f%%  %s\n",
               alloc_name, trace_name,
               r->utilization * 100.0,
               r->success ? "OK" : "FAIL");
    } else {
        printf("  %-18s %-28s  util=%5.1f%%  ops=%-6zu  %s\n",
               alloc_name, trace_name,
               r->utilization * 100.0,
               r->num_ops,
               r->success ? "OK" : "FAIL");
    }
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(int argc, char *argv[]) {
    /* --- Parse command-line arguments --- */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-B") == 0) {
            allocators[0].enabled = 1;  /* simple */
        } else if (strcmp(argv[i], "-W") == 0) {
            allocators[1].enabled = 1;  /* wrapped */
        } else if (strcmp(argv[i], "-P") == 0) {
            allocators[2].enabled = 1;  /* packed */
        } else if (strcmp(argv[i], "-F") == 0) {
            allocators[3].enabled = 1;  /* fixed_aligned */
        } else if (strcmp(argv[i], "-S") == 0) {
            allocators[4].enabled = 1;  /* smart */
        } else if (strcmp(argv[i], "-l") == 0) {
            allocators[5].enabled = 1;  /* libc */
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-g") == 0) {
            autograder = 1;
        } else if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 < argc) {
                user_traces[num_user_traces++] = argv[++i];
            } else {
                fprintf(stderr, "-f requires a filename argument\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Unknown flag: %s\n", argv[i]);
            fprintf(stderr, "Usage: %s [-B] [-W] [-P] [-F] [-S] [-l] [-v] [-g] [-f tracefile]\n",
                    argv[0]);
            return 1;
        }
    }

    /* Default: enable all allocators if none specified */
    int any_enabled = 0;
    for (int i = 0; i < NUM_ALLOCATORS - 1; i++) /* skip libc by default */
        any_enabled |= allocators[i].enabled;
    if (!any_enabled) {
        for (int i = 0; i < NUM_ALLOCATORS - 1; i++)
            allocators[i].enabled = 1;
    }

    /* --- Collect trace files --- */
    char **auto_traces = NULL;
    int    num_auto    = 0;

    char **trace_paths;
    int    num_traces;

    if (num_user_traces > 0) {
        trace_paths = user_traces;
        num_traces  = num_user_traces;
    } else {
        num_auto   = collect_traces(&auto_traces);
        if (num_auto == 0) {
            fprintf(stderr, "No trace files found in '%s/'. "
                            "Use -f <file> to specify a trace.\n", TRACES_DIR);
            return 1;
        }
        trace_paths = auto_traces;
        num_traces  = num_auto;
    }

    /* --- Initialise simulated heap --- */
    mem_init();

    /* --- Run each enabled allocator on each trace --- */
    if (!autograder) {
        printf("\n=== MIT 6.172 Memory Allocator Driver ===\n\n");
        printf("  %-18s %-28s  %s\n",
               "Allocator", "Trace", "Result");
        printf("  %s\n", "----------------------------------------------------------------------");
    }

    int total_tests = 0;
    int total_pass  = 0;

    for (int t = 0; t < num_traces; t++) {
        Trace *trace = load_trace(trace_paths[t]);
        if (!trace) continue;

        for (int a = 0; a < NUM_ALLOCATORS; a++) {
            if (!allocators[a].enabled) continue;

            int is_libc = (a == NUM_ALLOCATORS - 1);

            RunResult r = run_trace(&allocators[a], trace, is_libc);
            print_result(allocators[a].name, trace_paths[t], &r, autograder);

            total_tests++;
            if (r.success) total_pass++;
        }

        free_trace(trace);
    }

    if (!autograder) {
        printf("\n  Passed %d / %d tests\n\n", total_pass, total_tests);
    } else {
        printf("Score: %d / %d\n", total_pass, total_tests);
    }

    /* Clean up auto-discovered trace paths */
    if (auto_traces) {
        for (int i = 0; i < num_auto; i++) free(auto_traces[i]);
        free(auto_traces);
    }

    return (total_pass == total_tests) ? 0 : 1;
}
