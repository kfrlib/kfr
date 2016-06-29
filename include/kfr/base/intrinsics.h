#pragma once

#include "kfr.h"

#if KFR_COMPILER_CLANG

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"

#ifdef __AVX2__
#define KFR_AVX2_DEFINED
#endif
#ifdef __AVX__
#define KFR_AVX1_DEFINED
#endif
#ifdef __SSE4_2__
#define KFR_SSE42_DEFINED
#endif
#ifdef __SSE4_1__
#define KFR_SSE41_DEFINED
#endif
#ifdef __SSSE3__
#define KFR_SSSE3_DEFINED
#endif
#ifdef __SSE3__
#define KFR_SSE3_DEFINED
#endif
#ifdef __SSE2__
#define KFR_SSE2_DEFINED
#endif
#ifdef __SSE__
#define KFR_SSE1_DEFINED
#endif
#ifdef __MMX__
#define KFR_MMX_DEFINED
#endif

#ifndef KFR_AVX2_DEFINED
#define __AVX2__
#endif
#ifndef KFR_AVX1_DEFINED
#define __AVX__
#endif
#ifndef KFR_SSE42_DEFINED
#define __SSE4_2__
#endif
#ifndef KFR_SSE41_DEFINED
#define __SSE4_1__
#endif
#ifndef KFR_SSSE3_DEFINED
#define __SSSE3__
#endif
#ifndef KFR_SSE3_DEFINED
#define __SSE3__
#endif
#ifndef KFR_SSE2_DEFINED
#define __SSE2__
#endif
#ifndef KFR_SSE1_DEFINED
#define __SSE__
#endif
#ifndef KFR_MMX_DEFINED
#define __MMX__
#endif

#ifdef KFR_SKIP_AVX512
#ifndef __AVX512FINTRIN_H
#define __AVX512FINTRIN_H
#endif
#ifndef __AVX512VLINTRIN_H
#define __AVX512VLINTRIN_H
#endif
#ifndef __AVX512BWINTRIN_H
#define __AVX512BWINTRIN_H
#endif
#ifndef __AVX512CDINTRIN_H
#define __AVX512CDINTRIN_H
#endif
#ifndef __AVX512DQINTRIN_H
#define __AVX512DQINTRIN_H
#endif
#ifndef __AVX512VLBWINTRIN_H
#define __AVX512VLBWINTRIN_H
#endif
#ifndef __AVX512VLDQINTRIN_H
#define __AVX512VLDQINTRIN_H
#endif
#ifndef __AVX512ERINTRIN_H
#define __AVX512ERINTRIN_H
#endif
#ifndef __IFMAINTRIN_H
#define __IFMAINTRIN_H
#endif
#ifndef __IFMAVLINTRIN_H
#define __IFMAVLINTRIN_H
#endif
#ifndef __VBMIINTRIN_H
#define __VBMIINTRIN_H
#endif
#ifndef __VBMIVLINTRIN_H
#define __VBMIVLINTRIN_H
#endif

#endif

#include <immintrin.h>
#ifdef KFR_OS_WIN
#include <intrin.h>
#endif

#ifndef KFR_AVX2_DEFINED
#undef __AVX2__
#endif
#ifndef KFR_AVX1_DEFINED
#undef __AVX__
#endif
#ifndef KFR_SSE42_DEFINED
#undef __SSE4_2__
#endif
#ifndef KFR_SSE41_DEFINED
#undef __SSE4_1__
#endif
#ifndef KFR_SSSE3_DEFINED
#undef __SSSE3__
#endif
#ifndef KFR_SSE3_DEFINED
#undef __SSE3__
#endif
#ifndef KFR_SSE2_DEFINED
#undef __SSE2__
#endif
#ifndef KFR_SSE1_DEFINED
#undef __SSE__
#endif
#ifndef KFR_MMX_DEFINED
#undef __MMX__
#endif

#pragma clang diagnostic pop

#else

#include <intrin.h>

#endif
