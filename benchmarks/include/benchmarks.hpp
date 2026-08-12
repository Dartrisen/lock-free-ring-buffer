#pragma once

#include "stats.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <pthread.h>

#if defined(__linux__)
#include <sched.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <sys/qos.h>
#endif

// Required header for x86 pause intrinsic
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#include <immintrin.h>
#endif

// Spin hint: x86 uses pause, Apple Silicon ARM64 (M4) uses assembly yield
inline void cpu_relax() {
#if defined(__aarch64__) || defined(_M_ARM64)
    asm volatile("yield" ::: "memory");
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#if defined(_MSC_VER)
    _mm_pause();
#else
    __builtin_ia32_pause();
#endif
#endif
}

// Pinning & Thread Scheduling
inline void set_cpu_affinity(int core_or_tag) {
    if (core_or_tag < 0)
        return;

#if defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_or_tag, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

#elif defined(__APPLE__)
    // Force Apple Silicon scheduler to use Performance (P) cores
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);

    // Assign Mach Affinity Tag to request placement on separate core clusters
    thread_affinity_policy_data_t policy = {static_cast<integer_t>(core_or_tag + 1)};
    thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());

    thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, reinterpret_cast<thread_policy_t>(&policy),
                      THREAD_AFFINITY_POLICY_COUNT);
#else
    (void)core_or_tag;
#endif
}

// Payload structure for end-to-end latency timing
struct TimestampedMessage {
    std::chrono::steady_clock::time_point send_time;
    uint64_t payload;
};

// --- Throughput Engine ---
template <typename Queue>
double run_throughput_benchmark(Queue& queue, size_t ops, int prod_core = -1, int cons_core = -1) {
    std::atomic<bool> start{false};

    std::thread consumer([&]() {
        if (cons_core >= 0)
            set_cpu_affinity(cons_core);
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        size_t consumed = 0;
        uint64_t val;
        while (consumed < ops) {
            if (queue.pop(val)) {
                ++consumed;
            } else {
                cpu_relax();
            }
        }
    });

    std::thread producer([&]() {
        if (prod_core >= 0)
            set_cpu_affinity(prod_core);
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        for (size_t i = 0; i < ops; ++i) {
            while (!queue.push(i)) {
                cpu_relax();
            }
        }
    });

    auto wall_begin = std::chrono::steady_clock::now();
    start.store(true, std::memory_order_release);

    producer.join();
    consumer.join();
    auto wall_end = std::chrono::steady_clock::now();

    double elapsed_sec = std::chrono::duration<double>(wall_end - wall_begin).count();
    return static_cast<double>(ops) / elapsed_sec; // ops per second
}

// --- Latency Engine ---
template <typename Queue>
LatencyStats run_latency_benchmark(Queue& queue, size_t samples, int prod_core = -1, int cons_core = -1) {
    std::atomic<bool> start{false};
    std::vector<uint64_t> latencies_ns(samples);

    std::thread consumer([&]() {
        if (cons_core >= 0)
            set_cpu_affinity(cons_core);
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        size_t consumed = 0;
        TimestampedMessage msg;
        while (consumed < samples) {
            if (queue.pop(msg)) {
                auto recv_time = std::chrono::steady_clock::now();
                uint64_t elapsed = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(recv_time - msg.send_time).count());
                latencies_ns[consumed] = elapsed;
                ++consumed;
            } else {
                cpu_relax();
            }
        }
    });

    std::thread producer([&]() {
        if (prod_core >= 0)
            set_cpu_affinity(prod_core);
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        for (size_t i = 0; i < samples; ++i) {
            TimestampedMessage msg{std::chrono::steady_clock::now(), i};
            while (!queue.push(msg)) {
                cpu_relax();
            }
            // Small pause delay to prevent saturating ring buffer during latency runs
            for (int delay = 0; delay < 100; ++delay) {
                cpu_relax();
            }
        }
    });

    start.store(true, std::memory_order_release);
    producer.join();
    consumer.join();

    return compute_stats(latencies_ns);
}