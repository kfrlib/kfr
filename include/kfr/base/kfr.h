#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../cident.h"

#define KFR_INLINE CID_INLINE
#define KFR_INLINE_MEMBER CID_INLINE_MEMBER
#define KFR_INLINE_LAMBDA CID_INLINE_LAMBDA
#define KFR_NOINLINE CID_NOINLINE
#define KFR_FLATTEN CID_FLATTEN
#define KFR_RESTRICT CID_RESTRICT

#ifdef CID_COMPILER_CLANG
#define KFR_COMPILER_CLANG CID_COMPILER_CLANG
#endif

#ifdef CID_OS_WIN
#define KFR_OS_WIN CID_OS_WIN
#endif

#ifdef CID_OS_OSX
#define KFR_OS_OSX CID_OS_OSX
#endif

#ifdef CID_OS_LINUX
#define KFR_OS_LINUX CID_OS_LINUX
#endif

#ifdef CID_GNU_ATTRIBUTES
#define KFR_GNU_ATTRIBUTES CID_GNU_ATTRIBUTES
#endif

#ifdef CID_MSVC_ATTRIBUTES
#define KFR_MSVC_ATTRIBUTES CID_MSVC_ATTRIBUTES
#endif

#ifdef CID_ARCH_X64
#define KFR_ARCH_X64 CID_ARCH_X64
#endif

#ifdef CID_ARCH_X32
#define KFR_ARCH_X32 CID_ARCH_X32
#endif

#define KFR_ARCH_NAME CID_ARCH_NAME

#define KFR_CDECL CID_CDECL

#define KFR_PUBLIC_C CID_PUBLIC_C

#ifdef __cplusplus
namespace kfr
{
using ::cid::arraysize;
}
#endif

#define KFR_VERSION_STRING "0.9.1"
#define KFR_VERSION_MAJOR 0
#define KFR_VERSION_MINOR 9
#define KFR_VERSION_BUILD 1
#define KFR_VERSION 901

#ifdef __cplusplus
namespace kfr
{
constexpr const char version_string[] = KFR_VERSION_STRING;
constexpr int version_major           = KFR_VERSION_MAJOR;
constexpr int version_minor           = KFR_VERSION_MINOR;
constexpr int version_build           = KFR_VERSION_BUILD;
constexpr int version                 = KFR_VERSION;
}
#endif

//#define KFR_MEMORY_ALIGNMENT 64

#if KFR_COMPILER_CLANG
#define KFR_LOOP_NOUNROLL                                                                                    \
    _Pragma("clang loop vectorize( disable )") _Pragma("clang loop interleave( disable )")                   \
        _Pragma("clang loop unroll( disable )")

#define KFR_LOOP_UNROLL _Pragma("clang loop unroll( full )")

#define KFR_VEC_CC __attribute__((vectorcall))
#else
#define KFR_LOOP_NOUNROLL
#define KFR_LOOP_UNROLL
#ifdef KFR_COMPILER_MSVC
#define KFR_VEC_CC __vectorcall
#endif

#endif

#define KFR_AVAIL_AVX2 1
#define KFR_AVAIL_AVX 1
#define KFR_AVAIL_SSE42 1
#define KFR_AVAIL_SSE41 1
#define KFR_AVAIL_SSSE3 1
#define KFR_AVAIL_SSE3 1
#define KFR_AVAIL_SSE2 1
#define KFR_AVAIL_SSE 1

#if defined(KFR_GNU_ATTRIBUTES)

#define KFR_CPU_NAME_avx2 "avx2"
#define KFR_CPU_NAME_avx "avx"
#define KFR_CPU_NAME_sse42 "sse4.2"
#define KFR_CPU_NAME_sse41 "sse4.1"
#define KFR_CPU_NAME_ssse3 "ssse3"
#define KFR_CPU_NAME_sse3 "sse3"
#define KFR_CPU_NAME_sse2 "sse2"

#define KFR_USE_CPU(arch) __attribute__((target(KFR_CPU_NAME_##arch)))

#else
#define KFR_USE_CPU(arch)
#endif

#if defined(KFR_GNU_ATTRIBUTES)
#define KFR_FAST_CC __attribute__((fastcall))
#else
#define KFR_FAST_CC __fastcall
#endif

#define KFR_INTRIN CID_INTRIN
#define KFR_SINTRIN CID_INTRIN CID_NODEBUG static
#define KFR_AINTRIN inline CID_NODEBUG static
#define KFR_FAST_NOINLINE CID_NOINLINE

#define KFR_CPU_INTRIN(c) KFR_AINTRIN KFR_USE_CPU(c)
