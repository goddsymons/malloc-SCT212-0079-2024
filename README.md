# Memory Allocator Benchmark Report

## Benchmark Results

I ran the custom memory allocator with the same workload on both First-Fit and Best-Fit allocators. For each of these allocators i performed a series of 50,000 allocations and deallocations (with up to 1,000 simultaneous allocations and request sizes of up to 4,096 bytes) in succession.

| Measurement | First-Fit | Best-Fit |
|---|---:|---:|
| Heap memory | 1,376,256 bytes | 1,376,256 bytes |
| Free memory | 1,376,224 bytes | 1,376,224 bytes |
| Free blocks | 1 | 1 |
| Largest free block | 1,376,224 bytes | 1,376,224 bytes |
| Fragmentation | 0.0000 | 0.0000 |
| Execution time | 0.245507 seconds | 0.272813 seconds |

---

## Discussion
---

First-Fit did the job in 0.245507 seconds as compared to Best-Fit that took 0.272813 seconds. This makes First-Fit about 10% faster in this benchmark. The reason for this is the manner in which the search is done. First-Fit halts the search when it encounters the first free block which is large enough to accommodate the request. While Best-Fit searches through the entire free list for the smallest block.

Both approaches concluded with the same heap statistic. When all other remaining allocations were deallocated, coalescing combined the free blocks into a single large free block of 1,376,224 bytes. The final external fragmentation reading was 0.0000 as all remaining free payload memory was gathered in one block.

First-Fit was the more appropriate for this workload due to the faster allocation with no clear difference in the final heap statistics. As both policies were executed sequentially within the same process and had access to the state of the heap of the allocator, the numerical results should be taken with a pinch of salt. Separate executions with a new heap would be needed to make a significant comparison.

With a linked-list search, a suitable block can be located using linear O(n) traversal. Best-Fit has to do more searching, because it considers every block in the list. These conclusions are the same as for the threshold counter: the trade-off between speed of allocation, and the type of block-finding algorithm used.

---
### Testing

---

The test suite provided was used to verify the allocator, with all eight test groups T1-T8 all passing every individual check. There were 29 tests and they checked for: allocation, memory placement, block splitting, coalescing, reallocation, heap expansion and placement, policies and random stress.

Further testing was carried out with Address Sanitizer and Undefined Behavior Sanitizer to identify memory issues and undefined behaviour.

To run the normal tests:



```
make test
```

To run the sanitizers:

```
Gcc -std=c11 -g -fsanitize=address, undefined -o tasan myalloc.c testmyalloc.c


./t_asan
```

Implementation Mistakes

In the course of implementation, special emphasis was placed on:

- Working with void data using char * for byte pointer arithmetic.
- How the header size is taken into account when calculating the boundaries of the blocks.
- Rearranging linked-list pointers and updating the tail after splitting or merging blocks.
- The ability of to split remainders in order to have a header and minimum payload.
- Only combining blocks that are contiguous in memory.
- Returns payload addresses instead of block header addresses.
- Using size_t` for memory sizes.

---

## Questions

### 1. Why must the header size be a multiple of 16 as well as the payload sizes?

The header size must be a multiple of 16 to ensure that the payload begins at a 16-byte aligned memory address. The header is located at the memory address directly before the payload so if the header is kept aligned the payload will be correctly aligned too. Payload sizes are rounded up to 16 multiples too so the following blocks stay aligned.

### 2. Why can `printf()` inside `my_malloc()` cause infinite recursion?

The printf() function can itself require memory to be obtained using the standard malloc() function. If that malloc() call ends up being redirected to our version, my_malloc(), then printf() will be invoked again, which is the usual cause of endless recursion.

### 3. Your `my_free()` is O(n). Which extra field would let you find a block's previous neighbor in O(1), and what is the name of the technique?

If the block headers would have included a prev pointer, the allocator could have reached the previous block in the list in constant O(1) time, without traversing the list. The list with an extra prev pointer is called a doubly-linked list. It makes it easier to go from one list node to the next.

### 4. Explain the difference between internal and external fragmentation, and name one place in your allocator where each one comes from.

Internal fragmentation wasted memory within an allocated block, when you round up to the next multiple of 16 bytes External fragmentation when memory is divided into smaller non-contiguous blocks of free memory, not allowing larger allocations to be made In this version of the allocator external fragmentation was minimized by making use of block coalescing.

### 5. Why does `sbrk()` memory never get returned to the OS in your allocator? Describe how you could give back memory at the end of the heap.

The allocator uses sbrk() to request more memory but it does not provide a way to return it to the OS. We could return memory by checking if the last block is free and hit the current program break, then decreasing the break using brk() to release memory at the end of the heap.

### 6. Two blocks are consecutive in your list but not adjacent in memory. How did this happen, and what does your coalescing code do about it?

This can occur if a separate sbrk() call results in noncontiguous pages of memory, perhaps due to another library having grown the break in between the calls. While the blocks are immediately contiguous in the linked list, they may not actually be contiguous in memory. The coalescing function verifies their addresses and coalesces only when they're contiguous.
