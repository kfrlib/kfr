/** @addtogroup time
 *  @{
 */
/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  KFR is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with KFR.

  If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
  Buying a commercial license is mandatory as soon as you develop commercial activities without
  disclosing the source code of your own applications.
  See https://www.kfrlib.com for details.
 */
#pragma once

#include "../kfr.h"

#include <cstdint>
#include <chrono>

// Platform Detection
#if defined(_WIN32)
#define NOMINMAX 1
#include <windows.h>
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#include <mach/mach_time.h>
#else
#include <time.h>
#include <unistd.h>
#endif

namespace kfr
{

KFR_INLINE uint64_t rdtsc() noexcept
{
#if defined(__x86_64__) || defined(_M_X64)
    // lfence: execution serialization — drains the out-of-order engine so that
    // all prior instructions retire before RDTSC, and RDTSC completes before
    // any subsequent instruction starts.
    _mm_lfence();
#elif defined(__aarch64__)
    // isb: instruction synchronization barrier — flushes the pipeline so that
    // all prior instructions are complete before the counter is read.
    // dmb (what atomic_thread_fence emits) only orders *memory* accesses and
    // does not prevent the CPU from speculating across it.
    asm volatile("isb" ::: "memory");
#else
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif

#if defined(__aarch64__)
    uint64_t tsc;
    asm volatile("mrs %0, CNTVCT_EL0" : "=r"(tsc));
#elif defined(__clang__)
    uint64_t tsc = __builtin_readcyclecounter();
#else
    uint64_t tsc = __rdtsc();
#endif

#if defined(__x86_64__) || defined(_M_X64)
    _mm_lfence();
#elif defined(__aarch64__)
    asm volatile("isb" ::: "memory");
#else
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
    return tsc;
}

/**
 * @brief Returns the current value of the OS's highest-resolution monotonic clock.
 * On modern hardware, these typically map to hardware instructions (RDTSC/CNTVCT_EL0)
 * via user-mode shared pages, avoiding a heavy syscall context switch.
 */
inline uint64_t clock_now() noexcept
{
#if defined(_WIN32)
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return static_cast<uint64_t>(count.QuadPart);

#elif defined(__APPLE__)
    // On Apple Silicon and modern Intel Macs, this is a direct wrapper
    // around the hardware clock frequency.
    return mach_absolute_time();

#else
    // CLOCK_MONOTONIC_RAW is preferred for benchmarking as it is not
    // subject to NTP adjustments or slewing.
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL + static_cast<uint64_t>(ts.tv_nsec);
#endif
}

/**
 * @brief Returns the number of ticks per second for the clock used in clock_now().
 */
inline uint64_t clock_frequency() noexcept
{
#if defined(_WIN32)
    static const uint64_t freq = []()
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return static_cast<uint64_t>(f.QuadPart);
    }();
    return freq;

#elif defined(__APPLE__)
    static const uint64_t freq = []()
    {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        // mach_absolute_time * (numer / denom) = nanoseconds
        // Therefore, frequency = 1,000,000,000 * (denom / numer)
        return (1'000'000'000ULL * info.denom) / info.numer;
    }();
    return freq;

#else
    // clock_gettime(CLOCK_MONOTONIC_RAW) always returns nanoseconds.
    return 1'000'000'000ULL;
#endif
}

inline double clock_elapsed(uint64_t start_time) noexcept
{
    uint64_t now = clock_now();
    return static_cast<double>(now - start_time) / static_cast<double>(clock_frequency());
}

struct stopwatch
{
    uint64_t start_time;
    uint64_t frequency;
    stopwatch() : start_time(clock_now()), frequency(clock_frequency()) {}

    double elapsed_s() const noexcept
    {
        uint64_t now = clock_now();
        return static_cast<double>(now - start_time) / static_cast<double>(frequency);
    }

    template <typename Duration>
    Duration elapsed() const noexcept
    {
        std::chrono::duration<double> elapsed_seconds(elapsed_s());
        return std::chrono::duration_cast<Duration>(elapsed_seconds);
    }

    std::chrono::nanoseconds elapsed_ns() const noexcept { return elapsed<std::chrono::nanoseconds>(); }
    std::chrono::microseconds elapsed_us() const noexcept { return elapsed<std::chrono::microseconds>(); }
    std::chrono::milliseconds elapsed_ms() const noexcept { return elapsed<std::chrono::milliseconds>(); }
};

} // namespace kfr
