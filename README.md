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
- Bazel build system (v5.0+)
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

Sample benchmark output:

```
===========================================
     SPSC Queue Performance Benchmark      
===========================================

[1/3] Running Warmup Phase...
[2/3] Benchmarking Max Throughput (50000000 ops)...
  Result: 184.25 Million ops/sec

[3/3] Benchmarking Transit Latency (5000000 samples)...
  One-Way Transit Latency Profile:
  Mean : 24.3 ns
  p50  : 21 ns
  p90  : 32 ns
  p95  : 45 ns
  p99  : 85 ns
  Max  : 1420 ns

Benchmark Complete.
```

## Usage

Include `spsc_queue.hpp` in your project and instantiate the template:

```cpp
#include "spsc_queue.hpp"

SPSCQueue<int, 1024> queue; // Capacity must be a power of 2

// Producer thread
queue.push(42);

// Consumer thread
int value;
if (queue.pop(value)) {
    std::cout << "Received: " << value << '\n';
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