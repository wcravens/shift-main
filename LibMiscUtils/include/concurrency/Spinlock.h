#pragma once

#include <atomic>

namespace shift::concurrency {

// See: https://en.cppreference.com/w/cpp/atomic/atomic_flag
// And: https://rigtorp.se/spinlock/
class Spinlock {
public:
    void lock() noexcept
    {
        while (m_lock.test_and_set(std::memory_order_acquire)) { // acquire lock
// Since C++20, it is possible to update atomic_flag's
// value only when there is a chance to acquire the lock.
// See also: https://stackoverflow.com/questions/62318642
#if defined(__cpp_lib_atomic_flag_test)
            while (m_lock.test(std::memory_order_relaxed)) // test lock
#endif
                ; // spin
        }
    }

    bool try_lock() noexcept
    {
        return
// First do a relaxed test to check if lock is free in order to prevent
// unnecessary cache misses if someone does while(!try_lock())
#if defined(__cpp_lib_atomic_flag_test)
            !m_lock.test(std::memory_order_relaxed) &&
#endif
            !m_lock.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept
    {
        m_lock.clear(std::memory_order_release); // release lock
    }

private:
    std::atomic_flag m_lock = ATOMIC_FLAG_INIT;
};

} // shift::concurrency
