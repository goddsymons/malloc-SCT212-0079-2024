
#define _DEFAULT_SOURCE

#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "myalloc.h"

// Block header

#define MAGIC 0x12345678
#define ALIGNMENT 16

typedef struct block {
    size_t size;          // payload size in bytes
    struct block *next;   // next block in the list
    int free;             // 1 = free, 0 = allocated 
    int magic;            // used to detect invalid blocks 
} block_t;

//Header size rounded up to a multiple of 16 
#define HDR (((sizeof(block_t)) + 15) & ~(size_t)15)

//Round's a size up to the next multiple of 16
static size_t align_up(size_t n)
{
    return (n + (ALIGNMENT - 1)) & ~(size_t)(ALIGNMENT - 1);
}

//Returns the payload address of a block
static void *payload(block_t *b)
{
    return (char *)b + HDR;
}

// Returns the header address from a payload pointer 
static block_t *header(void *ptr)
{
    return (block_t *)((char *)ptr - HDR);
}



static block_t *head = NULL;
static block_t *tail = NULL;

static size_t heap_bytes_total = 0;

static fit_policy_t policy = FIT_FIRST;
