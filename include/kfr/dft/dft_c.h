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

    enum KFR_DFT_PACK_FORMAT
    {
        Perm = 0,
        CCs  = 1
    };

    // Complex DFT plans

    KFR_DFT_PLAN_F32* kfr_dft_create_plan_f32(size_t size);
    KFR_DFT_PLAN_F64* kfr_dft_create_plan_f64(size_t size);

    void kfr_dft_execute_f32(KFR_DFT_PLAN_F32* plan, size_t size, float* out, const float* in, uint8_t* temp);
    void kfr_dft_execute_f64(KFR_DFT_PLAN_F64* plan, size_t size, double* out, const double* in,
                             uint8_t* temp);

    void kfr_dft_execute_inverse_f32(KFR_DFT_PLAN_F32* plan, size_t size, float* out, const float* in,
                                     uint8_t* temp);
    void kfr_dft_execute_inverse_f64(KFR_DFT_PLAN_F64* plan, size_t size, double* out, const double* in,
                                     uint8_t* temp);

    void kfr_dft_delete_plan_f32(KFR_DFT_PLAN_F32* plan);
    void kfr_dft_delete_plan_f64(KFR_DFT_PLAN_F64* plan);

    // Real DFT plans

    KFR_DFT_REAL_PLAN_F32* kfr_dft_create_real_plan_f32(size_t size);
    KFR_DFT_REAL_PLAN_F64* kfr_dft_create_real_plan_f64(size_t size);

    void kfr_dft_execute_real_f32(KFR_DFT_REAL_PLAN_F32* plan, size_t size, float* out, const float* in,
                                  uint8_t* temp, KFR_DFT_PACK_FORMAT pack_format);
    void kfr_dft_execute_real_f64(KFR_DFT_REAL_PLAN_F64* plan, size_t size, double* out, const double* in,
                                  uint8_t* temp, KFR_DFT_PACK_FORMAT pack_format);

    void kfr_dft_execute_real_inverse_f32(KFR_DFT_REAL_PLAN_F32* plan, size_t size, float* out,
                                          const float* in, uint8_t* temp, KFR_DFT_PACK_FORMAT pack_format);
    void kfr_dft_execute_real_inverse_f64(KFR_DFT_REAL_PLAN_F64* plan, size_t size, double* out,
                                          const double* in, uint8_t* temp, KFR_DFT_PACK_FORMAT pack_format);

    void kfr_dft_delete_real_plan_f32(KFR_DFT_REAL_PLAN_F32* plan);
    void kfr_dft_delete_real_plan_f64(KFR_DFT_REAL_PLAN_F64* plan);

#ifdef __cplusplus
}
#endif
