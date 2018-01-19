#ifndef TRINITY_SHARED_SPINLOCK_HPP
#define TRINITY_SHARED_SPINLOCK_HPP

#include <atomic>

namespace Trinity {

class SpinLock final
{
    std::atomic_flag flag_;

public:
#if defined(__clang__) || defined(__GNUC__)
    SpinLock()
        : flag_{ATOMIC_FLAG_INIT}
    { }
#else
    SpinLock()
    {
        flag_.clear();
    }
#endif

    void lock()
    {
        while (flag_.test_and_set(std::memory_order_acquire))
            ;
    }

    bool try_lock()
    {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

    void unlock()
    {
        flag_.clear(std::memory_order_release);
    }
};

} // namespace Trinity

#endif // TRINITY_SHARED_SPINLOCK_HPP
