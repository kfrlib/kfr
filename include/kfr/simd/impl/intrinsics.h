#pragma once

#include "../../cident.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef CMT_ARCH_SSE2
#include <immintrin.h>
#ifdef CMT_OS_WIN
#include <intrin.h>
#endif
#endif

#ifdef CMT_ARCH_NEON
#include <arm_neon.h>
#endif

#if defined CMT_COMPILER_GCC && defined CMT_ARCH_X86
#include <x86intrin.h>
#endif

#ifdef CMT_COMPILER_CLANG
#define builtin_addressof(x) __builtin_addressof(x)
#else
template <class T>
inline T* builtin_addressof(T& arg)
{
    return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(arg)));
}
#endif

#ifdef CMT_COMPILER_GNU
CMT_INLINE float builtin_sqrt(float x) { return __builtin_sqrtf(x); }
CMT_INLINE double builtin_sqrt(double x) { return __builtin_sqrt(x); }
CMT_INLINE long double builtin_sqrt(long double x) { return __builtin_sqrtl(x); }
CMT_INLINE void builtin_memcpy(void* dest, const void* src, size_t size)
{
    __builtin_memcpy(dest, src, size);
}
CMT_INLINE void builtin_memmove(void* dest, const void* src, size_t size)
{
    __builtin_memmove(dest, src, size);
}
CMT_INLINE void builtin_memset(void* dest, int val, size_t size) { __builtin_memset(dest, val, size); }
#else
CMT_INLINE float builtin_sqrt(float x) { return ::sqrtf(x); }
CMT_INLINE double builtin_sqrt(double x) { return ::sqrt(x); }
CMT_INLINE long double builtin_sqrt(long double x) { return ::sqrtl(x); }
CMT_INLINE void builtin_memcpy(void* dest, const void* src, size_t size) { ::memcpy(dest, src, size); }
CMT_INLINE void builtin_memmove(void* dest, const void* src, size_t size) { ::memmove(dest, src, size); }
CMT_INLINE void builtin_memset(void* dest, int val, size_t size) { ::memset(dest, val, size); }
#endif

#define KFR_ENABLE_IF CMT_ENABLE_IF
