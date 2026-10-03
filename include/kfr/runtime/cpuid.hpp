/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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
#if defined(KFR_ARCH_X86) && !defined(__wasm)
/**
 * @brief Detected x86 CPU capabilities and identifying strings.
 *
 * Populated by @ref internal_generic::detect_cpu from CPUID leaves 0, 1, 7 and
 * the extended leaves 0x80000000–0x80000004. Each `has*` bitfield reports
 * whether the corresponding instruction-set extension is available, and the
 * `*OSSUPPORT` flags additionally require the OS to have enabled the relevant
 * XCR0 bits (so AVX/AVX-512 state is actually usable from user mode).
 */
struct cpu_features
{
    u32 max; ///< Highest supported standard CPUID leaf (leaf 0 EAX).
    u32 exmax; ///< Highest supported extended CPUID leaf (leaf 0x80000000 EAX).
    u32 isIntel : 1; ///< Vendor string is "GenuineIntel".
    u32 isAMD : 1; ///< Vendor string is "AuthenticAMD".
    u32 has3DNOW : 1; ///< AMD 3DNow! instructions.
    u32 has3DNOWEXT : 1; ///< AMD 3DNow! extension instructions.
    u32 hasABM : 1; ///< AMD Advanced Bit Manipulation (LZCNT + POPCNT on AMD).
    u32 hasADX : 1; ///< ADX instruction set (arbitrary-precision add with carry).
    u32 hasAES : 1; ///< AES instruction set.
    u32 hasAVX : 1; ///< AVX instruction set (CPUID-reported, see hasAVXOSSUPPORT).
    u32 hasAVX2 : 1; ///< AVX2 instruction set.
    u32 hasAVXOSSUPPORT : 1; ///< AVX state enabled by the OS (XCR0[2:1] == 0b11).
    u32 hasAVX512OSSUPPORT : 1; ///< AVX-512 state enabled by the OS (XCR0[7:5] == 0b111).
    u32 hasAVX512CD : 1; ///< AVX-512 Conflict Detection.
    u32 hasAVX512ER : 1; ///< AVX-512 Exponential and Reciprocal instructions.
    u32 hasAVX512F : 1; ///< AVX-512 Foundation.
    u32 hasAVX512DQ : 1; ///< AVX-512 Doubleword and Quadword.
    u32 hasAVX512PF : 1; ///< AVX-512 Prefetch.
    u32 hasAVX512BW : 1; ///< AVX-512 Byte and Word.
    u32 hasAVX512VL : 1; ///< AVX-512 Vector Length extensions.
    u32 hasBMI1 : 1; ///< Bit Manipulation Instructions 1.
    u32 hasBMI2 : 1; ///< Bit Manipulation Instructions 2.
    u32 hasCLFSH : 1; ///< CLFLUSH instruction.
    u32 hasCMOV : 1; ///< CMOVcc conditional move.
    u32 hasCMPXCHG16B : 1; ///< CMPXCHG16B (128-bit compare-exchange).
    u32 hasCX8 : 1; ///< CMPXCHG8B (64-bit compare-exchange).
    u32 hasERMS : 1; ///< Enhanced REP MOVSB/STOSB.
    u32 hasF16C : 1; ///< F16C half-precision conversion.
    u32 hasFMA : 1; ///< Fused Multiply-Add (FMA3).
    u32 hasFSGSBASE : 1; ///< RDFSBASE/WRFSBASE instructions.
    u32 hasFXSR : 1; ///< FXSAVE/FXRSTOR.
    u32 hasHLE : 1; ///< Hardware Lock Elision (Intel only).
    u32 hasINVPCID : 1; ///< INVPCID instruction.
    u32 hasLAHF : 1; ///< LAHF/SAHF in 64-bit mode.
    u32 hasLZCNT : 1; ///< LZCNT instruction (Intel only).
    u32 hasMMX : 1; ///< MMX.
    u32 hasMMXEXT : 1; ///< AMD MMX extensions.
    u32 hasMONITOR : 1; ///< MONITOR/MWAIT.
    u32 hasMOVBE : 1; ///< MOVBE (move byte-swap).
    u32 hasMSR : 1; ///< Model-Specific Registers (RDMSR/WRMSR).
    u32 hasOSXSAVE : 1; ///< OS enables XSAVE/XRSTOR (CPUID.1:ECX[27]).
    u32 hasPCLMULQDQ : 1; ///< PCLMULQDQ carry-less multiplication.
    u32 hasPOPCNT : 1; ///< POPCNT instruction.
    u32 hasPREFETCHWT1 : 1; ///< PREFETCHWT1 (Intel Xeon Phi).
    u32 hasRDRAND : 1; ///< RDRAND random number.
    u32 hasRDSEED : 1; ///< RDSEED random seed.
    u32 hasRDTSCP : 1; ///< RDTSCP instruction (Intel only).
    u32 hasRTM : 1; ///< Restricted Transactional Memory (Intel only).
    u32 hasSEP : 1; ///< SYSENTER/SYSEXIT.
    u32 hasSHA : 1; ///< SHA instruction set.
    u32 hasSSE : 1; ///< SSE.
    u32 hasSSE2 : 1; ///< SSE2.
    u32 hasSSE3 : 1; ///< SSE3.
    u32 hasSSE41 : 1; ///< SSE4.1.
    u32 hasSSE42 : 1; ///< SSE4.2.
    u32 hasSSE4a : 1; ///< AMD SSE4a.
    u32 hasSSSE3 : 1; ///< SSSE3.
    u32 hasSYSCALL : 1; ///< SYSCALL/SYSRET (Intel only).
    u32 hasTBM : 1; ///< AMD Trailing Bit Manipulation.
    u32 hasXOP : 1; ///< AMD XOP.
    u32 hasXSAVE : 1; ///< XSAVE/XRSTOR.
    u32 padding1 : 6;
    alignas(int32_t) char vendor[17]; ///< NUL-terminated CPUID vendor string (leaf 0).
    alignas(int32_t) char model[49]; ///< NUL-terminated CPUID model/brand string (leaves 0x80000002–4).
    alignas(int32_t) char padding2[2];
};

namespace internal_generic
{

struct cpu_data
{
    u32 data[4];
};

#if defined KFR_COMPILER_GNU || defined KFR_COMPILER_CLANG
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
#elif defined KFR_COMPILER_MSVC

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

namespace internal_generic
{

template <size_t = 0>
cpu_t detect_cpu()
{
    return cpu_t::native;
}
} // namespace internal_generic

#endif
} // namespace kfr
