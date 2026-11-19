
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "myalloc.h"

#define N_OPS    50000
#define MAX_LIVE 1000
#define MAX_SIZE 4096
#define SEED     42

// Calculate time passed between two timespec values in seconds
static double time_diff(struct timespec *start, struct timespec *end)
{
    return (end->tv_sec - start->tv_sec) +
           (end->tv_nsec - start->tv_nsec) / 1e9;
}

//Runs a workload and measures allocator performance 
static void run_workload(const char *label)
{
    void *ptrs[MAX_LIVE] = {0};
    struct timespec t0, t1;

    srand(SEED);

    clock_gettime(CLOCK_MONOTONIC, &t0);

    //Perform random allocation and deallocation operations
    for (int i = 0; i < N_OPS; i++) {
        int idx = rand() % MAX_LIVE;

        if (ptrs[idx]) {
            my_free(ptrs[idx]);
            ptrs[idx] = NULL;
        } else {
            size_t sz = (rand() % MAX_SIZE) + 1;
            ptrs[idx] = my_malloc(sz);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);

    //Release any allocations still in use
    for (int i = 0; i < MAX_LIVE; i++) {
        if (ptrs[i])
            my_free(ptrs[i]);
    }

    //Collect final heap statistics 
    heap_stats_t s;
    heap_stats(&s);

    printf("%-10s  heap=%zu  free=%zu  blocks=%zu  largest=%zu  "
           "frag=%.4f  time=%.6fs\n",
           label, s.heap_bytes, s.free_bytes, s.free_blocks,
           s.largest_free,
           s.free_bytes ? 1.0 - (double)s.largest_free / s.free_bytes : 0.0,
           time_diff(&t0, &t1));
}

int main(void)
{
    printf("First Fit\n");
    set_policy(FIT_FIRST);
    run_workload("FIRST");

    printf("\n Best-Fit \n");
    set_policy(FIT_BEST);
    run_workload("BEST");

    return 0;
}
