/*
  Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
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
/** @addtogroup utility
 *  @{
 */
#pragma once

#include "config.h"

#include <stddef.h>
#include <stdint.h>

#include "cident.h"

#define KFR_VERSION_MAJOR 7
#define KFR_VERSION_MINOR 0
#define KFR_VERSION_PATCH 0
#define KFR_VERSION_LABEL ""

#define KFR_VERSION_STRING                                                                                   \
    KFR_STRINGIFY(KFR_VERSION_MAJOR)                                                                         \
    "." KFR_STRINGIFY(KFR_VERSION_MINOR) "." KFR_STRINGIFY(KFR_VERSION_PATCH) KFR_VERSION_LABEL
#define KFR_VERSION (KFR_VERSION_MAJOR * 10000 + KFR_VERSION_MINOR * 100 + KFR_VERSION_PATCH)

#if defined DEBUG || defined KFR_DEBUG
#define KFR_DEBUG_STR " debug"
#elif defined NDEBUG || defined KFR_NDEBUG
#define KFR_DEBUG_STR " optimized"
#else
#define KFR_DEBUG_STR ""
#endif

#define KFR_NATIVE_INTRINSICS 1

#if defined KFR_COMPILER_CLANG && !defined KFR_DISABLE_CLANG_EXT
#define KFR_VEC_EXT
#endif

#ifdef KFR_NATIVE_INTRINSICS
#define KFR_BUILD_DETAILS_1 " +in"
#else
#define KFR_BUILD_DETAILS_1 ""
#endif

#ifdef KFR_VEC_EXT
#define KFR_BUILD_DETAILS_2 " +ve"
#else
#define KFR_BUILD_DETAILS_2 ""
#endif

#ifdef KFR_ENABLED_ARCHS
#define KFR_ENABLED_ARCHS_LIST "[" KFR_ENABLED_ARCHS "] "
#else
#define KFR_ENABLED_ARCHS_LIST ""
#endif

#define KFR_VERSION_FULL                                                                                     \
    "KFR " KFR_VERSION_STRING KFR_DEBUG_STR                                                                  \
    " " KFR_STRINGIFY(KFR_ARCH_NAME) " " KFR_ENABLED_ARCHS_LIST KFR_ARCH_BITNESS_NAME                        \
                                     " (" KFR_COMPILER_FULL_NAME "/" KFR_OS_NAME                             \
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

#ifdef KFR_FUNCTION_IS_INTRINSIC
#define KFR_FUNCTION KFR_INTRINSIC
#else
#define KFR_FUNCTION
#endif

#if defined KFR_ARCH_ARM && !defined KFR_ARCH_NEON && !defined KFR_FORCE_GENERIC_CPU
#error                                                                                                       \
    "ARM builds require NEON support. Add -march=native for native build or skip the check with KFR_FORCE_GENERIC_CPU=1"
#endif

#if defined KFR_ARCH_ARM && !defined KFR_COMPILER_CLANG && !defined KFR_FORCE_NON_CLANG
#error "ARM builds require Clang compiler. Disable checking with KFR_FORCE_NON_CLANG"
#endif
