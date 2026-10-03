#pragma once

#include "../../cident.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef KFR_ARCH_SSE2
#include <immintrin.h>
#ifdef KFR_OS_WIN
#include <intrin.h>
#endif
#endif

#ifdef KFR_ARCH_NEON
#include <arm_neon.h>
#endif

#ifdef KFR_ARCH_RVV
#include <riscv_vector.h>
#endif

#if defined KFR_COMPILER_GCC && defined KFR_ARCH_X86
#include <x86intrin.h>
#endif

#ifdef KFR_COMPILER_CLANG
#define builtin_addressof(x) __builtin_addressof(x)
#else
template <class T>
inline T* builtin_addressof(T& arg)
{
    return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(arg)));
}
#endif

#ifdef KFR_COMPILER_GNU
KFR_INLINE float builtin_sqrt(float x) { return __builtin_sqrtf(x); }
KFR_INLINE double builtin_sqrt(double x) { return __builtin_sqrt(x); }
KFR_INLINE long double builtin_sqrt(long double x) { return __builtin_sqrtl(x); }
KFR_INLINE void builtin_memcpy(void* dest, const void* src, size_t size)
{
    __builtin_memcpy(dest, src, size);
}
KFR_INLINE void builtin_memmove(void* dest, const void* src, size_t size)
{
    __builtin_memmove(dest, src, size);
}
KFR_INLINE void builtin_memset(void* dest, int val, size_t size) { __builtin_memset(dest, val, size); }
#else
KFR_INLINE float builtin_sqrt(float x) { return ::sqrtf(x); }
KFR_INLINE double builtin_sqrt(double x) { return ::sqrt(x); }
KFR_INLINE long double builtin_sqrt(long double x) { return ::sqrtl(x); }
KFR_INLINE void builtin_memcpy(void* dest, const void* src, size_t size) { ::memcpy(dest, src, size); }
KFR_INLINE void builtin_memmove(void* dest, const void* src, size_t size) { ::memmove(dest, src, size); }
KFR_INLINE void builtin_memset(void* dest, int val, size_t size) { ::memset(dest, val, size); }
#endif

#ifdef __cplusplus

namespace kfr
{
/**
 * @brief RAII guard that enables flush-to-zero (FTZ) and denormals-are-zero (DAZ)
 *        on supported CPUs (x86/x86_64, AArch64), restoring previous state on destruction.
 */
struct scoped_flush_denormals
{
public:
    scoped_flush_denormals() noexcept
    {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
        // x86/x64: use MXCSR
        old_mxcsr_             = _mm_getcsr();
        unsigned int new_mxcsr = old_mxcsr_ | (1 << 15) | (1 << 6); // FTZ | DAZ
        _mm_setcsr(new_mxcsr);

#elif defined(__aarch64__) || defined(__arm__)
        // ARM/AArch64: use FPCR (bit 24 = FZ)
        asm volatile("mrs %0, fpcr" : "=r"(old_fpcr_));
        uint64_t new_fpcr = old_fpcr_ | (1ull << 24);
        asm volatile("msr fpcr, %0" : : "r"(new_fpcr));

#else
        // Unsupported architecture: do nothing
#endif
    }

    ~scoped_flush_denormals() noexcept
    {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
        _mm_setcsr(old_mxcsr_);

#elif defined(__aarch64__) || defined(__arm__)
        asm volatile("msr fpcr, %0" : : "r"(old_fpcr_));

#else
        // No state to restore
#endif
    }

    scoped_flush_denormals(const scoped_flush_denormals&)            = delete;
    scoped_flush_denormals& operator=(const scoped_flush_denormals&) = delete;

private:
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    unsigned int old_mxcsr_;
#elif defined(__aarch64__) || defined(__arm__)
    uint64_t old_fpcr_;
#endif
};

} // namespace kfr

#endif
