/** @addtogroup utility
 *  @{
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../cident.h"

#define KFR_VERSION_STRING "1.2.0"
#define KFR_VERSION_MAJOR 1
#define KFR_VERSION_MINOR 2
#define KFR_VERSION_BUILD 0
#define KFR_VERSION 10200

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

#ifdef CMT_ARCH_X64
#define KFR_VERSION_FULL "KFR " KFR_VERSION_STRING " " CMT_STRINGIFY(CMT_ARCH_NAME) " 64-bit"
#else
#define KFR_VERSION_FULL "KFR " KFR_VERSION_STRING " " CMT_STRINGIFY(CMT_ARCH_NAME) " 32-bit"
#endif

#define KFR_INTRIN CMT_INTRIN
#define KFR_SINTRIN CMT_INTRIN static
