#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>

#include "spsc_queue.hpp"

static constexpr size_t kQueueSize = 1 << 16; // queue capacity is kQueueSize - 1
static constexpr size_t kOperations = 1'000'000;

struct LatencyStats {
    double mean_ns;
    uint64_t p50_ns;
    uint64_t p95_ns;
    uint64_t p99_ns;
    uint64_t max_ns;
};

LatencyStats compute_stats(std::vector<uint64_t>& samples) {
    std::sort(samples.begin(), samples.end());
    uint64_t sum = 0;
    for (uint64_t value : samples) {
        sum += value;
    }

    auto percentile = [&](double pct) -> uint64_t {
        if (samples.empty()) {
            return 0;
        }
        size_t index = static_cast<size_t>(std::ceil((pct / 100.0) * samples.size())) - 1;
        if (index >= samples.size()) {
            index = samples.size() - 1;
        }
        return samples[index];
    };

    return {
        static_cast<double>(sum) / samples.size(), percentile(50.0), percentile(95.0), percentile(99.0),
        samples.empty() ? 0 : samples.back(),
    };
}

void print_stats(std::string_view label, const LatencyStats& stats) {
    std::cout << label << " latencies (ns):\n";
    std::cout << "  mean = " << std::fixed << std::setprecision(1) << stats.mean_ns << " ns\n";
    std::cout << "  p50  = " << stats.p50_ns << " ns\n";
    std::cout << "  p95  = " << stats.p95_ns << " ns\n";
    std::cout << "  p99  = " << stats.p99_ns << " ns\n";
    std::cout << "  max  = " << stats.max_ns << " ns\n";
}

int main() {
    SPSCQueue<uint64_t, kQueueSize> queue;
    std::vector<uint64_t> push_latencies;
    std::vector<uint64_t> pop_latencies;
    push_latencies.reserve(kOperations);
    pop_latencies.reserve(kOperations);

    std::atomic<bool> start{false};
    std::atomic<size_t> produced{0};
    std::atomic<size_t> consumed{0};

    std::thread consumer([&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        while (consumed.load(std::memory_order_relaxed) < kOperations) {
            uint64_t value;
            auto begin = std::chrono::steady_clock::now();
            if (queue.pop(value)) {
                auto end = std::chrono::steady_clock::now();
                pop_latencies.push_back(
                    static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count()));
                consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    std::thread producer([&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        for (size_t i = 0; i < kOperations; ++i) {
            auto begin = std::chrono::steady_clock::now();
            while (!queue.push(i)) {
                std::this_thread::yield();
            }
            auto end = std::chrono::steady_clock::now();
            push_latencies.push_back(
                static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count()));
            produced.fetch_add(1, std::memory_order_relaxed);
        }
    });

    auto wall_begin = std::chrono::steady_clock::now();
    start.store(true, std::memory_order_release);

    producer.join();
    consumer.join();
    auto wall_end = std::chrono::steady_clock::now();

    double elapsed_seconds = std::chrono::duration<double>(wall_end - wall_begin).count();
    size_t total_operations = kOperations * 2; // push + pop
    double throughput = total_operations / elapsed_seconds;

    std::cout << "Single-producer/single-consumer benchmark\n";
    std::cout << "Queue entries = " << (kQueueSize - 1) << "\n";
    std::cout << "Iterations    = " << kOperations << " push + " << kOperations << " pop\n";
    std::cout << "Elapsed time  = " << std::fixed << std::setprecision(6) << elapsed_seconds << " s\n";
    std::cout << "Throughput    = " << std::fixed << std::setprecision(0) << throughput << " ops/s\n";
    std::cout << "\n";

    const LatencyStats push_stats = compute_stats(push_latencies);
    const LatencyStats pop_stats = compute_stats(pop_latencies);

    print_stats("Push", push_stats);
    std::cout << "\n";
    print_stats("Pop", pop_stats);

    return 0;
}
