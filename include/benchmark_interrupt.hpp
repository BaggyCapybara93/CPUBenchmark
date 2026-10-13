#pragma once

#include <atomic>
#include <exception>

namespace benchmark_interrupt {

// A lock-free atomic is safe in a signal handler, including on a worker thread.
static_assert(std::atomic<bool>::is_always_lock_free);
inline std::atomic<bool> requested{false};

inline void request_stop(int) noexcept {
    requested.store(true, std::memory_order_relaxed);
}

class interrupted : public std::exception {
public:
    const char* what() const noexcept override { return "Stress test stopped"; }
};

inline void check() {
    if (requested.load(std::memory_order_relaxed)) {
        throw interrupted{};
    }
}

} // namespace benchmark_interrupt
