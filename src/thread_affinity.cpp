#include "thread_affinity.hpp"

#include <thread>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#endif

namespace thread_affinity {

bool pin_current_thread(std::size_t logical_cpu) noexcept {
    const unsigned int logical_cpu_count = std::thread::hardware_concurrency();
    if (logical_cpu_count == 0) {
        return false;
    }
    logical_cpu %= logical_cpu_count;

#if defined(_WIN32)
    constexpr std::size_t mask_bits = sizeof(DWORD_PTR) * 8;
    if (logical_cpu >= mask_bits) {
        return false;
    }
    const DWORD_PTR mask = static_cast<DWORD_PTR>(1) << logical_cpu;
    return SetThreadAffinityMask(GetCurrentThread(), mask) != 0;
#elif defined(__linux__)
    if (logical_cpu >= CPU_SETSIZE) {
        return false;
    }
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(static_cast<int>(logical_cpu), &cpu_set);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set) == 0;
#elif defined(__APPLE__)
    thread_affinity_policy_data_t policy = {
        static_cast<integer_t>(logical_cpu + 1)
    };
    const mach_port_t thread = mach_thread_self();
    const kern_return_t result = thread_policy_set(thread, THREAD_AFFINITY_POLICY,
                                                   reinterpret_cast<thread_policy_t>(&policy),
                                                   THREAD_AFFINITY_POLICY_COUNT);
    mach_port_deallocate(mach_task_self(), thread);
    return result == KERN_SUCCESS;
#else
    (void)logical_cpu;
    return false;
#endif
}

}  // namespace thread_affinity
