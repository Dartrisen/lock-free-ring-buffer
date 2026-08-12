#include "benchmarks.hpp"
#include "spsc_queue.hpp"
#include "stats.hpp"
#include <iomanip>
#include <iostream>

static constexpr size_t kQueueSize = 1 << 16;
static constexpr size_t kThroughputOps = 50'000'000;
static constexpr size_t kLatencySamples = 5'000'000;

int main() {
    std::cout << "===========================================\n";
    std::cout << "     SPSC Queue Performance Benchmark      \n";
    std::cout << "===========================================\n\n";

    // Specify CPU Cores (e.g., Core 2 and Core 3). Set to -1 to disable affinity.
    constexpr int kProducerCore = 2;
    constexpr int kConsumerCore = 3;

    // --- Warmup Phase ---
    std::cout << "[1/3] Running Warmup Phase...\n";
    {
        SPSCQueue<uint64_t, kQueueSize> warmup_queue;
        run_throughput_benchmark(warmup_queue, 1'000'000, kProducerCore, kConsumerCore);
    }

    // --- Max Throughput Benchmark ---
    std::cout << "[2/3] Benchmarking Max Throughput (" << kThroughputOps << " ops)...\n";
    {
        SPSCQueue<uint64_t, kQueueSize> tp_queue;
        double ops_per_sec = run_throughput_benchmark(tp_queue, kThroughputOps, kProducerCore, kConsumerCore);

        std::cout << "  Result: " << std::fixed << std::setprecision(2) << (ops_per_sec / 1e6)
                  << " Million ops/sec\n\n";
    }

    // --- One-Way Transit Latency Benchmark ---
    std::cout << "[3/3] Benchmarking Transit Latency (" << kLatencySamples << " samples)...\n";
    {
        SPSCQueue<TimestampedMessage, kQueueSize> lat_queue;
        LatencyStats stats = run_latency_benchmark(lat_queue, kLatencySamples, kProducerCore, kConsumerCore);

        print_stats("  One-Way Transit", stats);
    }

    std::cout << "\nBenchmark Complete.\n";
    return 0;
}