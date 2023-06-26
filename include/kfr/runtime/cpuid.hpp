/** @addtogroup cpuid
 *  @{
 */
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
#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "../simd/platform.hpp"
#include "../simd/types.hpp"
#include <cstring>

namespace kfr
{
#ifdef CMT_ARCH_X86

struct cpu_features
{
    u32 max;
    u32 exmax;
    u32 isIntel : 1;
    u32 isAMD : 1;
    u32 has3DNOW : 1;
    u32 has3DNOWEXT : 1;
    u32 hasABM : 1;
    u32 hasADX : 1;
    u32 hasAES : 1;
    u32 hasAVX : 1;
    u32 hasAVX2 : 1;
    u32 hasAVXOSSUPPORT : 1;
    u32 hasAVX512OSSUPPORT : 1;
    u32 hasAVX512CD : 1;
    u32 hasAVX512ER : 1;
    u32 hasAVX512F : 1;
    u32 hasAVX512DQ : 1;
    u32 hasAVX512PF : 1;
    u32 hasAVX512BW : 1;
    u32 hasAVX512VL : 1;
    u32 hasBMI1 : 1;
    u32 hasBMI2 : 1;
    u32 hasCLFSH : 1;
    u32 hasCMOV : 1;
    u32 hasCMPXCHG16B : 1;
    u32 hasCX8 : 1;
    u32 hasERMS : 1;
    u32 hasF16C : 1;
    u32 hasFMA : 1;
    u32 hasFSGSBASE : 1;
    u32 hasFXSR : 1;
    u32 hasHLE : 1;
    u32 hasINVPCID : 1;
    u32 hasLAHF : 1;
    u32 hasLZCNT : 1;
    u32 hasMMX : 1;
    u32 hasMMXEXT : 1;
    u32 hasMONITOR : 1;
    u32 hasMOVBE : 1;
    u32 hasMSR : 1;
    u32 hasOSXSAVE : 1;
    u32 hasPCLMULQDQ : 1;
    u32 hasPOPCNT : 1;
    u32 hasPREFETCHWT1 : 1;
    u32 hasRDRAND : 1;
    u32 hasRDSEED : 1;
    u32 hasRDTSCP : 1;
    u32 hasRTM : 1;
    u32 hasSEP : 1;
    u32 hasSHA : 1;
    u32 hasSSE : 1;
    u32 hasSSE2 : 1;
    u32 hasSSE3 : 1;
    u32 hasSSE41 : 1;
    u32 hasSSE42 : 1;
    u32 hasSSE4a : 1;
    u32 hasSSSE3 : 1;
    u32 hasSYSCALL : 1;
    u32 hasTBM : 1;
    u32 hasXOP : 1;
    u32 hasXSAVE : 1;
    u32 padding1 : 6;
    alignas(int32_t) char vendor[17];
    alignas(int32_t) char model[49];
    alignas(int32_t) char padding2[2];
};

namespace internal_generic
{

struct cpu_data
{
    u32 data[4];
};

#if defined CMT_COMPILER_GNU || defined CMT_COMPILER_CLANG
#if defined __i386__
KFR_INTRINSIC u32 get_cpuid(u32 func, u32 subfunc, u32* eax, u32* ebx, u32* ecx, u32* edx)
{
    __asm__("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "0"(func), "2"(subfunc));
    return 1;
}
#else
KFR_INTRINSIC u32 get_cpuid(u32 func, u32 subfunc, u32* eax, u32* ebx, u32* ecx, u32* edx)
{
    __asm("xchgq  %%rbx,%q1\n"
          "cpuid\n"
          "xchgq  %%rbx,%q1"
          : "=a"(*eax), "=r"(*ebx), "=c"(*ecx), "=d"(*edx)
          : "0"(func), "2"(subfunc));
    return 1;
}
#endif
KFR_INTRINSIC void cpuid(u32* ptr, u32 func, u32 subfunc = 0)
{
    get_cpuid(func, subfunc, &ptr[0], &ptr[1], &ptr[2], &ptr[3]);
}
KFR_INTRINSIC u32 get_xcr0()
{
    u32 xcr0;
    __asm__ __volatile__("xgetbv" : "=a"(xcr0) : "c"(0) : "%edx");
    return xcr0;
}
#elif defined CMT_COMPILER_MSVC

KFR_INTRINSIC void cpuid(u32* ptr, u32 func, u32 subfunc = 0)
{
    __cpuidex((int*)ptr, (int)func, (int)subfunc);
}
KFR_INTRINSIC u32 get_xcr0()
{
#ifdef _XCR_XFEATURE_ENABLED_MASK
    unsigned long long Result = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
    return (u32)Result;
#else
    return 0;
#endif
}
#endif

template <size_t = 0>
cpu_t detect_cpu()
{
    cpu_features c;
    memset(&c, 0, sizeof(c));
    cpu_data data0;
    cpu_data exdata0;

    u32 f_1_ECX(0);
    u32 f_1_EDX(0);
    u32 f_7_EBX(0);
    u32 f_7_ECX(0);
    u32 f_81_ECX(0);
    u32 f_81_EDX(0);

    cpuid(data0.data, 0);
    c.max = static_cast<u32>(data0.data[0]);
    cpuid(exdata0.data, 0x80000000);
    c.exmax = static_cast<u32>(exdata0.data[0]);

    *ptr_cast<u32>(c.vendor)     = static_cast<u32>(data0.data[1]);
    *ptr_cast<u32>(c.vendor + 4) = static_cast<u32>(data0.data[3]);
    *ptr_cast<u32>(c.vendor + 8) = static_cast<u32>(data0.data[2]);

    c.isIntel = strncmp(c.vendor, "GenuineIntel", sizeof(c.vendor)) == 0 ? 1 : 0;
    c.isAMD   = strncmp(c.vendor, "AuthenticAMD", sizeof(c.vendor)) == 0 ? 1 : 0;

    if (c.max >= 1)
    {
        cpu_data data1;
        cpuid(data1.data, 1);
        f_1_ECX = static_cast<u32>(data1.data[2]);
        f_1_EDX = static_cast<u32>(data1.data[3]);
    }

    if (c.max >= 7)
    {
        cpu_data data7;
        cpuid(data7.data, 7);
        f_7_EBX = static_cast<u32>(data7.data[1]);
        f_7_ECX = static_cast<u32>(data7.data[2]);
    }

    if (c.exmax >= 0x80000001)
    {
        cpu_data data81;
        cpuid(data81.data, 0x80000001);
        f_81_ECX = static_cast<u32>(data81.data[2]);
        f_81_EDX = static_cast<u32>(data81.data[3]);
    }

    if (c.exmax >= 0x80000004)
    {
        cpu_data data82;
        cpu_data data83;
        cpu_data data84;
        cpuid(data82.data, 0x80000002);
        cpuid(data83.data, 0x80000003);
        cpuid(data84.data, 0x80000004);
        memcpy(c.model, data82.data, sizeof(cpu_data));
        memcpy(c.model + 16, data83.data, sizeof(cpu_data));
        memcpy(c.model + 32, data84.data, sizeof(cpu_data));
    }

    c.hasSSE3        = f_1_ECX >> 0 & 1;
    c.hasPCLMULQDQ   = f_1_ECX >> 1 & 1;
    c.hasMONITOR     = f_1_ECX >> 3 & 1;
    c.hasSSSE3       = f_1_ECX >> 9 & 1;
    c.hasFMA         = f_1_ECX >> 12 & 1;
    c.hasCMPXCHG16B  = f_1_ECX >> 13 & 1;
    c.hasSSE41       = f_1_ECX >> 19 & 1;
    c.hasSSE42       = f_1_ECX >> 20 & 1;
    c.hasMOVBE       = f_1_ECX >> 22 & 1;
    c.hasPOPCNT      = f_1_ECX >> 23 & 1;
    c.hasAES         = f_1_ECX >> 25 & 1;
    c.hasXSAVE       = f_1_ECX >> 26 & 1;
    c.hasOSXSAVE     = f_1_ECX >> 27 & 1;
    c.hasAVX         = f_1_ECX >> 28 & 1;
    c.hasF16C        = f_1_ECX >> 29 & 1;
    c.hasRDRAND      = f_1_ECX >> 30 & 1;
    c.hasMSR         = f_1_EDX >> 5 & 1;
    c.hasCX8         = f_1_EDX >> 8 & 1;
    c.hasSEP         = f_1_EDX >> 11 & 1;
    c.hasCMOV        = f_1_EDX >> 15 & 1;
    c.hasCLFSH       = f_1_EDX >> 19 & 1;
    c.hasMMX         = f_1_EDX >> 23 & 1;
    c.hasFXSR        = f_1_EDX >> 24 & 1;
    c.hasSSE         = f_1_EDX >> 25 & 1;
    c.hasSSE2        = f_1_EDX >> 26 & 1;
    c.hasFSGSBASE    = f_7_EBX >> 0 & 1;
    c.hasBMI1        = f_7_EBX >> 3 & 1;
    c.hasHLE         = c.isIntel && f_7_EBX >> 4 & 1;
    c.hasAVX2        = f_7_EBX >> 5 & 1;
    c.hasBMI2        = f_7_EBX >> 8 & 1;
    c.hasERMS        = f_7_EBX >> 9 & 1;
    c.hasINVPCID     = f_7_EBX >> 10 & 1;
    c.hasRTM         = c.isIntel && f_7_EBX >> 11 & 1;
    c.hasAVX512F     = f_7_EBX >> 16 & 1;
    c.hasAVX512DQ    = f_7_EBX >> 17 & 1;
    c.hasRDSEED      = f_7_EBX >> 18 & 1;
    c.hasADX         = f_7_EBX >> 19 & 1;
    c.hasAVX512PF    = f_7_EBX >> 26 & 1;
    c.hasAVX512ER    = f_7_EBX >> 27 & 1;
    c.hasAVX512CD    = f_7_EBX >> 28 & 1;
    c.hasSHA         = f_7_EBX >> 29 & 1;
    c.hasAVX512BW    = f_7_EBX >> 30 & 1;
    c.hasAVX512VL    = f_7_EBX >> 31 & 1;
    c.hasPREFETCHWT1 = f_7_ECX >> 0 & 1;
    c.hasLAHF        = f_81_ECX >> 0 & 1;
    c.hasLZCNT       = c.isIntel && f_81_ECX >> 5 & 1;
    c.hasABM         = c.isAMD && f_81_ECX >> 5 & 1;
    c.hasSSE4a       = c.isAMD && f_81_ECX >> 6 & 1;
    c.hasXOP         = c.isAMD && f_81_ECX >> 11 & 1;
    c.hasTBM         = c.isAMD && f_81_ECX >> 21 & 1;
    c.hasSYSCALL     = c.isIntel && f_81_EDX >> 11 & 1;
    c.hasMMXEXT      = c.isAMD && f_81_EDX >> 22 & 1;
    c.hasRDTSCP      = c.isIntel && f_81_EDX >> 27 & 1;
    c.has3DNOWEXT    = c.isAMD && f_81_EDX >> 30 & 1;
    c.has3DNOW       = c.isAMD && f_81_EDX >> 31 & 1;

    c.hasAVXOSSUPPORT    = c.hasAVX && c.hasOSXSAVE && (get_xcr0() & 0x06) == 0x06;
    c.hasAVX512OSSUPPORT = c.hasAVXOSSUPPORT && c.hasAVX512F && c.hasOSXSAVE && (get_xcr0() & 0xE0) == 0xE0;

    if (c.hasAVX512F && c.hasAVX512CD && c.hasAVX512VL && c.hasAVX512BW && c.hasAVX512DQ &&
        c.hasAVX512OSSUPPORT)
        return cpu_t::avx512;
    if (c.hasAVX2 && c.hasAVXOSSUPPORT)
        return cpu_t::avx2;
    if (c.hasAVX && c.hasAVXOSSUPPORT)
        return cpu_t::avx1;
    if (c.hasSSE42)
        return cpu_t::sse42;
    if (c.hasSSE41)
        return cpu_t::sse41;
    if (c.hasSSSE3)
        return cpu_t::ssse3;
    if (c.hasSSE3)
        return cpu_t::sse3;
    if (c.hasSSE2)
        return cpu_t::sse2;
    return cpu_t::lowest;
}
} // namespace internal_generic
#else

template <size_t = 0>
cpu_t detect_cpu()
{
    return cpu_t::native;
}

#endif
} // namespace kfr
