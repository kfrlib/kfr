#pragma once

#include "kfr.h"

#ifdef CID_ARCH_SSE2
#include <immintrin.h>
#ifdef KFR_OS_WIN
#include <intrin.h>
#endif
#endif

#ifdef CID_ARCH_NEON
#include <arm_neon.h>
#endif
