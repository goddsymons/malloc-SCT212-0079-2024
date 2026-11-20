# Custom Memory Allocator (`myalloc`)
**Course:** ICS 2305: Systems Programming  
**Author:** Nevins Symons  
**Student ID:** SCT212-0079/2024  

---

## How to Build and Run

### Prerequisites
A 64-bit Linux environment (such as Ubuntu on WSL or native Linux) with GCC and GNU Make.

### Build and Test Commands
- **Run the test suite:**
  ```bash
  make
  ```
- **Run AddressSanitizer and UndefinedBehaviorSanitizer:**
  ```bash
  gcc -std=c11 -g -fsanitize=address,undefined -o t_asan myalloc.c test_myalloc.c
  ./t_asan
  ```
- **Run the benchmark:**
  ```bash
  make bench
  ```
- **Clean build output:**
  ```bash
  make clean
  ```

---

## Tasks Implemented

| Task | Requirement | Status |
|:---:|:---|:---:|
| **1** | Block header, 16-byte alignment helpers (`align_up`), and pointer macros | Complete  |
| **2** | Heap growth using `sbrk()` (64 KB minimum request, break alignment padding) | Complete  |
| **3** | `my_malloc()` implementation with First-Fit placement | Complete  |
| **4** | Block splitting when residual space can hold header plus 16-byte payload | Complete  |
| **5** | `my_free()` with magic number check, double-free guard, and neighbor coalescing | Complete  |
| **6** | `my_calloc()` with overflow detection and `my_realloc()` with in-place growth | Complete  |
| **7** | Best-Fit policy, `heap_stats()`, `heap_dump()`, and `heap_check()` consistency validator | Complete  |
| **8** | Benchmark suite (`bench.c`) and comparative analysis report (`REPORT.md`) | Complete  |

---

## Test Results

Running `make` executes all 29 automated unit checks in `test_myalloc.c`. The final lines of the test run output:

```text
T7 best-fit vs first-fit
  PASS  first-fit takes the first hole
  PASS  best-fit takes the tightest hole
T8 randomized stress test
  PASS  no data corruption during 20000 operations
  PASS  everything freed
  PASS  fully coalesced (few free blocks remain)
  PASS  heap consistent

Result: 29 passed, 0 failed
```

---

## Design Explanation 

The custom allocator acquires memory from the OS via `sbrk()` in increments of at least 64 KB (`MIN_CHUNK`). All acquired memory is managed and linked in a singly linked list in the order of ascending memory addresses.

Before each managed memory block is a header (`block_t`) containing the size of the payload (`size_t`), next block (`struct block *`) pointer, allocation status (`free`), and a verification magic number (`0x12345678`). On a 64-bit architecture, `block_t` occupies 24 bytes which is rounded up to 32 bytes (`HDR`) via `align_up()` to preserve 16-byte alignment. User pointers are set to payloads immediately following `HDR`. All payload sizes are rounded up to 16 bytes. This ensures that all subsequent blocks are 16 bytes aligned.

When `my_malloc()` is called, the list is scanned according to the selected policy (First-Fit/Best-Fit) and a block is found that has a free status and is big enough to have an aligned header and a 16-byte payload. If such a block exists, the block is partitioned (`split()`) and the first segment returned to the caller and the remainder is inserted to the free list. If no such block is found, more heap space is requested (`grow()`).

When memory is released via `my_free()`, the block's magic number is checked and a double free is prevented. Merging of free blocks (coalescing) is done by scanning the list. Heap corruption is prevented by ensuring that two contiguous free blocks are physically aligned in memory before merging.

---

## Questions

### 1. Why must the header size be a multiple of 16 as well as the payload sizes?
The header is always located directly in front of the payload in memory. Assuming the block itself starts at an aligned address, adding a header whose size is a multiple of 16, ensures that the payload address (char *)b + HDR) is 16 byte aligned. Rounding payload sizes to multiples of 16, ensures that the following block header and its corresponding payload will also be 16 byte aligned.

### 2. Why can `printf()` inside `my_malloc()` cause infinite recursion?
Standard I/O functions such as `printf()` frequently allocate internal memory buffers dynamically via `malloc()`. If standard `malloc()` calls are linked to `my_malloc()`, invoking `printf()` inside `my_malloc()` triggers another call to `my_malloc()`, causing an infinite recursive loop that quickly exhausts stack space and terminates the program with a segmentation fault.

### 3. Your `my_free()` is O(n). Which extra field would let you find a block's previous neighbor in O(1), and what is the name of the technique?
Adding a `prev` pointer to each block header makes it so the allocator can find the previous block in the linked list in constant time, O(1). This creates a doubly-linked list. Although previous block lists are not necessarily physically adjacent in memory, the allocator must still check the physical addresses in order to coalesce them.

### 4. Explain the difference between internal and external fragmentation, and name one place in your allocator where each one comes from.
Internal fragmentation occurs when memory in a block is allocated but not used by a program. This happens in this allocator when a request is rounded up to a multiple of 16 bytes (a request for 5 bytes results in the allocation of 16 bytes of payload). External fragmentation happens when the total free memory of the heap is sufficient to satisfy a request, but is divided into many small, non-contiguous fragments. This happens when the heap has blocks of varying sizes and are allocated, freed, and interleaved in the heap.

### 5. Why does `sbrk()` memory never get returned to the OS in your allocator? Describe how you could give back memory at the end of the heap.
The allocator only increases the program break using positive offsets with `sbrk()` and never calls `brk()` to decrease it. To return memory to the OS, the allocator could inspect whether the last block in the linked list (`tail`) is free and extends to the current program break (`(char *)payload(tail) + tail->size == sbrk(0)`). If so, it could call `brk((void *)tail)` to set the break back to the start of the tail block's header, releasing both the header and payload back to the operating system.

### 6. Two blocks are consecutive in your list but not adjacent in memory. How did this happen, and what does your coalescing code do about it?
This can happen when another part of the program or a library obtains memory between separate `sbrk()` calls, potentially moving the program break and creating a gap between the allocator's memory regions. Our coalescing code explicitly verifies physical memory adjacency by checking `(char *)payload(b) + b->size == (char *)b->next`. If the physical end address of the current block does not match the start address of the next block, the merger is skipped.

---

## Known Limitations

- **Single Global Heap & No Thread Safety:** The allocator uses global `head` and `tail` pointers without synchronization primitives (such as mutexes), making it unsafe for concurrent multi-threaded environments.
- **Linear Search Overhead:** Both First-Fit and Best-Fit traverse a singly linked list in $O(n)$ time, which scales poorly under large numbers of live allocations compared to segregated bin or tree-based allocators.
- **No Dynamic Heap Contraction:** Memory remains in the allocator's free list indefinitely and is never released back to the OS via negative `sbrk()` or `brk()`.

---

## Sources Declaration

- **References & Documentation:**
## Sources & References

The following resources from the course materials were consulted for memory management, pointers, and allocator design:

- Review of Pointers and Memory Allocation (Purdue CS 252)] — Heap structure, `sbrk()`, block headers, and linked-list allocation algorithms.
- Systems Programming Book (Gustavo Rodriguez-Rivera) — Low-level dynamic memory management in C.
- Practical System Programming with C (Sri Manikanta Palakollu, Apress 2021) — Virtual memory segments, the program break, and low-level system calls.
- Debugging with GDB — GDB commands for inspecting block headers and heap structures.
- Learn C Easy Way — Pointer arithmetic using `char *`, structs, and memory alignment.
- **AI Tools:**
  - **Antigravity:**  -Helped with drafting of the README.md file as well as guided with conceptual explanations and documentation overview.
