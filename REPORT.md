# Memory Allocator Benchmark Report
**Course:** ICS 2305: Systems Programming  
**Author:** Nevins Symons  
**Student ID:** SCT212-0079/2024  

---

## 1. Benchmark Results

The benchmark executed a sequence of 50,000 random allocations and deallocations (using sizes between 1 and 4,096 bytes, with an array of up to 1,000 simultaneous live pointers and a fixed random seed of `42`) evaluating both First-Fit and Best-Fit allocation policies.

| Measurement | First-Fit (`FIT_FIRST`) | Best-Fit (`FIT_BEST`) |
|---|---:|---:|
| **Heap memory** | 1,376,256 bytes | 1,376,256 bytes |
| **Free memory** | 1,376,224 bytes | 1,376,224 bytes |
| **Free blocks** | 1 | 1 |
| **Largest free block** | 1,376,224 bytes | 1,376,224 bytes |
| **External Fragmentation** | 0.0000 | 0.0000 |
| **Execution Time** | 0.245507 seconds | 0.272813 seconds |

*External Fragmentation was computed as: $1 - \frac{\text{largest\_free}}{\text{free\_bytes}}$. Because all active blocks were freed and merged at the conclusion of each run, external fragmentation was 0.0000.*

---

## 2. Discussion & Policy Evaluation (~150 words)

First-Fit completed the 50,000-operation workload in 0.245507 seconds, whereas Best-Fit required 0.272813 seconds, making First-Fit roughly 10% faster. This performance difference is directly attributable to search termination: First-Fit stops as soon as it encounters the first free block large enough to satisfy the request, while Best-Fit scans the entire free list to find the candidate that minimizes leftover space.

Because both policies were executed sequentially within the same process and shared the heap state across phases, the identical memory footprint reflects the final coalescing rather than isolated fragmentation patterns. For an independent evaluation, executions in separate processes with clean heaps would be preferable.

Between the two policies, I would select **First-Fit**. In interactive systems, allocation latency is critical, and Best-Fit's exhaustive list traversal adds measurable overhead without yielding distinct space savings when block coalescing is active.

Compared to `glibc`'s production allocator, our implementation is slower because it relies on linear $O(n)$ linked-list traversals and full-list coalescing scans. In contrast, `glibc` uses segregated size classes, thread caches (`tcache`), and fastbins to achieve near $O(1)$ allocation times. Furthermore, our 32-byte aligned header introduces higher relative metadata overhead on small requests than `glibc`'s compact boundary-tag scheme.
