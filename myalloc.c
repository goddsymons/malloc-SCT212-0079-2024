
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

#define MIN_CHUNK  (64*1024)

static block_t *grow(size_t need){

    //calulates space needed foor the header and payload
    size_t chunk = HDR + need;

    if(chunk< MIN_CHUNK)
    chunk = MIN_CHUNK;

    void *brk =sbrk(0);

    if (brk ==(void *)-1)
    return NULL;

     //adds padding if the current break is not 16-byte aligned 
    size_t pad = 0;
    uintptr_t addr = (uintptr_t)brk;

    if (addr % ALIGNMENT != 0)
        pad = ALIGNMENT - (addr % ALIGNMENT);

    //Requests the memory from the operating system 
    void *p = sbrk(pad + chunk);

    if (p == (void *)-1)
        return NULL;

    // Place the block after any required alignment padding */
    block_t *b = (block_t *)((char *)p + pad);

    b->size = chunk - HDR;
    b->next = NULL;
    b->free = 1;
    b->magic = MAGIC;

    //tracks the total memory obtained
    heap_bytes_total += chunk;

    //appends the new block to the linked list 
    if (tail != NULL)
        tail->next = b;
    else
        head = b;

    tail = b;

    return b;
}

/* Minimum useful payload for a new free block */
#define MIN_PAYLOAD ALIGNMENT

/* Split a large block if the remaining space can hold a header and payload */
static void split(block_t *b, size_t need)
{
    if (b->size < need + HDR + MIN_PAYLOAD)
        return;

    // Creates the remaining free block after the requested payload
    block_t *rest = (block_t *)((char *)payload(b) + need);

    rest->size = b->size - need - HDR;
    rest->next = b->next;
    rest->free = 1;
    rest->magic = MAGIC;

    // This resizes the original block and link it to the new block
    b->size = need;
    b->next = rest;

    // Updates the tail if the original block was last
    if (tail == b)
        tail = rest;
}

//Merges neighbouring free blocks when they are physically adjacent 
static void coalesce(void)
{
    for (block_t *b = head; b != NULL && b->next != NULL; ) {
        block_t *n = b->next;

        //Confirms the blocks are next to each other in memory 
        char *b_end = (char *)payload(b) + b->size;

        if (b->free && n->free && b_end == (char *)n) {
            //Takes in the next block into the current block
            b->size += HDR + n->size;
            b->next = n->next;

            if (tail == n)
                tail = b;

            //Stays on this block to check for another merge 
        } else {
            b = b->next;
        }
    }
}


//find the first free block that is large enough to satisfy the requested size
static block_t *find_first(size_t need)
{
    for (block_t *b = head; b != NULL; b = b->next) {
        if (b->free && b->size >= need)
            return b;
    }

    return NULL;
}

// Allocate memory for the requested number of bytes
void *my_malloc(size_t size)
{
    // Zero-byte requests return NULL
    if (size == 0)
        return NULL;

    // Round the requested size up to 16-byte alignment
    size_t need = align_up(size);

    // Search for an existing free block
    block_t *b = find_first(need);

    // Requests more memory if no suitable block exists 
    if (b == NULL) {
        b = grow(need);

        if (b == NULL)
            return NULL;
    }

// Splits the block if it is larger than needed
    split(b, need);

    // Mark the selected block as allocated 
    b->free = 0;


    // Return the payload, not the block header
    return payload(b);
}

/* Release a previously allocated block */
void my_free(void *ptr)
{
    // Ignores NULL pointers
    if (ptr == NULL)
        return;

    block_t *b = header(ptr);

    // This rejects blocks with an invalid magic value
    if (b->magic != MAGIC) {
        const char msg[] = "my_free: invalid pointer\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }

    // Detects attempts to free an already free block
    if (b->free) {
        const char msg[] = "my_free: double free detected\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }

    // Marks the block as available and merges adjacent free blocks
    b->free = 1;

    coalesce();
}

