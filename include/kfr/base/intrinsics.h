#pragma once

#include "kfr.h"

#ifdef CMT_ARCH_SSE2
#include <immintrin.h>
#ifdef CMT_OS_WIN
#include <intrin.h>
#endif
#endif

#ifdef CMT_ARCH_NEON
#include <arm_neon.h>
#endif

#if defined CMT_COMPILER_GCC && defined CMT_ARCH_X86
#include <x86intrin.h>
#endif
