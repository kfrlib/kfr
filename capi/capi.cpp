/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
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
#define KFR_NO_C_COMPLEX_TYPES 1

#include <kfr/capi.h>
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>

namespace kfr
{

extern "C"
{
#define KFR_ENABLED_ARCHS "sse2,sse3,ssse3,sse4.1,avx,avx2,avx512"
    const char* kfr_version_string()
    {
        return "KFR " KFR_VERSION_STRING KFR_DEBUG_STR " " KFR_ENABLED_ARCHS " " CMT_ARCH_BITNESS_NAME
               " (" CMT_COMPILER_FULL_NAME "/" CMT_OS_NAME ")" KFR_BUILD_DETAILS_1 KFR_BUILD_DETAILS_2;
    }
    uint32_t kfr_version() { return KFR_VERSION; }
    const char* kfr_enabled_archs() { return KFR_ENABLED_ARCHS; }
    int kfr_current_arch() { return static_cast<int>(get_cpu()); }

    void* kfr_allocate(size_t size) { return details::aligned_malloc(size, KFR_DEFAULT_ALIGNMENT); }
    void* kfr_allocate_aligned(size_t size, size_t alignment)
    {
        return details::aligned_malloc(size, alignment);
    }
    void kfr_deallocate(void* ptr) { return details::aligned_free(ptr); }
    size_t kfr_allocated_size(void* ptr) { return details::aligned_size(ptr); }

    void* kfr_add_ref(void* ptr)
    {
        details::aligned_add_ref(ptr);
        return ptr;
    }
    void kfr_release(void* ptr) { details::aligned_release(ptr); }

    void* kfr_reallocate(void* ptr, size_t new_size)
    {
        return details::aligned_reallocate(ptr, new_size, KFR_DEFAULT_ALIGNMENT);
    }
    void* kfr_reallocate_aligned(void* ptr, size_t new_size, size_t alignment)
    {
        return details::aligned_reallocate(ptr, new_size, alignment);
    }

    KFR_DFT_PLAN_F32* kfr_dft_create_plan_f32(size_t size)
    {
        if (size < 2)
            return nullptr;
        if (size > 16777216)
            return nullptr;
        return reinterpret_cast<KFR_DFT_PLAN_F32*>(new kfr::dft_plan<float>(cpu_t::runtime, size));
    }
    KFR_DFT_PLAN_F64* kfr_dft_create_plan_f64(size_t size)
    {
        if (size < 2)
            return nullptr;
        if (size > 16777216)
            return nullptr;
        return reinterpret_cast<KFR_DFT_PLAN_F64*>(new kfr::dft_plan<double>(cpu_t::runtime, size));
    }

    void kfr_dft_dump_f32(KFR_DFT_PLAN_F32* plan) { reinterpret_cast<kfr::dft_plan<float>*>(plan)->dump(); }
    void kfr_dft_dump_f64(KFR_DFT_PLAN_F64* plan) { reinterpret_cast<kfr::dft_plan<double>*>(plan)->dump(); }

    size_t kfr_dft_get_size_f32(KFR_DFT_PLAN_F32* plan)
    {
        return reinterpret_cast<kfr::dft_plan<float>*>(plan)->size;
    }
    size_t kfr_dft_get_size_f64(KFR_DFT_PLAN_F64* plan)
    {
        return reinterpret_cast<kfr::dft_plan<double>*>(plan)->size;
    }

    size_t kfr_dft_get_temp_size_f32(KFR_DFT_PLAN_F32* plan)
    {
        return reinterpret_cast<kfr::dft_plan<float>*>(plan)->temp_size;
    }
    size_t kfr_dft_get_temp_size_f64(KFR_DFT_PLAN_F64* plan)
    {
        return reinterpret_cast<kfr::dft_plan<double>*>(plan)->temp_size;
    }

    void kfr_dft_execute_f32(KFR_DFT_PLAN_F32* plan, kfr_c32* out, const kfr_c32* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan<float>*>(plan)->execute(
            reinterpret_cast<kfr::complex<float>*>(out), reinterpret_cast<const kfr::complex<float>*>(in),
            temp, kfr::cfalse);
    }
    void kfr_dft_execute_f64(KFR_DFT_PLAN_F64* plan, kfr_c64* out, const kfr_c64* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan<double>*>(plan)->execute(
            reinterpret_cast<kfr::complex<double>*>(out), reinterpret_cast<const kfr::complex<double>*>(in),
            temp, kfr::cfalse);
    }
    void kfr_dft_execute_inverse_f32(KFR_DFT_PLAN_F32* plan, kfr_c32* out, const kfr_c32* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan<float>*>(plan)->execute(
            reinterpret_cast<kfr::complex<float>*>(out), reinterpret_cast<const kfr::complex<float>*>(in),
            temp, kfr::ctrue);
    }
    void kfr_dft_execute_inverse_f64(KFR_DFT_PLAN_F64* plan, kfr_c64* out, const kfr_c64* in, uint8_t* temp)
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

    KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_plan_f32(size_t size, KFR_DFT_PACK_FORMAT pack_format)
    {
        if (size < 4)
            return nullptr;
        if (size > 16777216)
            return nullptr;
        return reinterpret_cast<KFR_DFT_REAL_PLAN_F32*>(
            new kfr::dft_plan_real<float>(cpu_t::runtime, size, static_cast<dft_pack_format>(pack_format)));
    }
    KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_plan_f64(size_t size, KFR_DFT_PACK_FORMAT pack_format)
    {
        if (size < 4)
            return nullptr;
        if (size > 16777216)
            return nullptr;
        return reinterpret_cast<KFR_DFT_REAL_PLAN_F64*>(
            new kfr::dft_plan_real<double>(cpu_t::runtime, size, static_cast<dft_pack_format>(pack_format)));
    }

    void kfr_dft_real_dump_f32(KFR_DFT_REAL_PLAN_F32* plan)
    {
        reinterpret_cast<kfr::dft_plan_real<float>*>(plan)->dump();
    }
    void kfr_dft_real_dump_f64(KFR_DFT_REAL_PLAN_F64* plan)
    {
        reinterpret_cast<kfr::dft_plan_real<double>*>(plan)->dump();
    }

    size_t kfr_dft_real_get_size_f32(KFR_DFT_REAL_PLAN_F32* plan)
    {
        return reinterpret_cast<kfr::dft_plan<float>*>(plan)->size;
    }
    size_t kfr_dft_real_get_size_f64(KFR_DFT_REAL_PLAN_F64* plan)
    {
        return reinterpret_cast<kfr::dft_plan<double>*>(plan)->size;
    }

    size_t kfr_dft_real_get_temp_size_f32(KFR_DFT_REAL_PLAN_F32* plan)
    {
        return reinterpret_cast<kfr::dft_plan<float>*>(plan)->temp_size;
    }
    size_t kfr_dft_real_get_temp_size_f64(KFR_DFT_REAL_PLAN_F64* plan)
    {
        return reinterpret_cast<kfr::dft_plan<double>*>(plan)->temp_size;
    }

    void kfr_dft_real_execute_f32(KFR_DFT_REAL_PLAN_F32* plan, kfr_c32* out, const float* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan_real<float>*>(plan)->execute(
            reinterpret_cast<kfr::complex<float>*>(out), in, temp);
    }
    void kfr_dft_real_execute_f64(KFR_DFT_REAL_PLAN_F64* plan, kfr_c64* out, const double* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan_real<double>*>(plan)->execute(
            reinterpret_cast<kfr::complex<double>*>(out), in, temp);
    }
    void kfr_dft_real_execute_inverse_f32(KFR_DFT_REAL_PLAN_F32* plan, float* out, const kfr_c32* in,
                                          uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan_real<float>*>(plan)->execute(
            out, reinterpret_cast<const kfr::complex<float>*>(in), temp);
    }
    void kfr_dft_real_execute_inverse_f64(KFR_DFT_REAL_PLAN_F64* plan, double* out, const kfr_c64* in,
                                          uint8_t* temp)
    {
        reinterpret_cast<kfr::dft_plan_real<double>*>(plan)->execute(
            out, reinterpret_cast<const kfr::complex<double>*>(in), temp);
    }

    void kfr_dft_real_delete_plan_f32(KFR_DFT_REAL_PLAN_F32* plan)
    {
        delete reinterpret_cast<kfr::dft_plan_real<float>*>(plan);
    }
    void kfr_dft_real_delete_plan_f64(KFR_DFT_REAL_PLAN_F64* plan)
    {
        delete reinterpret_cast<kfr::dft_plan_real<double>*>(plan);
    }

    // Discrete Cosine Transform

    KFR_DCT_PLAN_F32* kfr_dct_create_plan_f32(size_t size)
    {
        if (size < 4)
            return nullptr;
        if (size > 16777216)
            return nullptr;
        return reinterpret_cast<KFR_DCT_PLAN_F32*>(new kfr::dct_plan<float>(cpu_t::runtime, size));
    }
    KFR_DCT_PLAN_F64* kfr_dct_create_plan_f64(size_t size)
    {
        if (size < 4)
            return nullptr;
        if (size > 16777216)
            return nullptr;
        return reinterpret_cast<KFR_DCT_PLAN_F64*>(new kfr::dct_plan<double>(cpu_t::runtime, size));
    }

    void kfr_dct_dump_f32(KFR_DCT_PLAN_F32* plan) { reinterpret_cast<kfr::dct_plan<float>*>(plan)->dump(); }
    void kfr_dct_dump_f64(KFR_DCT_PLAN_F64* plan) { reinterpret_cast<kfr::dct_plan<double>*>(plan)->dump(); }

    size_t kfr_dct_get_size_f32(KFR_DCT_PLAN_F32* plan)
    {
        return reinterpret_cast<kfr::dft_plan<float>*>(plan)->size;
    }
    size_t kfr_dct_get_size_f64(KFR_DCT_PLAN_F64* plan)
    {
        return reinterpret_cast<kfr::dft_plan<double>*>(plan)->size;
    }

    size_t kfr_dct_get_temp_size_f32(KFR_DCT_PLAN_F32* plan)
    {
        return reinterpret_cast<kfr::dft_plan<float>*>(plan)->temp_size;
    }
    size_t kfr_dct_get_temp_size_f64(KFR_DCT_PLAN_F64* plan)
    {
        return reinterpret_cast<kfr::dft_plan<double>*>(plan)->temp_size;
    }

    void kfr_dct_execute_f32(KFR_DCT_PLAN_F32* plan, float* out, const float* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dct_plan<float>*>(plan)->execute(out, in, temp);
    }
    void kfr_dct_execute_f64(KFR_DCT_PLAN_F64* plan, double* out, const double* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dct_plan<double>*>(plan)->execute(out, in, temp);
    }
    void kfr_dct_execute_inverse_f32(KFR_DCT_PLAN_F32* plan, float* out, const float* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dct_plan<float>*>(plan)->execute(out, in, temp);
    }
    void kfr_dct_execute_inverse_f64(KFR_DCT_PLAN_F64* plan, double* out, const double* in, uint8_t* temp)
    {
        reinterpret_cast<kfr::dct_plan<double>*>(plan)->execute(out, in, temp);
    }

    void kfr_dct_delete_plan_f32(KFR_DCT_PLAN_F32* plan)
    {
        delete reinterpret_cast<kfr::dct_plan<float>*>(plan);
    }
    void kfr_dct_delete_plan_f64(KFR_DCT_PLAN_F64* plan)
    {
        delete reinterpret_cast<kfr::dct_plan<double>*>(plan);
    }

    // Filters

    KFR_FILTER_F32* kfr_filter_create_fir_plan_f32(const kfr_f32* taps, size_t size)
    {
        return reinterpret_cast<KFR_FILTER_F32*>(
            make_fir_filter<float>(cpu_t::runtime, make_univector(taps, size)));
    }
    KFR_FILTER_F64* kfr_filter_create_fir_plan_f64(const kfr_f64* taps, size_t size)
    {
        return reinterpret_cast<KFR_FILTER_F64*>(
            make_fir_filter<double>(cpu_t::runtime, make_univector(taps, size)));
    }

    KFR_FILTER_F32* kfr_filter_create_convolution_plan_f32(const kfr_f32* taps, size_t size,
                                                           size_t block_size)
    {
        return reinterpret_cast<KFR_FILTER_F32*>(make_convolve_filter<float>(
            cpu_t::runtime, make_univector(taps, size), block_size ? block_size : 1024));
    }
    KFR_FILTER_F64* kfr_filter_create_convolution_plan_f64(const kfr_f64* taps, size_t size,
                                                           size_t block_size)
    {
        return reinterpret_cast<KFR_FILTER_F64*>(make_convolve_filter<double>(
            cpu_t::runtime, make_univector(taps, size), block_size ? block_size : 1024));
    }

    KFR_FILTER_F32* kfr_filter_create_iir_plan_f32(const kfr_f32* sos, size_t sos_count)
    {
        if (sos_count < 1 || sos_count > 64)
            return nullptr;
        return reinterpret_cast<KFR_FILTER_F32*>(make_biquad_filter<float, 64>(
            cpu_t::runtime, reinterpret_cast<const biquad_params<float>*>(sos), sos_count));
    }
    KFR_FILTER_F64* kfr_filter_create_iir_plan_f64(const kfr_f64* sos, size_t sos_count)
    {
        if (sos_count < 1 || sos_count > 64)
            return nullptr;
        return reinterpret_cast<KFR_FILTER_F64*>(make_biquad_filter<double, 64>(
            cpu_t::runtime, reinterpret_cast<const biquad_params<double>*>(sos), sos_count));
    }

    void kfr_filter_process_f32(KFR_FILTER_F32* plan, kfr_f32* output, const kfr_f32* input, size_t size)
    {
        reinterpret_cast<filter<float>*>(plan)->apply(output, input, size);
    }
    void kfr_filter_process_f64(KFR_FILTER_F64* plan, kfr_f64* output, const kfr_f64* input, size_t size)
    {
        reinterpret_cast<filter<double>*>(plan)->apply(output, input, size);
    }

    void kfr_filter_reset_f32(KFR_FILTER_F32* plan) { reinterpret_cast<filter<float>*>(plan)->reset(); }
    void kfr_filter_reset_f64(KFR_FILTER_F64* plan) { reinterpret_cast<filter<double>*>(plan)->reset(); }

    void kfr_filter_delete_plan_f32(KFR_FILTER_F32* plan) { delete reinterpret_cast<filter<f32>*>(plan); }
    void kfr_filter_delete_plan_f64(KFR_FILTER_F64* plan) { delete reinterpret_cast<filter<f64>*>(plan); }
}

} // namespace kfr
