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
