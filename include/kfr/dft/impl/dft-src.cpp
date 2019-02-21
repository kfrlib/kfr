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

#include "../dft_c.h"
#include "../fft.hpp"

namespace kfr
{

extern "C"
{

    KFR_DFT_PLAN_F32* kfr_dft_create_plan_f32(size_t size)
    {
        return reinterpret_cast<KFR_DFT_PLAN_F32*>(new kfr::dft_plan<float>(size));
    }
    KFR_DFT_PLAN_F64* kfr_dft_create_plan_f64(size_t size)
    {
        return reinterpret_cast<KFR_DFT_PLAN_F64*>(new kfr::dft_plan<double>(size));
    }

    void kfr_dft_execute_f32(KFR_DFT_PLAN_F32* plan, size_t, float* out, const float* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan<float>*>(plan)->execute(
            reinterpret_cast<kfr::complex<float>*>(out), reinterpret_cast<const kfr::complex<float>*>(in),
            temp, kfr::cfalse);
    }
    void kfr_dft_execute_f64(KFR_DFT_PLAN_F64* plan, size_t, double* out, const double* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan<double>*>(plan)->execute(
            reinterpret_cast<kfr::complex<double>*>(out), reinterpret_cast<const kfr::complex<double>*>(in),
            temp, kfr::cfalse);
    }
    void kfr_dft_execute_inverse_f32(KFR_DFT_PLAN_F32* plan, size_t, float* out, const float* in,
                                     uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan<float>*>(plan)->execute(
            reinterpret_cast<kfr::complex<float>*>(out), reinterpret_cast<const kfr::complex<float>*>(in),
            temp, kfr::ctrue);
    }
    void kfr_dft_execute_inverse_f64(KFR_DFT_PLAN_F64* plan, size_t, double* out, const double* in,
                                     uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan<double>*>(plan)->execute(
            reinterpret_cast<kfr::complex<double>*>(out), reinterpret_cast<const kfr::complex<double>*>(in),
            temp, kfr::ctrue);
    }

    void kfr_dft_delete_plan_f32(KFR_DFT_PLAN_F32* plan)
    {
        delete reinterpret_cast<kfr::dft_plan<float>*>(plan);
    }
    void kfr_dft_delete_plan_f64(KFR_DFT_PLAN_F64* plan)
    {
        delete reinterpret_cast<kfr::dft_plan<double>*>(plan);
    }

    // Real DFT plans

    KFR_DFT_REAL_PLAN_F32* kfr_dft_create_real_plan_f32(size_t size)
    {
        return reinterpret_cast<KFR_DFT_REAL_PLAN_F32*>(new kfr::dft_plan_real<float>(size));
    }
    KFR_DFT_REAL_PLAN_F64* kfr_dft_create_real_plan_f64(size_t size)
    {
        return reinterpret_cast<KFR_DFT_REAL_PLAN_F64*>(new kfr::dft_plan_real<double>(size));
    }

    void kfr_dft_execute_real_f32(KFR_DFT_REAL_PLAN_F32* plan, size_t, float* out, const float* in,
                                  uint8_t* temp, KFR_DFT_PACK_FORMAT pack_format)
    {
        reinterpret_cast<kfr::dft_plan_real<float>*>(plan)->execute(
            reinterpret_cast<kfr::complex<float>*>(out), in, temp,
            static_cast<kfr::dft_pack_format>(pack_format));
    }
    void kfr_dft_execute_real_f64(KFR_DFT_REAL_PLAN_F64* plan, size_t, double* out, const double* in,
                                  uint8_t* temp, KFR_DFT_PACK_FORMAT pack_format)
    {
        reinterpret_cast<kfr::dft_plan_real<double>*>(plan)->execute(
            reinterpret_cast<kfr::complex<double>*>(out), in, temp,
            static_cast<kfr::dft_pack_format>(pack_format));
    }
    void kfr_dft_execute_real_inverse_f32(KFR_DFT_REAL_PLAN_F32* plan, size_t, float* out, const float* in,
                                          uint8_t* temp, KFR_DFT_PACK_FORMAT pack_format)
    {
        reinterpret_cast<kfr::dft_plan_real<float>*>(plan)->execute(
            out, reinterpret_cast<const kfr::complex<float>*>(in), temp,
            static_cast<kfr::dft_pack_format>(pack_format));
    }
    void kfr_dft_execute_real_inverse__f64(KFR_DFT_REAL_PLAN_F64* plan, size_t, double* out, const double* in,
                                           uint8_t* temp, KFR_DFT_PACK_FORMAT pack_format)
    {
        reinterpret_cast<kfr::dft_plan_real<double>*>(plan)->execute(
            out, reinterpret_cast<const kfr::complex<double>*>(in), temp,
            static_cast<kfr::dft_pack_format>(pack_format));
    }

    void kfr_dft_delete_real_plan_f32(KFR_DFT_REAL_PLAN_F32* plan)
    {
        delete reinterpret_cast<kfr::dft_plan_real<float>*>(plan);
    }
    void kfr_dft_delete_real_plan_f64(KFR_DFT_REAL_PLAN_F64* plan)
    {
        delete reinterpret_cast<kfr::dft_plan_real<double>*>(plan);
    }
}
} // namespace kfr
