#pragma once

#include "kfr.h"

#ifdef CID_ARCH_X86
#include <immintrin.h>
#ifdef KFR_OS_WIN
#include <intrin.h>
#endif
#else
#include <arm_neon.h>
#endif
