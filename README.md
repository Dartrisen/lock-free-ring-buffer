# Lock-Free Ring Buffer

A high-performance, lock-free Single Producer Single Consumer (SPSC) ring buffer implementation in C++.

## Overview

This project provides a template-based SPSC queue optimized for low-latency, high-throughput scenarios. It uses atomic operations with appropriate memory orderings to ensure thread safety without locks, making it suitable for real-time systems, financial trading platforms, and other performance-critical applications.

Key features:
- Lock-free design for SPSC use cases
- Cache-aligned atomic indices to prevent false sharing
- Template-based for type safety and efficiency
- Comprehensive benchmarking with latency percentiles

## Requirements

- C++17 or later
- Bazel build system
- A compiler supporting `<atomic>` (e.g., GCC, Clang)

## Building

To build the benchmark:

```bash
bazel build //:spsc_benchmark
```

## Running the Benchmark

Run the benchmark to measure throughput and latency:

```bash
bazel run //:spsc_benchmark
```

Example output:

```
Single-producer/single-consumer benchmark
Queue entries = 65535
Iterations    = 1000000 push + 1000000 pop
Elapsed time  = 0.097074 s
Throughput    = 20602857 ops/s

Push latencies (ns):
  mean = 45.4 ns
  p50  = 40 ns
  p95  = 79 ns
  p99  = 80 ns
  max  = 44297 ns

Pop latencies (ns):
  mean = 47.1 ns
  p50  = 40 ns
  p95  = 79 ns
  p99  = 89 ns
  max  = 42580 ns
```

## Usage

Include `SPSCQueue.hpp` in your project and instantiate the template:

```cpp
#include "SPSCQueue.hpp"

SPSCQueue<int, 1024> queue; // Capacity must be a power of 2

// Producer thread
queue.push(42);

// Consumer thread
int value;
if (queue.pop(value)) {
    // Use value
}
```

## Design Notes

- **Memory Ordering**: Uses relaxed loads for initial checks, acquire for synchronization, and release for publishing changes.
- **Capacity**: Must be a power of 2 for optimal modulo operations.
- **Thread Safety**: Designed exclusively for single producer and single consumer. Using multiple producers or consumers will lead to data races.
- **Performance**: Indices are cache-aligned to avoid false sharing between producer and consumer threads.

## License

See LICENSE file for details.