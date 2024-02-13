/** @addtogroup cometa
 *  @{
 */
#pragma once

#ifdef LIBC_WORKAROUND_GETS
extern char* gets(char* __s);
#endif

#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined(__wasm)
#define CMT_ARCH_X86 1
#elif defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(__aarch64__)
#define CMT_ARCH_ARM 1
#endif

#ifdef CMT_ARCH_X86
#if defined(_M_X64) || defined(__x86_64__) || defined(__wasm64)
#define CMT_ARCH_X64 1
#define CMT_ARCH_BITNESS_NAME "64-bit"
#else
#define CMT_ARCH_X32 1
#define CMT_ARCH_BITNESS_NAME "32-bit"
#endif

#ifndef CMT_FORCE_GENERIC_CPU

#if defined __AVX512F__ && !defined CMT_ARCH_AVX512
#define CMT_ARCH_AVX512 1
#define CMT_ARCH_AVX2 1
#define CMT_ARCH_AVX 1
#define CMT_ARCH_SSE4_2 1
#define CMT_ARCH_SSE4_1 1
#define CMT_ARCH_SSE42 1
#define CMT_ARCH_SSE41 1
#define CMT_ARCH_SSSE3 1
#define CMT_ARCH_SSE3 1
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif
#if defined __AVX2__ && !defined CMT_ARCH_AVX2
#define CMT_ARCH_AVX2 1
#define CMT_ARCH_AVX 1
#define CMT_ARCH_SSE4_2 1
#define CMT_ARCH_SSE4_1 1
#define CMT_ARCH_SSE42 1
#define CMT_ARCH_SSE41 1
#define CMT_ARCH_SSSE3 1
#define CMT_ARCH_SSE3 1
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif
#if defined __AVX__ && !defined CMT_ARCH_AVX
#define CMT_ARCH_AVX 1
#define CMT_ARCH_SSE4_2 1
#define CMT_ARCH_SSE4_1 1
#define CMT_ARCH_SSE42 1
#define CMT_ARCH_SSE41 1
#define CMT_ARCH_SSSE3 1
#define CMT_ARCH_SSE3 1
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif
#if defined __SSE4_2__ && !defined CMT_ARCH_SSE4_2
#define CMT_ARCH_SSE4_2 1
#define CMT_ARCH_SSE42 1
#define CMT_ARCH_SSE4_1 1
#define CMT_ARCH_SSE41 1
#define CMT_ARCH_SSSE3 1
#define CMT_ARCH_SSE3 1
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif
#if defined __SSE4_1__ && !defined CMT_ARCH_SSE4_1
#define CMT_ARCH_SSE4_1 1
#define CMT_ARCH_SSE41 1
#define CMT_ARCH_SSSE3 1
#define CMT_ARCH_SSE3 1
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif
#if defined __SSSE3__ && !defined CMT_ARCH_SSSE3
#define CMT_ARCH_SSSE3 1
#define CMT_ARCH_SSE3 1
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif
#if defined __SSE3__ && !defined CMT_ARCH_SSE3
#define CMT_ARCH_SSE3 1
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif
#if (defined CMT_ARCH_X64 || defined __SSE2__ || (defined _M_IX86_FP && _M_IX86_FP == 2)) &&                 \
    !defined CMT_ARCH_SSE2
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif

#if (defined CMT_ARCH_X64 || defined __SSE__ || (defined _M_IX86_FP && _M_IX86_FP == 1)) &&                  \
    !defined CMT_ARCH_SSE
#define CMT_ARCH_SSE 1
#endif

#if defined __FMA__ && !defined CMT_ARCH_FMA
#define CMT_ARCH_FMA 1
#endif

#if defined __AES__ && !defined CMT_ARCH_AES
#define CMT_ARCH_AES 1
#endif

#if defined __BMI__ && !defined CMT_ARCH_BMI
#define CMT_ARCH_BMI 1
#endif

#if defined __BMI2__ && !defined CMT_ARCH_BMI2
#define CMT_ARCH_BMI2 1
#endif

#if defined __LZCNT__ && !defined CMT_ARCH_LZCNT
#define CMT_ARCH_LZCNT 1
#endif

#endif // CMT_FORCE_GENERIC_CPU

#if defined CMT_ARCH_AVX512
#define CMT_ARCH_NAME avx512
#define CMT_ARCH_IS_AVX512 1
#elif defined CMT_ARCH_AVX2
#define CMT_ARCH_NAME avx2
#define CMT_ARCH_IS_AVX2 1
#elif defined CMT_ARCH_AVX
#define CMT_ARCH_NAME avx
#define CMT_ARCH_IS_AVX 1
#elif defined CMT_ARCH_SSE42
#define CMT_ARCH_NAME sse42
#define CMT_ARCH_IS_SSE42 1
#elif defined CMT_ARCH_SSE41
#define CMT_ARCH_NAME sse41
#define CMT_ARCH_IS_SSE4 1
#elif defined CMT_ARCH_SSSE3
#define CMT_ARCH_NAME ssse3
#define CMT_ARCH_IS_SSSE3 1
#elif defined CMT_ARCH_SSE3
#define CMT_ARCH_NAME sse3
#define CMT_ARCH_IS_SSE3 1
#elif defined CMT_ARCH_SSE2
#define CMT_ARCH_NAME sse2
#define CMT_ARCH_IS_SSE2 1
#elif defined CMT_ARCH_SSE
#define CMT_ARCH_NAME sse
#define CMT_ARCH_IS_SSE 1
#else
#define CMT_ARCH_IS_GENERIC 1
#endif

#elif defined(CMT_ARCH_ARM)

#if defined(__aarch64__)
#define CMT_ARCH_X64 1
#define CMT_ARCH_BITNESS_NAME "64-bit"
#else
#define CMT_ARCH_X32 1
#define CMT_ARCH_BITNESS_NAME "32-bit"
#endif

#if defined __ARM_NEON__ || defined __ARM_NEON

#if __ARM_ARCH >= 8 && defined(__aarch64__)
#define CMT_ARCH_NEON64 1
#define CMT_ARCH_NEON 1
#define CMT_ARCH_NAME neon64
#else
#define CMT_ARCH_NEON 1
#define CMT_ARCH_NAME neon
#define CMT_NO_NATIVE_F64 1
#endif
#endif

#endif

#if defined CMT_ARCH_ARM && defined CMT_ARCH_X64
#define CMT_ARCH_ARM64 1
#define CMT_ARCH_AARCH64 1
#endif

#ifndef CMT_ARCH_NAME
#define CMT_ARCH_NAME generic
#endif

#define CMT_ARCH_ID_GENERIC 0

#define CMT_ARCH_ID_SSE2 1
#define CMT_ARCH_ID_SSE3 2
#define CMT_ARCH_ID_SSSE3 3
#define CMT_ARCH_ID_SSE41 4
#define CMT_ARCH_ID_SSE42 5
#define CMT_ARCH_ID_AVX 6
#define CMT_ARCH_ID_AVX2 7
#define CMT_ARCH_ID_AVX512 8

#define CMT_ARCH_ID_NEON 1
#define CMT_ARCH_ID_NEON64 2

#ifdef CMT_ENABLE_SSE2
#define CMT_EXPAND_IF_ARCH_sse2(...) __VA_ARGS__
#else
#define CMT_EXPAND_IF_ARCH_sse2(...)
#endif

#ifdef CMT_ENABLE_SSE3
#define CMT_EXPAND_IF_ARCH_sse3(...) __VA_ARGS__
#else
#define CMT_EXPAND_IF_ARCH_sse3(...)
#endif

#ifdef CMT_ENABLE_SSSE3
#define CMT_EXPAND_IF_ARCH_ssse3(...) __VA_ARGS__
#else
#define CMT_EXPAND_IF_ARCH_ssse3(...)
#endif

#ifdef CMT_ENABLE_SSE41
#define CMT_EXPAND_IF_ARCH_sse41(...) __VA_ARGS__
#else
#define CMT_EXPAND_IF_ARCH_sse41(...)
#endif

#ifdef CMT_ENABLE_SSE41
#define CMT_EXPAND_IF_ARCH_sse41(...) __VA_ARGS__
#else
#define CMT_EXPAND_IF_ARCH_sse41(...)
#endif

#ifdef CMT_ENABLE_SSE42
#define CMT_EXPAND_IF_ARCH_sse42(...) __VA_ARGS__
#else
#define CMT_EXPAND_IF_ARCH_sse42(...)
#endif

#ifdef CMT_ENABLE_AVX
#define CMT_EXPAND_IF_ARCH_avx(...) __VA_ARGS__
#else
#define CMT_EXPAND_IF_ARCH_avx(...)
#endif

#ifdef CMT_ENABLE_AVX2
#define CMT_EXPAND_IF_ARCH_avx2(...) __VA_ARGS__
#else
#define CMT_EXPAND_IF_ARCH_avx2(...)
#endif

#ifdef CMT_ENABLE_AVX512
#define CMT_EXPAND_IF_ARCH_avx512(...) __VA_ARGS__
#else
#define CMT_EXPAND_IF_ARCH_avx512(...)
#endif

#ifndef CMT_NO_NATIVE_F64
#define CMT_NATIVE_F64 1
#endif

#ifndef CMT_NO_NATIVE_I64
#define CMT_NATIVE_I64 1
#endif

#define CMT_STRINGIFY2(x) #x
#define CMT_STRINGIFY(x) CMT_STRINGIFY2(x)

#if defined(_WIN32) // Windows
#define CMT_OS_WIN 1
#endif

#if defined(__APPLE__)
#include "TargetConditionals.h"
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define CMT_OS_IOS 1
#define CMT_OS_MOBILE 1
#define CMT_OS_APPLE 1
#elif defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
#define CMT_OS_IOS 1
#define CMT_OS_IOS_SIMULATOR 1
#define CMT_OS_MOBILE 1
#define CMT_OS_APPLE 1
#elif defined(TARGET_OS_MAC) && TARGET_OS_MAC
#define CMT_OS_MAC 1
#define CMT_OS_MACOS 1
#define CMT_OS_OSX 1
#define CMT_OS_APPLE 1
#endif
#define CMT_OS_POSIX 1
#define CMT_OS_APPLE 1
#endif

#if defined(__ANDROID__)
#define CMT_OS_ANDROID 1
#define CMT_OS_MOBILE 1
#define CMT_OS_POSIX 1
#endif

#if defined(__linux__)
#define CMT_OS_LINUX 1
#define CMT_OS_POSIX 1
#endif

#if defined(_MSC_VER) // Visual C/C++
#define CMT_COMPILER_MSVC 1
#define CMT_MSVC_ATTRIBUTES 1
#define CMT_MSC_VER _MSC_VER
#define CMT_COMPILER_IS_MSVC 1
#else
#define CMT_MSC_VER 0
#endif

#if defined(__GNUC__) || defined(__clang__) // GCC, Clang
#define CMT_COMPILER_GNU 1

#if !defined(__clang__) // GCC only
#define CMT_COMPILER_GCC 1
#endif

#define CMT_GNU_ATTRIBUTES 1
#define CMT_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#if __cplusplus >= 201103L || defined __GXX_EXPERIMENTAL_CXX0X__
#define CMT_HAS_GXX_CXX11 1
#endif
#else
#define CMT_GCC_VERSION 0
#endif

#if defined(__INTEL_COMPILER) // Intel Compiler
#define CMT_COMPILER_INTEL 1
#define CMT_ICC_VERSION __INTEL_COMPILER
#elif defined(__ICL)
#define CMT_COMPILER_INTEL 1
#define CMT_ICC_VERSION __ICL
#else
#define CMT_ICC_VERSION 0
#endif

#if defined(__clang__) // Clang
#define CMT_COMPILER_CLANG 1
#ifndef CMT_GNU_ATTRIBUTES
#define CMT_GNU_ATTRIBUTES 1
#endif
#endif

#if defined _MSC_VER && !defined(__clang__) && !defined(CMT_FORCE_INLINE_MSVC)
#define CMT_NO_FORCE_INLINE 1
#endif

#if defined(CMT_COMPILER_INTEL) || defined(CMT_COMPILER_CLANG)
#ifdef CMT_COMPILER_IS_MSVC
#undef CMT_COMPILER_IS_MSVC
#endif
#endif

#if defined(CMT_GNU_ATTRIBUTES)

#define CMT_NODEBUG

#ifndef CMT_NO_FORCE_INLINE
#define CMT_ALWAYS_INLINE __attribute__((__always_inline__))
#else
#define CMT_ALWAYS_INLINE
#endif

#ifdef NDEBUG
#define CMT_INLINE_IN_RELEASE CMT_ALWAYS_INLINE
#else
#define CMT_INLINE_IN_RELEASE
#endif

#define CMT_INLINE __inline__ CMT_INLINE_IN_RELEASE
#define CMT_INLINE_MEMBER CMT_INLINE_IN_RELEASE
#if defined(CMT_COMPILER_GCC) &&                                                                             \
    (CMT_GCC_VERSION >= 900 && CMT_GCC_VERSION < 904 || CMT_GCC_VERSION >= 1000 && CMT_GCC_VERSION < 1002)
// Workaround for GCC 9/10 bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90333
#define CMT_INLINE_LAMBDA
#else
#define CMT_INLINE_LAMBDA CMT_INLINE_MEMBER
#endif
#define CMT_NOINLINE __attribute__((__noinline__))
#ifndef CMT_NO_FORCE_INLINE
#define CMT_FLATTEN __attribute__((__flatten__))
#else
#define CMT_FLATTEN
#endif
#define CMT_RESTRICT __restrict__

#define CMT_LIKELY(...) __builtin_expect(!!(__VA_ARGS__), 1)
#define CMT_UNLIKELY(...) __builtin_expect(!!(__VA_ARGS__), 0)

#elif defined(CMT_MSVC_ATTRIBUTES)

#ifndef CMT_NO_FORCE_INLINE
#if _MSC_VER >= 1927 && _MSVC_LANG >= 202002L
#define CMT_ALWAYS_INLINE [[msvc::forceinline]]
#else
#define CMT_ALWAYS_INLINE __forceinline
#endif
#else
#define CMT_ALWAYS_INLINE
#endif

#ifdef NDEBUG
#define CMT_INLINE_IN_RELEASE CMT_ALWAYS_INLINE
#else
#define CMT_INLINE_IN_RELEASE
#endif

#define CMT_NODEBUG
#define CMT_INLINE inline CMT_INLINE_IN_RELEASE
#define CMT_INLINE_MEMBER CMT_INLINE_IN_RELEASE
#if _MSC_VER >= 1927 && _MSVC_LANG >= 202002L
#define CMT_INLINE_LAMBDA [[msvc::forceinline]]
#else
#define CMT_INLINE_LAMBDA
#endif
#define CMT_NOINLINE __declspec(noinline)
#define CMT_FLATTEN
#define CMT_RESTRICT __restrict

#define CMT_LIKELY(...) (__VA_ARGS__)
#define CMT_UNLIKELY(...) (__VA_ARGS__)

#endif

#define CMT_INTRINSIC CMT_INLINE CMT_NODEBUG
#define CMT_MEM_INTRINSIC CMT_INLINE CMT_NODEBUG

#if defined _MSC_VER && _MSC_VER >= 1900 &&                                                                  \
    (!defined(__clang__) ||                                                                                  \
     (defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 9))))
#define CMT_EMPTY_BASES __declspec(empty_bases)
#else
#define CMT_EMPTY_BASES
#endif

#define CMT_INLINE_STATIC CMT_INLINE static

#define CMT_EXTERN_C extern "C"

#define CMT_PUBLIC_C CMT_EXTERN_C CMT_NOINLINE

#define CMT_ALWAYS_INLINE_STATIC CMT_ALWAYS_INLINE static

#ifdef CMT_ARCH_x86
#ifdef CMT_OS_WIN
#define CMT_CDECL __cdecl
#else
#define CMT_CDECL __attribute__((cdecl))
#endif
#else
#define CMT_CDECL
#endif

#ifdef CMT_OS_WIN
#if defined(CMT_MSVC_ATTRIBUTES)
#define CMT_DLL_EXPORT __declspec(dllexport)
#define CMT_DLL_IMPORT __declspec(dllimport)
#else
#define CMT_DLL_EXPORT __attribute__((dllexport))
#define CMT_DLL_IMPORT __attribute__((dllimport))
#endif
#else
#define CMT_DLL_EXPORT
#define CMT_DLL_IMPORT
#endif

#ifdef __has_builtin
#define CMT_HAS_BUILTIN(builtin) __has_builtin(builtin)
#else
#define CMT_HAS_BUILTIN(builtin) 0
#endif

#define CMT_NOOP                                                                                             \
    do                                                                                                       \
    {                                                                                                        \
    } while (0)

#if CMT_HAS_BUILTIN(CMT_ASSUME)
#define CMT_ASSUME(x) __builtin_assume(x)
#else
#define CMT_ASSUME(x) CMT_NOOP
#endif

#if CMT_HAS_BUILTIN(CMT_ASSUME)
#define CMT_ASSUME_ALIGNED(x, a) __builtin_assume_aligned(x, a)
#else
#define CMT_ASSUME_ALIGNED(x, a) x
#endif

#ifdef __has_feature
#define CMT_HAS_FEATURE(feature) __has_feature(feature)
#else
#define CMT_HAS_FEATURE(feature) 0
#endif

#ifdef __has_extension
#define CMT_HAS_EXTENSION(extension) __has_extension(extension)
#else
#define CMT_HAS_EXTENSION(extension) 0
#endif

#ifdef __has_attribute
#define CMT_HAS_ATTRIBUTE(attribute) __has_attribute(attribute)
#else
#define CMT_HAS_ATTRIBUTE(attribute) 0
#endif

#ifdef __has_warning
#define CMT_HAS_WARNING(warning) __has_warning(warning)
#else
#define CMT_HAS_WARNING(warning) 0
#endif

#define CMT_HAS_VARIADIC_TEMPLATES                                                                           \
    (CMT_HAS_FEATURE(cxx_variadic_templates) || (CMT_GCC_VERSION >= 404 && CMT_HAS_GXX_CXX11) ||             \
     CMT_MSC_VER >= 1800)

#ifdef CMT_BUILDING_DLL
#define CMT_C_API CMT_DLL_EXPORT
#else
#define CMT_C_API CMT_DLL_IMPORT
#endif

#if __cplusplus >= 201103L || CMT_MSC_VER >= 1900 || CMT_HAS_FEATURE(cxx_constexpr)
#define CMT_HAS_CONSTEXPR 1
#endif

#if __cpp_constexpr >= 201304 || CMT_HAS_FEATURE(cxx_constexpr)
#define CMT_HAS_FULL_CONSTEXPR 1
#endif

#if CMT_HAS_CONSTEXPR
#define CMT_CONSTEXPR constexpr
#else
#define CMT_CONSTEXPR
#endif

#if CMT_HAS_FEATURE(cxx_noexcept) || (CMT_GCC_VERSION >= 408 && CMT_HAS_GXX_CXX11) || CMT_MSC_VER >= 1900
#define CMT_HAS_NOEXCEPT 1
#endif

#if CMT_HAS_NOEXCEPT
#define CMT_NOEXCEPT noexcept
#define CMT_NOEXCEPT_SPEC(...) noexcept(__VA_ARGS__)
#else
#define CMT_NOEXCEPT
#define CMT_NOEXCEPT_SPEC(...)
#endif

#if CMT_COMPILER_GNU && !defined(__EXCEPTIONS)
#define CMT_HAS_EXCEPTIONS 0
#endif
#if CMT_COMPILER_MSVC && !_HAS_EXCEPTIONS
#define CMT_HAS_EXCEPTIONS 0
#endif

#ifndef CMT_HAS_EXCEPTIONS
#define CMT_HAS_EXCEPTIONS 1
#endif

#if defined __has_include
#if __has_include(<assert.h>)
#include <assert.h>
#define CMT_HAS_ASSERT_H 1
#endif
#endif

#ifndef CMT_THROW
#if CMT_HAS_EXCEPTIONS
#define CMT_THROW(x) throw x
#else
#ifdef CMT_HAS_ASSERT_H
#define CMT_THROW(x) assert(false)
#else
#define CMT_THROW(x) abort()
#endif
#endif
#endif

#ifdef CMT_COMPILER_MSVC
#define CMT_FUNC_SIGNATURE __FUNCSIG__
#else
#define CMT_FUNC_SIGNATURE __PRETTY_FUNCTION__
#endif

#if CMT_COMPILER_CLANG
#define CMT_LOOP_NOUNROLL                                                                                    \
    _Pragma("clang loop vectorize( disable )") _Pragma("clang loop interleave( disable )")                   \
        _Pragma("clang loop unroll( disable )")

#define CMT_LOOP_UNROLL _Pragma("clang loop unroll( full )")
#define CMT_VEC_CC __attribute__((vectorcall))
#else
#define CMT_LOOP_NOUNROLL
#define CMT_LOOP_UNROLL
#ifdef CMT_COMPILER_MSVC
#define CMT_VEC_CC __vectorcall
#endif
#endif

#define CMT_PRAGMA(...) _Pragma(#__VA_ARGS__)

#if defined(CMT_GNU_ATTRIBUTES)
#define CMT_FAST_CC __attribute__((fastcall))
#define CMT_UNUSED __attribute__((unused))
#define CMT_GNU_CONSTEXPR constexpr
#define CMT_GNU_NOEXCEPT noexcept
#define CMT_GNU_PACKED __attribute__((packed))
#define CMT_PRAGMA_PACK_PUSH_1
#define CMT_PRAGMA_PACK_POP
#define CMT_FP(h, d) h
#define CMT_PRAGMA_GNU(...) _Pragma(#__VA_ARGS__)
#ifdef CMT_COMPILER_CLANG
#define CMT_PRAGMA_CLANG(...) _Pragma(#__VA_ARGS__)
#else
#define CMT_PRAGMA_CLANG(...)
#endif
#ifdef CMT_COMPILER_CLANG
#define CMT_PRAGMA_GCC(...) _Pragma(#__VA_ARGS__)
#else
#define CMT_PRAGMA_GCC(...)
#endif
#define CMT_PRAGMA_MSVC(...)
#else
#define CMT_FAST_CC __fastcall
#define CMT_UNUSED
#define CMT_GNU_CONSTEXPR
#define CMT_GNU_NOEXCEPT
#define CMT_GNU_PACKED
#define CMT_PRAGMA_PACK_PUSH_1 __pragma(pack(push, 1))
#define CMT_PRAGMA_PACK_POP __pragma(pack(pop))
#define CMT_FP(h, d) d
#define CMT_PRAGMA_GNU(...)
#define CMT_PRAGMA_CLANG(...)
#define CMT_PRAGMA_GCC(...)
#define CMT_PRAGMA_MSVC(...) __pragma(__VA_ARGS__)
#endif

#if defined CMT_OS_IOS
#define CMT_OS_NAME "ios"
#elif defined CMT_OS_MAC
#define CMT_OS_NAME "macos"
#elif defined CMT_OS_ANDROIS
#define CMT_OS_NAME "android"
#elif defined CMT_OS_LINUX
#define CMT_OS_NAME "linux"
#elif defined CMT_OS_WIN
#define CMT_OS_NAME "windows"
#else
#define CMT_OS_NAME "unknown"
#endif

#if defined CMT_COMPILER_INTEL
#if defined _MSC_VER
#define CMT_COMPILER_NAME "intel-msvc"
#define CMT_COMPILER_FULL_NAME                                                                               \
    "clang-msvc-" CMT_STRINGIFY(__ICL) "." CMT_STRINGIFY(__INTEL_COMPILER_UPDATE) "." CMT_STRINGIFY(         \
        __INTEL_COMPILER_BUILD_DATE)
#else
#define CMT_COMPILER_NAME "intel"
#ifdef __INTEL_CLANG_COMPILER
#define CMT_COMPILER_INTEL_SPEC "-clang"
#ifdef __INTEL_LLVM_COMPILER
#define CMT_COMPILER_INTEL_SPEC "-clang-llvm"
#endif
#else
#ifdef __INTEL_LLVM_COMPILER
#define CMT_COMPILER_INTEL_SPEC "-llvm"
#else
#define CMT_COMPILER_INTEL_SPEC ""
#endif
#endif
#define CMT_COMPILER_FULL_NAME                                                                               \
    "intel-" CMT_STRINGIFY(__INTEL_COMPILER) CMT_COMPILER_INTEL_SPEC                                         \
        "." CMT_STRINGIFY(__INTEL_COMPILER_UPDATE) "." CMT_STRINGIFY(__INTEL_COMPILER_BUILD_DATE)
#endif
#elif defined CMT_COMPILER_CLANG
#if defined _MSC_VER
#define CMT_COMPILER_NAME "clang-msvc"
#define CMT_COMPILER_FULL_NAME                                                                               \
    "clang-msvc-" CMT_STRINGIFY(__clang_major__) "." CMT_STRINGIFY(__clang_minor__) "." CMT_STRINGIFY(       \
        __clang_patchlevel__)
#else
#define CMT_COMPILER_NAME "clang-mingw"
#define CMT_COMPILER_FULL_NAME                                                                               \
    "clang-" CMT_STRINGIFY(__clang_major__) "." CMT_STRINGIFY(__clang_minor__) "." CMT_STRINGIFY(            \
        __clang_patchlevel__)
#endif
#elif defined CMT_COMPILER_GCC
#define CMT_COMPILER_NAME "gcc"
#define CMT_COMPILER_FULL_NAME                                                                               \
    "gcc-" CMT_STRINGIFY(__GNUC__) "." CMT_STRINGIFY(__GNUC_MINOR__) "." CMT_STRINGIFY(__GNUC_PATCHLEVEL__)
#elif defined CMT_COMPILER_MSVC
#define CMT_COMPILER_NAME "msvc"
#define CMT_COMPILER_FULL_NAME "msvc-" CMT_STRINGIFY(_MSC_VER) "." CMT_STRINGIFY(_MSC_FULL_VER)
#else
#define CMT_COMPILER_NAME "unknown"
#define CMT_COMPILER_FULL_NAME "unknown"
#endif

#define CMT_CONCAT(a, b) a##b

#define CMT_NARGS2(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10
#define CMT_NARGS(...) CMT_NARGS2(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define CMT_IF_IS_AVX512(...)
#define CMT_IF_IS_AVX2(...)
#define CMT_IF_IS_AVX(...)
#define CMT_IF_IS_SSE42(...)
#define CMT_IF_IS_SSE41(...)
#define CMT_IF_IS_SSSE3(...)
#define CMT_IF_IS_SSE3(...)
#define CMT_IF_IS_SSE2(...)

#if defined CMT_ARCH_AVX512
#undef CMT_IF_IS_AVX512
#define CMT_IF_IS_AVX512(...) __VA_ARGS__
#elif defined CMT_ARCH_AVX2
#undef CMT_IF_IS_AVX2
#define CMT_IF_IS_AVX2(...) __VA_ARGS__
#elif defined CMT_ARCH_AVX
#undef CMT_IF_IS_AVX
#define CMT_IF_IS_AVX(...) __VA_ARGS__
#elif defined CMT_ARCH_SSE42
#undef CMT_IF_IS_SSE42
#define CMT_IF_IS_SSE42(...) __VA_ARGS__
#elif defined CMT_ARCH_SSE41
#undef CMT_IF_IS_SSE41
#define CMT_IF_IS_SSE41(...) __VA_ARGS__
#elif defined CMT_ARCH_SSSE3
#undef CMT_IF_IS_SSSE3
#define CMT_IF_IS_SSSE3(...) __VA_ARGS__
#elif defined CMT_ARCH_SSE3
#undef CMT_IF_IS_SSE3
#define CMT_IF_IS_SSE3(...) __VA_ARGS__
#elif defined CMT_ARCH_SSE2
#undef CMT_IF_IS_SSE2
#define CMT_IF_IS_SSE2(...) __VA_ARGS__
#endif

#ifdef CMT_COMPILER_GNU
#define CMT_UNREACHABLE                                                                                      \
    do                                                                                                       \
    {                                                                                                        \
        __builtin_unreachable();                                                                             \
    } while (0)
#elif defined(_MSC_VER)
#define CMT_UNREACHABLE                                                                                      \
    do                                                                                                       \
    {                                                                                                        \
        __assume(false);                                                                                     \
    } while (0)
#endif
