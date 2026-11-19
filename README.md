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

## Design Explanation (250 words)

The custom allocator manages memory obtained from the operating system via `sbrk()` in increments of at least 64 KB (`MIN_CHUNK`). All managed memory is tracked in a singly linked list ordered strictly by ascending memory addresses.

Each block is preceded by a header structure (`block_t`) containing the payload `size` (`size_t`), a pointer to the `next` block (`struct block *`), an allocation status flag (`free`), and a verification `magic` number (`0x12345678`). On 64-bit systems, the `block_t` struct occupies 24 bytes, which is rounded up to 32 bytes (`HDR`) via `align_up` to preserve 16-byte alignment. User pointers point directly to the payload immediately following `HDR`; rounding all requested payload sizes to multiples of 16 ensures every subsequent block and payload remains 16-byte aligned.

When allocating via `my_malloc()`, the list is traversed according to the selected policy (First-Fit or Best-Fit). If a suitable free block is found and the leftover space can accommodate an aligned header plus a 16-byte minimum payload, `split()` partitions the block, returning the first segment to the caller and inserting the remainder into the free list. If no candidate block exists, `grow()` requests additional heap space.

When releasing memory via `my_free()`, the allocator validates the block's magic number and guards against double-free errors. Coalescing is performed by scanning the list and merging adjacent free blocks. Crucially, two blocks are merged only if they are physically contiguous in memory—specifically verifying that `(char *)payload(b) + b->size == (char *)b->next`—which prevents corrupting the heap across memory gaps caused by external memory allocations.

---

## Questions

### 1. Why must the header size be a multiple of 16 as well as the payload sizes?
The header is positioned directly in front of the payload in memory. Assuming the block itself starts at an aligned address, adding a header whose size is a multiple of 16 guarantees that the payload address (`(char *)b + HDR`) is also 16-byte aligned. Rounding payload sizes to multiples of 16 ensures that the next block header and its corresponding payload will likewise start on 16-byte aligned boundaries.

### 2. Why can `printf()` inside `my_malloc()` cause infinite recursion?
Standard I/O functions such as `printf()` frequently allocate internal memory buffers dynamically via `malloc()`. If standard `malloc()` calls are mapped or linked to `my_malloc()`, invoking `printf()` inside `my_malloc()` triggers another call to `my_malloc()`, establishing an infinite recursive loop that quickly exhausts stack space and terminates the program with a segmentation fault.

### 3. Your `my_free()` is O(n). Which extra field would let you find a block's previous neighbor in O(1), and what is the name of the technique?
Adding a `prev` pointer to each block header would allow the allocator to access the previous block in the linked list in constant time, O(1), without traversing the list. This creates a doubly-linked list. However, the previous list node is not necessarily physically adjacent in memory, so the allocator must still check physical addresses before coalescing.

### 4. Explain the difference between internal and external fragmentation, and name one place in your allocator where each one comes from.
Internal fragmentation refers to allocated memory within a block that remains unused by the program. In this allocator, it arises when an allocation request is rounded up to the nearest multiple of 16 bytes (e.g., requesting 5 bytes allocates 16 bytes of payload). External fragmentation occurs when total free memory across the heap is sufficient to satisfy a request, but it is divided into non-contiguous fragments too small individually to satisfy the allocation. It arises after blocks of varying sizes are allocated, freed, and interleaved across the heap.

### 5. Why does `sbrk()` memory never get returned to the OS in your allocator? Describe how you could give back memory at the end of the heap.
Our allocator only increases the program break using positive offsets with `sbrk()` and never calls `brk()` to decrease it. To return memory to the OS, the allocator could inspect whether the last block in the linked list (`tail`) is free and extends to the current program break (`(char *)payload(tail) + tail->size == sbrk(0)`). If so, it could call `brk((void *)tail)` to set the break back to the start of the tail block's header, releasing both the header and payload back to the operating system.

### 6. Two blocks are consecutive in your list but not adjacent in memory. How did this happen, and what does your coalescing code do about it?
This can happen when another part of the program or a library obtains memory between separate `sbrk()` calls, potentially moving the program break and creating a gap between the allocator's memory regions. Our coalescing code explicitly verifies physical memory adjacency by checking `(char *)payload(b) + b->size == (char *)b->next`. If the physical end address of the current block does not match the start address of the next block, the merger is skipped.

---

## Known Limitations

- **Single Global Heap & No Thread Safety:** The allocator uses global `head` and `tail` pointers without synchronization primitives (such as mutexes), making it unsafe for concurrent multi-threaded environments.
- **Linear Search Overhead:** Both First-Fit and Best-Fit traverse a singly linked list in $O(n)$ time, which scales poorly under large numbers of live allocations compared to segregated bin or tree-based allocators.
- **No Dynamic Heap Contraction:** Memory remains in the allocator's free list indefinitely and is never released back to the OS via negative `sbrk()` or `brk()`.

---

## Sources & AI-Tool Declaration

- **References & Documentation:**
  - Linux Programmer's Manual / POSIX man pages (`sbrk(2)`, `brk(2)`, `write(2)`).
  - Bryant & O'Hallaron, *Computer Systems: A Programmer's Perspective* (CS:APP), Chapter 9: Dynamic Memory Allocation.
- **AI Tools:**
  - **Antigravity:**  -Helped with drafting of the README.md file as well as guided with conceptual explanations and documentation overview.
