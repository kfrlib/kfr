#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../cident.h"

#ifdef __cplusplus
namespace kfr
{
using ::cid::arraysize;
}
#endif

#define KFR_VERSION_STRING "1.0.0"
#define KFR_VERSION_MAJOR 1
#define KFR_VERSION_MINOR 0
#define KFR_VERSION_BUILD 0
#define KFR_VERSION 10000

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

#define KFR_INTRIN CMT_INTRIN
#define KFR_SINTRIN CMT_INTRIN static
