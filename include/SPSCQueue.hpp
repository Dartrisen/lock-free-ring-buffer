#include <atomic>
#include <memory>

/**
 * @brief A lock-free Single Producer Single Consumer (SPSC) queue implementation.
 *
 * This queue is designed for high-performance scenarios where one thread produces
 * data and another consumes it. It uses atomic operations with appropriate memory
 * orderings to ensure thread safety without locks.
 *
 * @tparam T The type of elements stored in the queue.
 * @tparam N The capacity of the queue. For optimal performance, N should be a power of 2.
 */
template <typename T, size_t N>
struct SPSCQueue
{
    static_assert(N > 0 && (N & (N - 1)) == 0, "N must be a power of 2 for optimal performance");

    T buffer[N]; ///< Circular buffer to store elements.

    alignas(64) std::atomic<size_t> head{0}; ///< Consumer read index, cache-aligned to avoid false sharing.
    alignas(64) std::atomic<size_t> tail{0}; ///< Producer write index, cache-aligned to avoid false sharing.

public:
    /**
     * @brief Attempts to push an element into the queue.
     *
     * @param value The value to push.
     * @return true if the push was successful, false if the queue is full.
     */
    bool push(const T &value);

    /**
     * @brief Attempts to pop an element from the queue.
     *
     * @param value Reference to store the popped value.
     * @return true if the pop was successful, false if the queue is empty.
     */
    bool pop(T &value);
};

template <typename T, size_t N>
bool SPSCQueue<T, N>::push(const T &value)
{
    size_t t = tail.load(std::memory_order_relaxed);
    size_t next = (t + 1) & (N - 1);

    if (next == head.load(std::memory_order_acquire))
    {
        return false; // full
    }

    buffer[t] = value;

    tail.store(next, std::memory_order_release);
    return true;
}

template <typename T, size_t N>
bool SPSCQueue<T, N>::pop(T &value)
{
    size_t h = head.load(std::memory_order_relaxed);

    if (h == tail.load(std::memory_order_acquire))
    {
        return false; // empty
    }

    value = buffer[h];

    head.store((h + 1) & (N - 1), std::memory_order_release);
    return true;
}
