#pragma once

#include <atomic>
#include <cstddef>
#include <iostream>
#include <new>
#include <utility>

#if defined(__cpp_lib_hardware_interference_size)
using std::hardware_destructive_interference_size;
#else
// Safe standard fallback for x86 and ARM64
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

/**
 * @brief A lock-free Single Producer Single Consumer (SPSC) queue
 * implementation.
 *
 * This queue is designed for high-performance scenarios where one thread
 * produces data and another consumes it. It uses atomic operations with
 * appropriate memory orderings to ensure thread safety without locks.
 *
 * @tparam T The type of elements stored in the queue.
 * @tparam N The capacity of the queue. For optimal performance, N should be a
 * power of 2.
 */
template <typename T, size_t N>
struct SPSCQueue {
    static_assert(N >= 2, "N must be at least 2 for the queue to function correctly");
    static_assert((N & (N - 1)) == 0, "N must be a power of 2 for optimal performance");
    static_assert(std::atomic<std::size_t>::is_always_lock_free,
                  "std::atomic<std::size_t> must be lock-free for optimal performance");

  public:
    SPSCQueue() noexcept = default;

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    /**
     * @brief Attempts to push an element into the queue.
     *
     * @param value The value to push.
     * @return true if the push was successful, false if the queue is full.
     */
    [[nodiscard]] bool push(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        size_t tail = tail_.load(std::memory_order_relaxed); ///< atomic read/write with no ordering constraints
        size_t next = increment(tail);

        if (next == head_.load(std::memory_order_acquire)) { ///< all stores before this load are visible to any
                                                             ///< thread/process that aquires this location
            return false;                                    // full
        }

        buffer_[tail] = value;

        tail_.store(next, std::memory_order_release); ///< all loads after this one see the effects of all stores that
                                                      ///< release this location
        return true;
    }

    /**
     * @brief Attempts to pop an element from the queue.
     *
     * @param value Reference to store the popped value.
     * @return true if the pop was successful, false if the queue is empty.
     */
    [[nodiscard]] bool pop(T& value) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        size_t head = head_.load(std::memory_order_relaxed);

        if (head == tail_.load(std::memory_order_acquire)) {
            return false; // empty
        }

        value = buffer_[head];

        head_.store(increment(head), std::memory_order_release);
        return true;
    }

  private:
    [[nodiscard]] size_t increment(size_t index) const noexcept {
        return (index + 1) & (N - 1);
    }
    T buffer_[N]; ///< Circular buffer to store elements.

    alignas(hardware_destructive_interference_size) std::atomic<size_t> head_{
        0}; ///< Consumer read index, cache-aligned.
    alignas(hardware_destructive_interference_size) std::atomic<size_t> tail_{
        0}; ///< Producer write index, cache-aligned.
};
