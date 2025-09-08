/** @addtogroup meta
 *  @{
 */
#pragma once

#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined(__wasm)
#define KFR_ARCH_X86 1
#elif defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(__aarch64__)
#define KFR_ARCH_ARM 1
#endif

#ifdef KFR_ARCH_X86
#if defined(_M_X64) || defined(__x86_64__) || defined(__wasm64)
#define KFR_ARCH_X64 1
#define KFR_ARCH_BITNESS_NAME "64-bit"
#else
#define KFR_ARCH_X32 1
#define KFR_ARCH_BITNESS_NAME "32-bit"
#endif

#ifndef KFR_FORCE_GENERIC_CPU

#if defined __AVX512F__ && !defined KFR_ARCH_AVX512
#define KFR_ARCH_AVX512 1
#define KFR_ARCH_AVX2 1
#define KFR_ARCH_AVX 1
#define KFR_ARCH_SSE4_2 1
#define KFR_ARCH_SSE4_1 1
#define KFR_ARCH_SSE42 1
#define KFR_ARCH_SSE41 1
#define KFR_ARCH_SSSE3 1
#define KFR_ARCH_SSE3 1
#define KFR_ARCH_SSE2 1
#define KFR_ARCH_SSE 1
#endif
#if defined __AVX2__ && !defined KFR_ARCH_AVX2
#define KFR_ARCH_AVX2 1
#define KFR_ARCH_AVX 1
#define KFR_ARCH_SSE4_2 1
#define KFR_ARCH_SSE4_1 1
#define KFR_ARCH_SSE42 1
#define KFR_ARCH_SSE41 1
#define KFR_ARCH_SSSE3 1
#define KFR_ARCH_SSE3 1
#define KFR_ARCH_SSE2 1
#define KFR_ARCH_SSE 1
#endif
#if defined __AVX__ && !defined KFR_ARCH_AVX
#define KFR_ARCH_AVX 1
#define KFR_ARCH_SSE4_2 1
#define KFR_ARCH_SSE4_1 1
#define KFR_ARCH_SSE42 1
#define KFR_ARCH_SSE41 1
#define KFR_ARCH_SSSE3 1
#define KFR_ARCH_SSE3 1
#define KFR_ARCH_SSE2 1
#define KFR_ARCH_SSE 1
#endif
#if defined __SSE4_2__ && !defined KFR_ARCH_SSE4_2
#define KFR_ARCH_SSE4_2 1
#define KFR_ARCH_SSE42 1
#define KFR_ARCH_SSE4_1 1
#define KFR_ARCH_SSE41 1
#define KFR_ARCH_SSSE3 1
#define KFR_ARCH_SSE3 1
#define KFR_ARCH_SSE2 1
#define KFR_ARCH_SSE 1
#endif
#if defined __SSE4_1__ && !defined KFR_ARCH_SSE4_1
#define KFR_ARCH_SSE4_1 1
#define KFR_ARCH_SSE41 1
#define KFR_ARCH_SSSE3 1
#define KFR_ARCH_SSE3 1
#define KFR_ARCH_SSE2 1
#define KFR_ARCH_SSE 1
#endif
#if defined __SSSE3__ && !defined KFR_ARCH_SSSE3
#define KFR_ARCH_SSSE3 1
#define KFR_ARCH_SSE3 1
#define KFR_ARCH_SSE2 1
#define KFR_ARCH_SSE 1
#endif
#if defined __SSE3__ && !defined KFR_ARCH_SSE3
#define KFR_ARCH_SSE3 1
#define KFR_ARCH_SSE2 1
#define KFR_ARCH_SSE 1
#endif
#if (defined KFR_ARCH_X64 || defined __SSE2__ || (defined _M_IX86_FP && _M_IX86_FP == 2)) &&                 \
    !defined KFR_ARCH_SSE2
#define KFR_ARCH_SSE2 1
#define KFR_ARCH_SSE 1
#endif

#if (defined KFR_ARCH_X64 || defined __SSE__ || (defined _M_IX86_FP && _M_IX86_FP == 1)) &&                  \
    !defined KFR_ARCH_SSE
#define KFR_ARCH_SSE 1
#endif

#if defined __FMA__ && !defined KFR_ARCH_FMA
#define KFR_ARCH_FMA 1
#endif

#if defined __AES__ && !defined KFR_ARCH_AES
#define KFR_ARCH_AES 1
#endif

#if defined __BMI__ && !defined KFR_ARCH_BMI
#define KFR_ARCH_BMI 1
#endif

#if defined __BMI2__ && !defined KFR_ARCH_BMI2
#define KFR_ARCH_BMI2 1
#endif

#if defined __LZCNT__ && !defined KFR_ARCH_LZCNT
#define KFR_ARCH_LZCNT 1
#endif

#endif // KFR_FORCE_GENERIC_CPU

#if defined KFR_ARCH_AVX512
#define KFR_ARCH_NAME avx512
#define KFR_ARCH_IS_AVX512 1
#elif defined KFR_ARCH_AVX2
#define KFR_ARCH_NAME avx2
#define KFR_ARCH_IS_AVX2 1
#elif defined KFR_ARCH_AVX
#define KFR_ARCH_NAME avx
#define KFR_ARCH_IS_AVX 1
#elif defined KFR_ARCH_SSE42
#define KFR_ARCH_NAME sse42
#define KFR_ARCH_IS_SSE42 1
#elif defined KFR_ARCH_SSE41
#define KFR_ARCH_NAME sse41
#define KFR_ARCH_IS_SSE4 1
#elif defined KFR_ARCH_SSSE3
#define KFR_ARCH_NAME ssse3
#define KFR_ARCH_IS_SSSE3 1
#elif defined KFR_ARCH_SSE3
#define KFR_ARCH_NAME sse3
#define KFR_ARCH_IS_SSE3 1
#elif defined KFR_ARCH_SSE2
#define KFR_ARCH_NAME sse2
#define KFR_ARCH_IS_SSE2 1
#elif defined KFR_ARCH_SSE
#define KFR_ARCH_NAME sse
#define KFR_ARCH_IS_SSE 1
#else
#define KFR_ARCH_IS_GENERIC 1
#endif

#elif defined(KFR_ARCH_ARM)

#if defined(__aarch64__)
#define KFR_ARCH_X64 1
#define KFR_ARCH_BITNESS_NAME "64-bit"
#else
#define KFR_ARCH_X32 1
#define KFR_ARCH_BITNESS_NAME "32-bit"
#endif

#if defined __ARM_NEON__ || defined __ARM_NEON

#if __ARM_ARCH >= 8 && defined(__aarch64__)
#define KFR_ARCH_NEON64 1
#define KFR_ARCH_NEON 1
#define KFR_ARCH_NAME neon64
#else
#define KFR_ARCH_NEON 1
#define KFR_ARCH_NAME neon
#define KFR_NO_NATIVE_F64 1
#endif
#endif

#endif

#if defined KFR_ARCH_ARM && defined KFR_ARCH_X64
#define KFR_ARCH_ARM64 1
#define KFR_ARCH_AARCH64 1
#endif

#ifndef KFR_ARCH_NAME
#define KFR_ARCH_NAME generic
#endif

#define KFR_ARCH_ID_GENERIC 0

#define KFR_ARCH_ID_SSE2 1
#define KFR_ARCH_ID_SSE3 2
#define KFR_ARCH_ID_SSSE3 3
#define KFR_ARCH_ID_SSE41 4
#define KFR_ARCH_ID_SSE42 5
#define KFR_ARCH_ID_AVX 6
#define KFR_ARCH_ID_AVX2 7
#define KFR_ARCH_ID_AVX512 8

#define KFR_ARCH_ID_NEON 1
#define KFR_ARCH_ID_NEON64 2

#ifdef KFR_ENABLE_SSE2
#define KFR_EXPAND_IF_ARCH_sse2(...) __VA_ARGS__
#else
#define KFR_EXPAND_IF_ARCH_sse2(...)
#endif

#ifdef KFR_ENABLE_SSE3
#define KFR_EXPAND_IF_ARCH_sse3(...) __VA_ARGS__
#else
#define KFR_EXPAND_IF_ARCH_sse3(...)
#endif

#ifdef KFR_ENABLE_SSSE3
#define KFR_EXPAND_IF_ARCH_ssse3(...) __VA_ARGS__
#else
#define KFR_EXPAND_IF_ARCH_ssse3(...)
#endif

#ifdef KFR_ENABLE_SSE41
#define KFR_EXPAND_IF_ARCH_sse41(...) __VA_ARGS__
#else
#define KFR_EXPAND_IF_ARCH_sse41(...)
#endif

#ifdef KFR_ENABLE_SSE41
#define KFR_EXPAND_IF_ARCH_sse41(...) __VA_ARGS__
#else
#define KFR_EXPAND_IF_ARCH_sse41(...)
#endif

#ifdef KFR_ENABLE_SSE42
#define KFR_EXPAND_IF_ARCH_sse42(...) __VA_ARGS__
#else
#define KFR_EXPAND_IF_ARCH_sse42(...)
#endif

#ifdef KFR_ENABLE_AVX
#define KFR_EXPAND_IF_ARCH_avx(...) __VA_ARGS__
#else
#define KFR_EXPAND_IF_ARCH_avx(...)
#endif

#ifdef KFR_ENABLE_AVX2
#define KFR_EXPAND_IF_ARCH_avx2(...) __VA_ARGS__
#else
#define KFR_EXPAND_IF_ARCH_avx2(...)
#endif

#ifdef KFR_ENABLE_AVX512
#define KFR_EXPAND_IF_ARCH_avx512(...) __VA_ARGS__
#else
#define KFR_EXPAND_IF_ARCH_avx512(...)
#endif

#ifndef KFR_NO_NATIVE_F64
#define KFR_NATIVE_F64 1
#endif

#ifndef KFR_NO_NATIVE_I64
#define KFR_NATIVE_I64 1
#endif

#define KFR_STRINGIFY2(x) #x
#define KFR_STRINGIFY(x) KFR_STRINGIFY2(x)

#if defined(_WIN32) // Windows
#define KFR_OS_WIN 1
#endif

#if defined(__APPLE__)
#include "TargetConditionals.h"
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define KFR_OS_IOS 1
#define KFR_OS_MOBILE 1
#define KFR_OS_APPLE 1
#elif defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
#define KFR_OS_IOS 1
#define KFR_OS_IOS_SIMULATOR 1
#define KFR_OS_MOBILE 1
#define KFR_OS_APPLE 1
#elif defined(TARGET_OS_MAC) && TARGET_OS_MAC
#define KFR_OS_MAC 1
#define KFR_OS_MACOS 1
#define KFR_OS_OSX 1
#define KFR_OS_APPLE 1
#endif
#define KFR_OS_POSIX 1
#define KFR_OS_APPLE 1
#endif

#if defined(__ANDROID__)
#define KFR_OS_ANDROID 1
#define KFR_OS_MOBILE 1
#define KFR_OS_POSIX 1
#endif

#if defined(__linux__)
#define KFR_OS_LINUX 1
#define KFR_OS_POSIX 1
#endif

#if defined(_MSC_VER) // Visual C/C++
#define KFR_COMPILER_MSVC 1
#define KFR_MSVC_ATTRIBUTES 1
#define KFR_MSC_VER _MSC_VER
#define KFR_COMPILER_IS_MSVC 1
#else
#define KFR_MSC_VER 0
#endif

#if defined(__GNUC__) || defined(__clang__) // GCC, Clang
#define KFR_COMPILER_GNU 1

#if !defined(__clang__) // GCC only
#define KFR_COMPILER_GCC 1
#endif

#define KFR_GNU_ATTRIBUTES 1
#define KFR_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#if __cplusplus >= 201103L || defined __GXX_EXPERIMENTAL_CXX0X__
#define KFR_HAS_GXX_CXX11 1
#endif
#else
#define KFR_GCC_VERSION 0
#endif

#if defined(__INTEL_COMPILER) // Intel Compiler
#define KFR_COMPILER_INTEL 1
#define KFR_ICC_VERSION __INTEL_COMPILER
#elif defined(__ICL)
#define KFR_COMPILER_INTEL 1
#define KFR_ICC_VERSION __ICL
#else
#define KFR_ICC_VERSION 0
#endif

#if defined(__clang__) // Clang
#define KFR_COMPILER_CLANG 1
#ifndef KFR_GNU_ATTRIBUTES
#define KFR_GNU_ATTRIBUTES 1
#endif
#endif

#if defined _MSC_VER && !defined(__clang__) && !defined(KFR_FORCE_INLINE_MSVC)
#define KFR_NO_FORCE_INLINE 1
#endif

#if defined(KFR_COMPILER_INTEL) || defined(KFR_COMPILER_CLANG)
#ifdef KFR_COMPILER_IS_MSVC
#undef KFR_COMPILER_IS_MSVC
#endif
#endif

#if defined(KFR_GNU_ATTRIBUTES)

#define KFR_NODEBUG

#ifndef KFR_NO_FORCE_INLINE
#define KFR_ALWAYS_INLINE __attribute__((__always_inline__))
#else
#define KFR_ALWAYS_INLINE
#endif

#ifdef NDEBUG
#define KFR_INLINE_IN_RELEASE KFR_ALWAYS_INLINE
#else
#define KFR_INLINE_IN_RELEASE
#endif

#define KFR_INLINE __inline__ KFR_INLINE_IN_RELEASE
#define KFR_INLINE_MEMBER KFR_INLINE_IN_RELEASE
#if defined(KFR_COMPILER_GCC) &&                                                                             \
    (KFR_GCC_VERSION >= 900 && KFR_GCC_VERSION < 904 || KFR_GCC_VERSION >= 1000 && KFR_GCC_VERSION < 1002)
// Workaround for GCC 9/10 bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90333
#define KFR_INLINE_LAMBDA
#else
#define KFR_INLINE_LAMBDA KFR_INLINE_MEMBER
#endif
#define KFR_NOINLINE __attribute__((__noinline__))
#ifndef KFR_NO_FORCE_INLINE
#define KFR_FLATTEN __attribute__((__flatten__))
#else
#define KFR_FLATTEN
#endif
#define KFR_RESTRICT __restrict__

#define KFR_LIKELY(...) __builtin_expect(!!(__VA_ARGS__), 1)
#define KFR_UNLIKELY(...) __builtin_expect(!!(__VA_ARGS__), 0)

#elif defined(KFR_MSVC_ATTRIBUTES)

#ifndef KFR_NO_FORCE_INLINE
#if _MSC_VER >= 1927 && _MSVC_LANG >= 202002L
#define KFR_ALWAYS_INLINE [[msvc::forceinline]]
#else
#define KFR_ALWAYS_INLINE __forceinline
#endif
#else
#define KFR_ALWAYS_INLINE
#endif

#ifdef NDEBUG
#define KFR_INLINE_IN_RELEASE KFR_ALWAYS_INLINE
#else
#define KFR_INLINE_IN_RELEASE
#endif

#define KFR_NODEBUG
#define KFR_INLINE inline KFR_INLINE_IN_RELEASE
#define KFR_INLINE_MEMBER KFR_INLINE_IN_RELEASE
#if _MSC_VER >= 1927 && _MSVC_LANG >= 202002L
#define KFR_INLINE_LAMBDA [[msvc::forceinline]]
#else
#define KFR_INLINE_LAMBDA
#endif
#define KFR_NOINLINE __declspec(noinline)
#define KFR_FLATTEN
#define KFR_RESTRICT __restrict

#define KFR_LIKELY(...) (__VA_ARGS__)
#define KFR_UNLIKELY(...) (__VA_ARGS__)

#endif

#define KFR_INTRINSIC KFR_INLINE KFR_NODEBUG
#define KFR_MEM_INTRINSIC KFR_INLINE KFR_NODEBUG

#if defined _MSC_VER && _MSC_VER >= 1900 &&                                                                  \
    (!defined(__clang__) ||                                                                                  \
     (defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 9))))
#define KFR_EMPTY_BASES __declspec(empty_bases)
#else
#define KFR_EMPTY_BASES
#endif

#define KFR_EXTERN_C extern "C"

#define KFR_PUBLIC_C KFR_EXTERN_C KFR_NOINLINE

#ifdef KFR_ARCH_x86
#ifdef KFR_OS_WIN
#define KFR_CDECL __cdecl
#else
#define KFR_CDECL __attribute__((cdecl))
#endif
#else
#define KFR_CDECL
#endif

#ifdef KFR_OS_WIN
#if defined(KFR_MSVC_ATTRIBUTES)
#define KFR_DLL_EXPORT __declspec(dllexport)
#define KFR_DLL_IMPORT __declspec(dllimport)
#else
#define KFR_DLL_EXPORT __attribute__((dllexport))
#define KFR_DLL_IMPORT __attribute__((dllimport))
#endif
#else
#define KFR_DLL_EXPORT
#define KFR_DLL_IMPORT
#endif

#ifdef __has_builtin
#define KFR_HAS_BUILTIN(builtin) __has_builtin(builtin)
#else
#define KFR_HAS_BUILTIN(builtin) 0
#endif

#define KFR_NOOP                                                                                             \
    do                                                                                                       \
    {                                                                                                        \
    } while (0)

#if KFR_HAS_BUILTIN(__builtin_assume)
#define KFR_ASSUME(x) __builtin_assume(x)
#else
#define KFR_ASSUME(x) KFR_NOOP
#endif

#if KFR_HAS_BUILTIN(__builtin_assume_aligned)
#define KFR_ASSUME_ALIGNED(x, a) __builtin_assume_aligned(x, a)
#else
#define KFR_ASSUME_ALIGNED(x, a) x
#endif

#ifdef __has_feature
#define KFR_HAS_FEATURE(feature) __has_feature(feature)
#else
#define KFR_HAS_FEATURE(feature) 0
#endif

#ifdef __has_extension
#define KFR_HAS_EXTENSION(extension) __has_extension(extension)
#else
#define KFR_HAS_EXTENSION(extension) 0
#endif

#ifdef __has_attribute
#define KFR_HAS_ATTRIBUTE(attribute) __has_attribute(attribute)
#else
#define KFR_HAS_ATTRIBUTE(attribute) 0
#endif

#ifdef __has_warning
#define KFR_HAS_WARNING(warning) __has_warning(warning)
#else
#define KFR_HAS_WARNING(warning) 0
#endif

#ifdef KFR_BUILDING_DLL
#define KFR_C_API KFR_DLL_EXPORT
#else
#define KFR_C_API KFR_DLL_IMPORT
#endif

#if KFR_COMPILER_GNU && !defined(__EXCEPTIONS)
#define KFR_HAS_EXCEPTIONS 0
#endif
#if KFR_COMPILER_MSVC && !_HAS_EXCEPTIONS
#define KFR_HAS_EXCEPTIONS 0
#endif

#ifndef KFR_HAS_EXCEPTIONS
#define KFR_HAS_EXCEPTIONS 1
#endif

#if defined __has_include
#if __has_include(<assert.h>)
#include <assert.h>
#define KFR_HAS_ASSERT_H 1
#endif
#endif

#ifndef KFR_THROW
#if KFR_HAS_EXCEPTIONS
#define KFR_THROW(x) throw x
#else
#ifdef KFR_HAS_ASSERT_H
#define KFR_THROW(x) assert(false)
#else
#define KFR_THROW(x) abort()
#endif
#endif
#endif

#ifdef KFR_COMPILER_MSVC
#define KFR_FUNC_SIGNATURE __FUNCSIG__
#else
#define KFR_FUNC_SIGNATURE __PRETTY_FUNCTION__
#endif

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

#define KFR_PRAGMA(...) _Pragma(#__VA_ARGS__)

#if defined(KFR_GNU_ATTRIBUTES)
#define KFR_FAST_CC __attribute__((fastcall))
#define KFR_UNUSED __attribute__((unused))
#define KFR_GNU_PACKED __attribute__((packed))
#define KFR_PRAGMA_PACK_PUSH_1
#define KFR_PRAGMA_PACK_POP
#define KFR_FP(h, d) h
#define KFR_PRAGMA_GNU(...) _Pragma(#__VA_ARGS__)
#ifdef KFR_COMPILER_CLANG
#define KFR_PRAGMA_CLANG(...) _Pragma(#__VA_ARGS__)
#else
#define KFR_PRAGMA_CLANG(...)
#endif
#ifdef KFR_COMPILER_CLANG
#define KFR_PRAGMA_GCC(...) _Pragma(#__VA_ARGS__)
#else
#define KFR_PRAGMA_GCC(...)
#endif
#define KFR_PRAGMA_MSVC(...)
#else
#define KFR_FAST_CC __fastcall
#define KFR_UNUSED
#define KFR_GNU_PACKED
#define KFR_PRAGMA_PACK_PUSH_1 __pragma(pack(push, 1))
#define KFR_PRAGMA_PACK_POP __pragma(pack(pop))
#define KFR_FP(h, d) d
#define KFR_PRAGMA_GNU(...)
#define KFR_PRAGMA_CLANG(...)
#define KFR_PRAGMA_GCC(...)
#define KFR_PRAGMA_MSVC(...) __pragma(__VA_ARGS__)
#endif

#if defined KFR_OS_IOS
#define KFR_OS_NAME "ios"
#elif defined KFR_OS_MAC
#define KFR_OS_NAME "macos"
#elif defined KFR_OS_ANDROIS
#define KFR_OS_NAME "android"
#elif defined KFR_OS_LINUX
#define KFR_OS_NAME "linux"
#elif defined KFR_OS_WIN
#define KFR_OS_NAME "windows"
#else
#define KFR_OS_NAME "unknown"
#endif

#if defined KFR_COMPILER_INTEL
#if defined _MSC_VER
#define KFR_COMPILER_NAME "intel-msvc"
#define KFR_COMPILER_FULL_NAME                                                                               \
    "clang-msvc-" KFR_STRINGIFY(__ICL) "." KFR_STRINGIFY(__INTEL_COMPILER_UPDATE) "." KFR_STRINGIFY(         \
        __INTEL_COMPILER_BUILD_DATE)
#else
#define KFR_COMPILER_NAME "intel"
#ifdef __INTEL_CLANG_COMPILER
#define KFR_COMPILER_INTEL_SPEC "-clang"
#ifdef __INTEL_LLVM_COMPILER
#define KFR_COMPILER_INTEL_SPEC "-clang-llvm"
#endif
#else
#ifdef __INTEL_LLVM_COMPILER
#define KFR_COMPILER_INTEL_SPEC "-llvm"
#else
#define KFR_COMPILER_INTEL_SPEC ""
#endif
#endif
#define KFR_COMPILER_FULL_NAME                                                                               \
    "intel-" KFR_STRINGIFY(__INTEL_COMPILER) KFR_COMPILER_INTEL_SPEC                                         \
        "." KFR_STRINGIFY(__INTEL_COMPILER_UPDATE) "." KFR_STRINGIFY(__INTEL_COMPILER_BUILD_DATE)
#endif
#elif defined KFR_COMPILER_CLANG
#if defined _MSC_VER
#define KFR_COMPILER_NAME "clang-msvc"
#define KFR_COMPILER_FULL_NAME                                                                               \
    "clang-msvc-" KFR_STRINGIFY(__clang_major__) "." KFR_STRINGIFY(__clang_minor__) "." KFR_STRINGIFY(       \
        __clang_patchlevel__)
#else
#define KFR_COMPILER_NAME "clang-mingw"
#define KFR_COMPILER_FULL_NAME                                                                               \
    "clang-" KFR_STRINGIFY(__clang_major__) "." KFR_STRINGIFY(__clang_minor__) "." KFR_STRINGIFY(            \
        __clang_patchlevel__)
#endif
#elif defined KFR_COMPILER_GCC
#define KFR_COMPILER_NAME "gcc"
#define KFR_COMPILER_FULL_NAME                                                                               \
    "gcc-" KFR_STRINGIFY(__GNUC__) "." KFR_STRINGIFY(__GNUC_MINOR__) "." KFR_STRINGIFY(__GNUC_PATCHLEVEL__)
#elif defined KFR_COMPILER_MSVC
#define KFR_COMPILER_NAME "msvc"
#define KFR_COMPILER_FULL_NAME "msvc-" KFR_STRINGIFY(_MSC_VER) "." KFR_STRINGIFY(_MSC_FULL_VER)
#else
#define KFR_COMPILER_NAME "unknown"
#define KFR_COMPILER_FULL_NAME "unknown"
#endif

#define KFR_CONCAT(a, b) a##b

#define KFR_NARGS2(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10
#define KFR_NARGS(...) KFR_NARGS2(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define KFR_IF_IS_AVX512(...)
#define KFR_IF_IS_AVX2(...)
#define KFR_IF_IS_AVX(...)
#define KFR_IF_IS_SSE42(...)
#define KFR_IF_IS_SSE41(...)
#define KFR_IF_IS_SSSE3(...)
#define KFR_IF_IS_SSE3(...)
#define KFR_IF_IS_SSE2(...)

#if defined KFR_ARCH_AVX512
#undef KFR_IF_IS_AVX512
#define KFR_IF_IS_AVX512(...) __VA_ARGS__
#elif defined KFR_ARCH_AVX2
#undef KFR_IF_IS_AVX2
#define KFR_IF_IS_AVX2(...) __VA_ARGS__
#elif defined KFR_ARCH_AVX
#undef KFR_IF_IS_AVX
#define KFR_IF_IS_AVX(...) __VA_ARGS__
#elif defined KFR_ARCH_SSE42
#undef KFR_IF_IS_SSE42
#define KFR_IF_IS_SSE42(...) __VA_ARGS__
#elif defined KFR_ARCH_SSE41
#undef KFR_IF_IS_SSE41
#define KFR_IF_IS_SSE41(...) __VA_ARGS__
#elif defined KFR_ARCH_SSSE3
#undef KFR_IF_IS_SSSE3
#define KFR_IF_IS_SSSE3(...) __VA_ARGS__
#elif defined KFR_ARCH_SSE3
#undef KFR_IF_IS_SSE3
#define KFR_IF_IS_SSE3(...) __VA_ARGS__
#elif defined KFR_ARCH_SSE2
#undef KFR_IF_IS_SSE2
#define KFR_IF_IS_SSE2(...) __VA_ARGS__
#endif

#ifdef KFR_COMPILER_GNU
#define KFR_UNREACHABLE                                                                                      \
    do                                                                                                       \
    {                                                                                                        \
        __builtin_unreachable();                                                                             \
    } while (0)
#elif defined(_MSC_VER)
#define KFR_UNREACHABLE                                                                                      \
    do                                                                                                       \
    {                                                                                                        \
        __assume(false);                                                                                     \
    } while (0)
#endif
