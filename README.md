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

## Discussion

First-Fit did the job in 0.245507 seconds as compared to Best-Fit that took 0.272813 seconds. This makes First-Fit about 10% faster in this benchmark. The reason for this is the manner in which the search is done. First-Fit halts the search when it encounters the first free block which is large enough to accommodate the request. While Best-Fit searches through the entire free list for the smallest block.

Both approaches concluded with the same heap statistic. When all other remaining allocations were deallocated, coalescing combined the free blocks into a single large free block of 1,376,224 bytes. The final external fragmentation reading was 0.0000 as all remaining free payload memory was gathered in one block.

First-Fit was the more appropriate for this workload due to the faster allocation with no clear difference in the final heap statistics. As both policies were executed sequentially within the same process and had access to the state of the heap of the allocator, the numerical results should be taken with a pinch of salt. Separate executions with a new heap would be needed to make a significant comparison.

With a linked-list search, a suitable block can be located using linear O(n) traversal. Best-Fit has to do more searching, because it considers every block in the list. These conclusions are the same as for the threshold counter: the trade-off between speed of allocation, and the type of block-finding algorithm used.