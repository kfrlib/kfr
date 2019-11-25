/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include <stddef.h>
#include <stdint.h>

#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__)
#define KFR_ARCH_IS_X86 1
#elif defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(__aarch64__)
#define KFR_ARCH_IS_ARM 1
#endif

#if defined(_M_X64) || defined(__x86_64__)
#define KFR_CDECL
#else
#ifdef _WIN32
#define KFR_CDECL __cdecl
#elif defined KFR_ARCH_IS_X86
#define KFR_CDECL __attribute__((__cdecl__))
#else
#define KFR_CDECL
#endif
#endif

#ifdef _WIN32
#ifdef KFR_BUILDING_DLL
#define KFR_API_SPEC KFR_CDECL __declspec(dllexport)
#else
#define KFR_API_SPEC KFR_CDECL __declspec(dllimport)
#endif
#else
#define KFR_API_SPEC KFR_CDECL
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct KFR_DFT_PLAN_F32
    {
        size_t size;
        size_t temp_size;
    } KFR_DFT_PLAN_F32;
    typedef struct KFR_DFT_PLAN_F64
    {
        size_t size;
        size_t temp_size;
    } KFR_DFT_PLAN_F64;

    typedef struct KFR_DFT_REAL_PLAN_F32
    {
        size_t dummy;
        size_t temp_size;
        size_t size;
    } KFR_DFT_REAL_PLAN_F32;
    typedef struct KFR_DFT_REAL_PLAN_F64
    {
        size_t dummy;
        size_t temp_size;
        size_t size;
    } KFR_DFT_REAL_PLAN_F64;

    typedef enum KFR_DFT_PACK_FORMAT
    {
        Perm = 0,
        CCs  = 1
    } KFR_DFT_PACK_FORMAT;

    // Complex DFT plans

    KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_plan_f32(size_t size);
    KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_plan_f64(size_t size);

    KFR_API_SPEC void kfr_dft_dump_f32(KFR_DFT_PLAN_F32* plan);
    KFR_API_SPEC void kfr_dft_dump_f64(KFR_DFT_PLAN_F64* plan);

    KFR_API_SPEC void kfr_dft_execute_f32(KFR_DFT_PLAN_F32* plan, float* out, const float* in, uint8_t* temp);
    KFR_API_SPEC void kfr_dft_execute_f64(KFR_DFT_PLAN_F64* plan, double* out, const double* in,
                                          uint8_t* temp);

    KFR_API_SPEC void kfr_dft_execute_inverse_f32(KFR_DFT_PLAN_F32* plan, float* out, const float* in,
                                                  uint8_t* temp);
    KFR_API_SPEC void kfr_dft_execute_inverse_f64(KFR_DFT_PLAN_F64* plan, double* out, const double* in,
                                                  uint8_t* temp);

    KFR_API_SPEC void kfr_dft_delete_plan_f32(KFR_DFT_PLAN_F32* plan);
    KFR_API_SPEC void kfr_dft_delete_plan_f64(KFR_DFT_PLAN_F64* plan);

    // Real DFT plans

    KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_create_real_plan_f32(size_t size,
                                                                     KFR_DFT_PACK_FORMAT pack_format);
    KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_create_real_plan_f64(size_t size,
                                                                     KFR_DFT_PACK_FORMAT pack_format);

    KFR_API_SPEC void kfr_dft_dump_real_f32(KFR_DFT_REAL_PLAN_F32* plan);
    KFR_API_SPEC void kfr_dft_dump_real_f64(KFR_DFT_REAL_PLAN_F64* plan);

    KFR_API_SPEC void kfr_dft_execute_real_f32(KFR_DFT_REAL_PLAN_F32* plan, float* out, const float* in,
                                               uint8_t* temp);
    KFR_API_SPEC void kfr_dft_execute_real_f64(KFR_DFT_REAL_PLAN_F64* plan, double* out, const double* in,
                                               uint8_t* temp);

    KFR_API_SPEC void kfr_dft_execute_real_inverse_f32(KFR_DFT_REAL_PLAN_F32* plan, float* out,
                                                       const float* in, uint8_t* temp);
    KFR_API_SPEC void kfr_dft_execute_real_inverse_f64(KFR_DFT_REAL_PLAN_F64* plan, double* out,
                                                       const double* in, uint8_t* temp);

    KFR_API_SPEC void kfr_dft_delete_real_plan_f32(KFR_DFT_REAL_PLAN_F32* plan);
    KFR_API_SPEC void kfr_dft_delete_real_plan_f64(KFR_DFT_REAL_PLAN_F64* plan);

#ifdef __cplusplus
}
#endif
