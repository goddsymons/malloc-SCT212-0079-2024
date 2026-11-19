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

The results for the benchmark showed that First-Fit took 0.245507 seconds to finish the workload, while Best-Fit took 0.272813 seconds. First-Fit was thus 10% faster in this test. The difference can be attributed to the fact that First-Fit stops searching as soon as it has encountered the first free block that can satisfy the request, where as Best-Fit searches the whole set of free blocks for the smallest one to satisfy the request.

Both strategies left the final heap size, free memory and fragmentation value the same. The zero-valued fragmentation indicates that when the other allocations had been released and the free segments coalesced together, all the free memory was in one big block.

This indicates that First-Fit was faster on the test workload. The benchmark is somewhat misleading, because both strategies used in the same process and used the same heap state of the allocator. Running them in separate processes with a clean heap would be more representative.