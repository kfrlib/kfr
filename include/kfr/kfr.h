/** @addtogroup utility
 *  @{
 */
#pragma once

#include "config.h"

#include <stddef.h>
#include <stdint.h>

#include "cident.h"

#define KFR_VERSION_MAJOR 4
#define KFR_VERSION_MINOR 2
#define KFR_VERSION_PATCH 0
#define KFR_VERSION_LABEL ""

#define KFR_VERSION_STRING                                                                                   \
    CMT_STRINGIFY(KFR_VERSION_MAJOR)                                                                         \
    "." CMT_STRINGIFY(KFR_VERSION_MINOR) "." CMT_STRINGIFY(KFR_VERSION_PATCH) KFR_VERSION_LABEL
#define KFR_VERSION (KFR_VERSION_MAJOR * 10000 + KFR_VERSION_MINOR * 100 + KFR_VERSION_PATCH)

#if defined DEBUG || defined KFR_DEBUG
#define KFR_DEBUG_STR " debug"
#elif defined NDEBUG || defined KFR_NDEBUG
#define KFR_DEBUG_STR " optimized"
#else
#define KFR_DEBUG_STR ""
#endif

#define KFR_NATIVE_INTRINSICS 1

#if defined CMT_COMPILER_CLANG && !defined CMT_DISABLE_CLANG_EXT
#define CMT_CLANG_EXT
#endif

#ifdef KFR_NATIVE_INTRINSICS
#define KFR_BUILD_DETAILS_1 " +in"
#else
#define KFR_BUILD_DETAILS_1 ""
#endif

#ifdef CMT_CLANG_EXT
#define KFR_BUILD_DETAILS_2 " +ve"
#else
#define KFR_BUILD_DETAILS_2 ""
#endif

#define KFR_VERSION_FULL                                                                                     \
    "KFR " KFR_VERSION_STRING KFR_DEBUG_STR                                                                  \
    " " CMT_STRINGIFY(CMT_ARCH_NAME) " " CMT_ARCH_BITNESS_NAME " (" CMT_COMPILER_FULL_NAME "/" CMT_OS_NAME   \
                                     ")" KFR_BUILD_DETAILS_1 KFR_BUILD_DETAILS_2

#ifdef __cplusplus
namespace kfr
{
/// @brief KFR version string
constexpr inline const char version_string[] = KFR_VERSION_STRING;

constexpr inline int version_major = KFR_VERSION_MAJOR;
constexpr inline int version_minor = KFR_VERSION_MINOR;
constexpr inline int version_patch = KFR_VERSION_PATCH;
constexpr inline int version       = KFR_VERSION;

/// @brief KFR version string including architecture and compiler name
constexpr inline const char version_full[] = KFR_VERSION_FULL;
} // namespace kfr
#endif

#define KFR_INTRINSIC CMT_INTRINSIC
#define KFR_MEM_INTRINSIC CMT_MEM_INTRINSIC
#ifdef KFR_FUNCTION_IS_INTRINSIC
#define KFR_FUNCTION CMT_INTRINSIC
#else
#define KFR_FUNCTION CMT_FUNCTION
#endif
#ifdef CMT_NATIVE_F64
#define KFR_NATIVE_F64 CMT_NATIVE_F64
#endif

#if defined CMT_ARCH_ARM && !defined CMT_ARCH_NEON && !defined CMT_FORCE_GENERIC_CPU
#error "ARM builds require NEON support. Add -march=native for native build or skip the check with CMT_FORCE_GENERIC_CPU=1"
#endif

#if defined CMT_ARCH_ARM && !defined CMT_COMPILER_CLANG && !defined CMT_FORCE_NON_CLANG
#error "ARM builds require Clang compiler. Disable checking with CMT_FORCE_NON_CLANG"
#endif
