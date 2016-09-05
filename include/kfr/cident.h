/** @addtogroup cometa
 *  @{
 */
#pragma once

#ifdef LIBC_WORKAROUND_GETS
extern char* gets(char* __s);
#endif

#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__)
#define CMT_ARCH_X86 1
#elif defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(__aarch64__)
#define CMT_ARCH_ARM 1
#endif

#ifdef CMT_ARCH_X86
#if defined(_M_X64) || defined(__x86_64__)
#define CMT_ARCH_X64 1
#else
#define CMT_ARCH_X32 1
#endif

#if defined __AVX512F__ && !defined CMT_ARCH_AVX512
#define CMT_ARCH_AVX512 1
#define CMT_ARCH_AVX2 1
#define CMT_ARCH_AVX 1
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
#define CMT_ARCH_SSE42 1
#define CMT_ARCH_SSE41 1
#define CMT_ARCH_SSSE3 1
#define CMT_ARCH_SSE3 1
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif
#if defined __AVX__ && !defined CMT_ARCH_AVX
#define CMT_ARCH_AVX 1
#define CMT_ARCH_SSE42 1
#define CMT_ARCH_SSE41 1
#define CMT_ARCH_SSSE3 1
#define CMT_ARCH_SSE3 1
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif
#if defined __SSE4_2__ && !defined CMT_ARCH_SSE4_2
#define CMT_ARCH_SSE4_2 1
#define CMT_ARCH_SSE41 1
#define CMT_ARCH_SSSE3 1
#define CMT_ARCH_SSE3 1
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif
#if defined __SSE4_1__ && !defined CMT_ARCH_SSE4_1
#define CMT_ARCH_SSE4_1 1
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
#if (defined CMT_ARCH_X64 || defined __SSE2__) && !defined CMT_ARCH_SSE2
#define CMT_ARCH_SSE2 1
#define CMT_ARCH_SSE 1
#endif

#if (defined CMT_ARCH_X64 || defined __SSE__) && !defined CMT_ARCH_SSE1
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

#if defined CMT_ARCH_AVX512
#define CMT_ARCH_NAME avx512
#elif defined CMT_ARCH_AVX2
#define CMT_ARCH_NAME avx2
#elif defined CMT_ARCH_AVX
#define CMT_ARCH_NAME avx
#elif defined CMT_ARCH_SSE4_1
#define CMT_ARCH_NAME sse41
#elif defined CMT_ARCH_SSSE3
#define CMT_ARCH_NAME ssse3
#elif defined CMT_ARCH_SSE3
#define CMT_ARCH_NAME sse3
#elif defined CMT_ARCH_SSE2
#define CMT_ARCH_NAME sse2
#elif defined CMT_ARCH_SSE
#define CMT_ARCH_NAME sse
#endif

#elif defined(CMT_ARCH_ARM)

#if defined(__aarch64__)
#define CMT_ARCH_X64 1
#else
#define CMT_ARCH_X32 1
#endif

#ifdef __ARM_NEON__

#if __ARM_ARCH >= 8 && defined(__aarch64__)
#define CMT_ARCH_NEON64 1
#define CMT_ARCH_NEON 1
#define CMT_ARCH_NAME neon64
#else
#define CMT_ARCH_NEON 1
#define CMT_ARCH_NAME neon
#define KFR_NO_NATIVE_F64 1
#endif
#endif

#endif

#ifndef CMT_ARCH_NAME
#define CMT_ARCH_NAME common
#endif

#ifndef KFR_NO_NATIVE_F64
#define KFR_NATIVE_F64 1
#endif

#ifndef KFR_NO_NATIVE_I64
#define KFR_NATIVE_I64 1
#endif

#define CMT_STRINGIFY2(x) #x
#define CMT_STRINGIFY(x) CMT_STRINGIFY2(x)

#if defined(_WIN32) // Windows
#define CMT_OS_WIN 1
#endif

#if defined(__APPLE__)
#include "TargetConditionals.h"
#ifdef TARGET_OS_IPHONE
#define CMT_OS_IOS 1
#define CMT_OS_MOBILE 1
#elif TARGET_IPHONE_SIMULATOR
#define CMT_OS_IOS 1
#define CMT_OS_IOS_SIMULATOR 1
#define CMT_OS_MOBILE 1
#elif TARGET_OS_MAC
#define CMT_OS_MAC 1
#define CMT_OS_MACOS 1
#define CMT_OS_OSX 1
#endif
#define CMT_OS_POSIX 1
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
#else
#define CMT_MSC_VER 0
#endif

#if defined(__GNUC__) || defined(__clang__) // GCC, Clang
#define CMT_COMPILER_GNU 1
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

#if defined(CMT_GNU_ATTRIBUTES)

#define CMT_NODEBUG
// __attribute__((__nodebug__))
#ifdef NDEBUG
#define CMT_ALWAYS_INLINE __attribute__((__always_inline__))
#else
#define CMT_ALWAYS_INLINE
#endif
#define CMT_INLINE __inline__ CMT_ALWAYS_INLINE
#define CMT_INTRIN CMT_INLINE CMT_NODEBUG
#define CMT_INLINE_MEMBER CMT_ALWAYS_INLINE
#define CMT_INLINE_LAMBDA CMT_INLINE_MEMBER
#define CMT_NOINLINE __attribute__((__noinline__))
#define CMT_FLATTEN __attribute__((__flatten__))
#define CMT_RESTRICT __restrict__

#elif defined(CMT_MSVC_ATTRIBUTES)

#define CMT_NODEBUG
#define CMT_INLINE inline __forceinline
#define CMT_INTRIN CMT_INLINE CMT_NODEBUG
#define CMT_INLINE_MEMBER __forceinline
#define CMT_INLINE_LAMBDA
#define CMT_NOINLINE __declspec(noinline)
#define CMT_FLATTEN
#define CMT_RESTRICT __restrict

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

#if CMT_HAS_BUILTIN(CMT_ASSUME)
#define CMT_ASSUME(x) __builtin_assume(x)
#else
#define CMT_ASSUME(x)                                                                                        \
    do                                                                                                       \
    {                                                                                                        \
    } while (0)
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
#else
#define CMT_NOEXCEPT
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

#if __has_include(<assert.h>)
#include <assert.h>
#define CMT_HAS_ASSERT_H 1
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

#if defined(CMT_GNU_ATTRIBUTES)
#define CMT_FAST_CC __attribute__((fastcall))
#else
#define CMT_FAST_CC __fastcall
#endif
