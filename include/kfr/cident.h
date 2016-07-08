#pragma once

#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__)
#define CID_ARCH_X86 1
#endif

#ifdef CID_ARCH_X86
#if defined(_M_X64) || defined(__x86_64__)
#define CID_ARCH_X64 1
#else
#define CID_ARCH_X32 1
#endif

#if defined __AVX512F__ && !defined CID_ARCH_AVX512
#define CID_ARCH_AVX512 1
#define CID_ARCH_AVX2 1
#define CID_ARCH_AVX 1
#define CID_ARCH_SSE42 1
#define CID_ARCH_SSE41 1
#define CID_ARCH_SSSE3 1
#define CID_ARCH_SSE3 1
#define CID_ARCH_SSE2 1
#define CID_ARCH_SSE 1
#endif
#if defined __AVX2__ && !defined CID_ARCH_AVX2
#define CID_ARCH_AVX2 1
#define CID_ARCH_AVX 1
#define CID_ARCH_SSE42 1
#define CID_ARCH_SSE41 1
#define CID_ARCH_SSSE3 1
#define CID_ARCH_SSE3 1
#define CID_ARCH_SSE2 1
#define CID_ARCH_SSE 1
#endif
#if defined __AVX__ && !defined CID_ARCH_AVX
#define CID_ARCH_AVX 1
#define CID_ARCH_SSE42 1
#define CID_ARCH_SSE41 1
#define CID_ARCH_SSSE3 1
#define CID_ARCH_SSE3 1
#define CID_ARCH_SSE2 1
#define CID_ARCH_SSE 1
#endif
#if defined __SSE4_2__ && !defined CID_ARCH_SSE4_2
#define CID_ARCH_SSE4_2 1
#define CID_ARCH_SSE41 1
#define CID_ARCH_SSSE3 1
#define CID_ARCH_SSE3 1
#define CID_ARCH_SSE2 1
#define CID_ARCH_SSE 1
#endif
#if defined __SSE4_1__ && !defined CID_ARCH_SSE4_1
#define CID_ARCH_SSE4_1 1
#define CID_ARCH_SSSE3 1
#define CID_ARCH_SSE3 1
#define CID_ARCH_SSE2 1
#define CID_ARCH_SSE 1
#endif
#if defined __SSSE3__ && !defined CID_ARCH_SSSE3
#define CID_ARCH_SSSE3 1
#define CID_ARCH_SSE3 1
#define CID_ARCH_SSE2 1
#define CID_ARCH_SSE 1
#endif
#if defined __SSE3__ && !defined CID_ARCH_SSE3
#define CID_ARCH_SSE3 1
#define CID_ARCH_SSE2 1
#define CID_ARCH_SSE 1
#endif
#if (defined CID_ARCH_X64 || defined __SSE2__) && !defined CID_ARCH_SSE2
#define CID_ARCH_SSE2 1
#define CID_ARCH_SSE 1
#endif

#if (defined CID_ARCH_X64 || defined __SSE__) && !defined CID_ARCH_SSE1
#define CID_ARCH_SSE 1
#endif

#if defined __FMA__ && !defined CID_ARCH_FMA
#define CID_ARCH_FMA 1
#endif

#if defined __AES__ && !defined CID_ARCH_AES
#define CID_ARCH_AES 1
#endif

#if defined __BMI__ && !defined CID_ARCH_BMI
#define CID_ARCH_BMI 1
#endif

#if defined __BMI2__ && !defined CID_ARCH_BMI2
#define CID_ARCH_BMI2 1
#endif

#if defined __LZCNT__ && !defined CID_ARCH_LZCNT
#define CID_ARCH_LZCNT 1
#endif

#if defined CID_ARCH_AVX512
#define CID_ARCH_NAME avx512
#elif defined CID_ARCH_AVX2
#define CID_ARCH_NAME avx2
#elif defined CID_ARCH_AVX
#define CID_ARCH_NAME avx
#elif defined CID_ARCH_SSE4_1
#define CID_ARCH_NAME sse41
#elif defined CID_ARCH_SSSE3
#define CID_ARCH_NAME ssse3
#elif defined CID_ARCH_SSE3
#define CID_ARCH_NAME sse3
#elif defined CID_ARCH_SSE2
#define CID_ARCH_NAME sse2
#elif defined CID_ARCH_SSE
#define CID_ARCH_NAME sse
#else
#define CID_ARCH_NAME legacy
#endif

#endif

#define CID_STRINGIFY2(x) #x
#define CID_STRINGIFY(x) CID_STRINGIFY2(x)

#if defined(_WIN32) // Windows
#define CID_OS_WIN 1
#endif

#if defined(__APPLE__)
#include "TargetConditionals.h"
#ifdef TARGET_OS_IPHONE
#define CID_OS_IOS 1
#define CID_OS_MOBILE 1
#elif TARGET_IPHONE_SIMULATOR
#define CID_OS_IOS 1
#define CID_OS_IOS_SIMULATOR 1
#define CID_OS_MOBILE 1
#elif TARGET_OS_MAC
#define CID_OS_MAC 1
#define CID_OS_MACOS 1
#define CID_OS_OSX 1
#endif
#define CID_OS_POSIX 1
#endif

#if defined(__ANDROID__)
#define CID_OS_ANDROID 1
#define CID_OS_MOBILE 1
#define CID_OS_POSIX 1
#endif

#if defined(__linux__)
#define CID_OS_LINUX 1
#define CID_OS_POSIX 1
#endif

#if defined(_MSC_VER) // Visual C/C++
#define CID_COMPILER_MSVC 1
#define CID_MSVC_ATTRIBUTES 1
#define CID_MSC_VER _MSC_VER
#else
#define CID_MSC_VER 0
#endif

#if defined(__GNUC__) // GCC, Clang
#define CID_COMPILER_GNU 1
#define CID_GNU_ATTRIBUTES 1
#define CID_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#if __cplusplus >= 201103L || defined __GXX_EXPERIMENTAL_CXX0X__
#define CID_HAS_GXX_CXX11 1
#endif
#else
#define CID_GCC_VERSION 0
#endif

#if defined(__INTEL_COMPILER) // Intel Compiler
#define CID_COMPILER_INTEL 1
#define CID_ICC_VERSION __INTEL_COMPILER
#elif defined(__ICL)
#define CID_COMPILER_INTEL 1
#define CID_ICC_VERSION __ICL
#else
#define CID_ICC_VERSION 0
#endif

#if defined(__clang__) // Clang
#define CID_COMPILER_CLANG 1
#ifndef CID_GNU_ATTRIBUTES
#define CID_GNU_ATTRIBUTES 1
#endif
#endif

#if defined(CID_GNU_ATTRIBUTES)

#define CID_NODEBUG
// __attribute__((__nodebug__))
#define CID_INLINE __inline__ __attribute__((__always_inline__))
#define CID_INTRIN CID_INLINE CID_NODEBUG
#define CID_INLINE_MEMBER __attribute__((__always_inline__))
#define CID_INLINE_LAMBDA CID_INLINE_MEMBER
#define CID_NOINLINE __attribute__((__noinline__))
#define CID_FLATTEN __attribute__((__flatten__))
#define CID_RESTRICT __restrict__

#elif defined(CID_MSVC_ATTRIBUTES)

#define CID_NODEBUG
#define CID_INLINE inline __forceinline
#define CID_INTRIN CID_INLINE CID_NODEBUG
#define CID_INLINE_MEMBER __forceinline
#define CID_INLINE_LAMBDA
#define CID_NOINLINE __declspec(noinline)
#define CID_FLATTEN
#define CID_RESTRICT __restrict

#endif

#define CID_INLINE_STATIC CID_INLINE static

#define CID_EXTERN_C extern "C"

#define CID_PUBLIC_C CID_EXTERN_C CID_NOINLINE

#define CID_ALWAYS_INLINE_STATIC CID_ALWAYS_INLINE static

#ifdef CID_OS_WIN
#define CID_CDECL __cdecl
#else
#define CID_CDECL __attribute__((cdecl))
#endif

#ifdef CID_OS_WIN
#if defined(CID_MSVC_ATTRIBUTES)
#define CID_DLL_EXPORT __declspec(dllexport)
#define CID_DLL_IMPORT __declspec(dllimport)
#else
#define CID_DLL_EXPORT __attribute__((dllexport))
#define CID_DLL_IMPORT __attribute__((dllimport))
#endif
#else
#define CID_DLL_EXPORT
#define CID_DLL_IMPORT
#endif

#ifdef __has_builtin
#define CID_HAS_BUILTIN(builtin) __has_builtin(builtin)
#else
#define CID_HAS_BUILTIN(builtin) 0
#endif

#ifdef __has_feature
#define CID_HAS_FEATURE(feature) __has_feature(feature)
#else
#define CID_HAS_FEATURE(feature) 0
#endif

#ifdef __has_extension
#define CID_HAS_EXTENSION(extension) __has_extension(extension)
#else
#define CID_HAS_EXTENSION(extension) 0
#endif

#ifdef __has_attribute
#define CID_HAS_ATTRIBUTE(attribute) __has_attribute(attribute)
#else
#define CID_HAS_ATTRIBUTE(attribute) 0
#endif

#ifdef __has_warning
#define CID_HAS_WARNING(warning) __has_warning(warning)
#else
#define CID_HAS_WARNING(warning) 0
#endif

#define CID_HAS_VARIADIC_TEMPLATES                                                                           \
    (CID_HAS_FEATURE(cxx_variadic_templates) || (CID_GCC_VERSION >= 404 && CID_HAS_GXX_CXX11) ||             \
     CID_MSC_VER >= 1800)

#ifdef CID_BUILDING_DLL
#define CID_C_API CID_DLL_EXPORT
#else
#define CID_C_API CID_DLL_IMPORT
#endif

#if __cplusplus >= 201103L || CID_MSC_VER >= 1900 || CID_HAS_FEATURE(cxx_constexpr)
#define CID_HAS_CONSTEXPR 1
#endif

#if __cpp_constexpr >= 201304 || CID_HAS_FEATURE(cxx_constexpr)
#define CID_HAS_FULL_CONSTEXPR 1
#endif

#if CID_HAS_CONSTEXPR
#define CID_CONSTEXPR constexpr
#else
#define CID_CONSTEXPR
#endif

#if CID_HAS_FEATURE(cxx_noexcept) || (CID_GCC_VERSION >= 408 && CID_HAS_GXX_CXX11) || CID_MSC_VER >= 1900
#define CID_HAS_NOEXCEPT 1
#endif

#if CID_HAS_NOEXCEPT
#define CID_NOEXCEPT noexcept
#else
#define CID_NOEXCEPT
#endif

#if CID_COMPILER_GNU && !defined(__EXCEPTIONS)
#define CID_HAS_EXCEPTIONS 0
#endif
#if CID_MSC_VER && !_HAS_EXCEPTIONS
#define CID_HAS_EXCEPTIONS 0
#endif

#ifndef CID_HAS_EXCEPTIONS
#define CID_HAS_EXCEPTIONS 1
#endif

#include <assert.h>

#ifndef CID_THROW
#if CID_HAS_EXCEPTIONS
#define CID_THROW(x) throw x
#else
#define CID_THROW(x) assert(false)
#endif
#endif

#if __cplusplus >= 201103L || CID_MSC_VER >= 1900 || CID_HAS_FEATURE(cxx_constexpr)

#include <cstdint>
namespace cid
{
template <typename T, size_t N>
constexpr inline static size_t arraysize(const T (&)[N]) noexcept
{
    return N;
}
}

#define CID_ARRAYSIZE(arr) ::cid::arraysize(arr)
#elif CID_COMPILER_MSVC
#define CID_ARRAYSIZE(arr) _countof(arr)
#elif __cplusplus >= 199711L &&                                                                              \
    (defined(__INTEL_COMPILER) || defined(__clang__) ||                                                      \
     (defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))))
template <typename T, size_t N>
char (&COUNTOF_REQUIRES_ARRAY_ARGUMENT(T (&)[N]))[N];
#define CID_ARRAYSIZE(x) sizeof(COUNTOF_REQUIRES_ARRAY_ARGUMENT(x))
#else
#define CID_ARRAYSIZE(arr) sizeof(arr) / sizeof(arr[0])
#endif

#ifdef CID_COMPILER_MSVC
#define CID_FUNC_SIGNATURE __FUNCSIG__
#else
#define CID_FUNC_SIGNATURE __PRETTY_FUNCTION__
#endif
