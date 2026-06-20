/** @addtogroup time
 *  @{
 */
/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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

#include <kfr/simd/impl/intrinsics.h>

#include <array>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <cstdio>
#include <atomic>
#include <algorithm>
#include <cinttypes>

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

template <bool fence = true>
KFR_INLINE uint64_t rdtsc() noexcept
{
    if constexpr (fence)
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
    }

#if defined(__aarch64__)
    uint64_t tsc;
    asm volatile("mrs %0, CNTVCT_EL0" : "=r"(tsc));
#elif defined(__clang__)
    uint64_t tsc = __builtin_readcyclecounter();
#else
    uint64_t tsc = __rdtsc();
#endif

    if constexpr (fence)
    {
#if defined(__x86_64__) || defined(_M_X64)
        _mm_lfence();
#elif defined(__aarch64__)
        asm volatile("isb" ::: "memory");
#else
        std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
    }
    return tsc;
}

inline std::chrono::nanoseconds steady_time()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch());
}

inline double measure_rdtsc_cycle_time()
{
#if defined(__aarch64__)
    // On AArch64 the virtual counter CNTVCT_EL0 is driven by a fixed-frequency
    // oscillator whose rate is published in CNTFRQ_EL0. No measurement loop is
    // needed — just read the register directly.
    uint64_t cntfrq;
    asm volatile("mrs %0, CNTFRQ_EL0" : "=r"(cntfrq));
    return 1e9 / static_cast<double>(cntfrq); // ns per tick
#elif defined KFR_ARCH_X86
    // On x86, busy-wait against steady_time() so the CPU stays at its full running
    // frequency throughout calibration (sleep_for causes a frequency drop on
    // wakeup that corrupts the TSC-to-wall-clock ratio).
    constexpr int count     = 10;
    constexpr auto interval = std::chrono::milliseconds(50);
    double tsc_freq         = 0;
    for (int i = 0; i < count; ++i)
    {
        // Align to a fresh steady_time() tick to avoid a partial first interval.
        std::chrono::nanoseconds os_start = steady_time();
        while (steady_time() == os_start)
            ;
        os_start           = steady_time();
        uint64_t tsc_start = rdtsc();
        std::chrono::nanoseconds os_end;
        do
        {
            os_end = steady_time();
        } while (os_end - os_start < interval);
        uint64_t tsc_duration                = rdtsc() - tsc_start;
        std::chrono::nanoseconds os_duration = os_end - os_start;
        tsc_freq += tsc_duration / static_cast<double>(os_duration.count());
    }
    tsc_freq /= count; // ticks/ns
    return 1.0 / tsc_freq;
#else
    return 0;
#endif
}

/**
 * @brief High-resolution scoped profiler using RDTSC (x86) or CNTVCT_EL0 (AArch64).
 *
 * Records a sequence of timestamped checkpoints during execution. Each call to
 * @ref record captures the elapsed cycle count since the previous checkpoint
 * (or since @ref init), subtracts the measured RDTSC overhead, and stores an
 * optional label. At most @p max_count entries are kept; further calls are
 * flagged as overflow.
 *
 * Overhead calibration is performed once at startup via @ref compute_rdtsc_overhead.
 *
 * @tparam max_count Maximum number of timestamp entries (default 64).
 * @tparam fence    If true, insert serialization fences around the hardware
 *                  counter read to reduce out-of-order measurement error.
 */
template <size_t max_count = 64, bool fence = true>
struct timestamps
{
    /** @brief Measure and return the RDTSC read overhead in cycles.
     *
     * Takes 50 back-to-back RDTSC readings, sorts them, and returns the
     * 25th-percentile value. This value is subtracted from every recorded
     * duration so that short measurements are not dominated by the cost of
     * reading the counter itself.
     */
    static uint64_t compute_rdtsc_overhead()
    {
        uint64_t result = 0;
#ifdef KFR_RECORD_TIMESTAMPS
        std::array<uint64_t, 50> overhead_samples{};

        // compute rdtsc_overhead as the average of several back-to-back rdtsc calls, to improve accuracy of
        // short measurements
        for (size_t i = 0; i < overhead_samples.size(); ++i)
        {
            uint64_t start      = rdtsc<fence>();
            uint64_t dur        = rdtsc<fence>() - start;
            overhead_samples[i] = dur;
        }
        std::sort(overhead_samples.begin(), overhead_samples.end());
        // 25th percentile
        result = overhead_samples[overhead_samples.size() / 4];
#endif
        return result;
    }
    static inline uint64_t rdtsc_overhead = compute_rdtsc_overhead();

    uint64_t last = 0;
    uint64_t dur[max_count]{};
    const char* msg[max_count]{};
    size_t count = 0;

    /** @brief Reset the profiler, capturing the current cycle count as time zero. */
    KFR_INTRINSIC void init() noexcept
    {
        last  = rdtsc<fence>();
        count = 0;
    }

    /** @brief Return a new timestamps object containing the per-slot minimum durations.
     *
     * For each recorded slot, the smaller duration (and its associated label) is
     * selected from this and @p other. If the two objects have different counts,
     * the one with fewer entries is returned as-is.
     */
    timestamps min(const timestamps& other) const noexcept
    {
        if (this->count != other.count)
            return other.count < this->count ? *this : other;
        timestamps result;
        result.count = count;
        for (size_t i = 0; i < result.count; ++i)
        {
            if (dur[i] < other.dur[i])
            {
                result.dur[i] = dur[i];
                result.msg[i] = msg[i];
            }
            else
            {
                result.dur[i] = other.dur[i];
                result.msg[i] = other.msg[i];
            }
        }
        return result;
    }

    /** @brief Record a timestamp checkpoint with an optional label.
     *
     * Computes the elapsed cycles since the last checkpoint (or @ref init),
     * subtracts the pre-calibrated RDTSC overhead, and stores the result.
     *
     * @param msg Short description stored alongside this checkpoint.
     * @return true if the entry was recorded; false if max_count was exceeded.
     */
    KFR_INTRINSIC bool record(const char* msg) noexcept
    {
#ifdef KFR_RECORD_TIMESTAMPS
        if (count < max_count)
        {
            uint64_t now      = rdtsc<fence>();
            uint64_t duration = now - last;
            this->dur[count]  = duration > rdtsc_overhead ? duration - rdtsc_overhead : 0;
            last              = now;
            this->msg[count]  = msg;
            ++count;
            return true;
        }
        else
        {
            this->msg[max_count - 1] = "overflow";
            return false;
        }
#else
        (void)msg;
        return false;
#endif
    }
};

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
