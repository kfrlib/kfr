/** @addtogroup dft
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

#include <stddef.h>
#include <stdint.h>
#if defined __STDC_IEC_559_COMPLEX__ && !defined KFR_NO_C_COMPLEX_TYPES
#include <complex.h>
#endif

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

    enum
    {
        KFR_ARCH_X86    = 0,
        KFR_ARCH_SSE2   = 1,
        KFR_ARCH_SSE3   = 2,
        KFR_ARCH_SSSE3  = 3,
        KFR_ARCH_SSE41  = 4,
        KFR_ARCH_SSE42  = 5,
        KFR_ARCH_AVX    = 6,
        KFR_ARCH_AVX2   = 7,
        KFR_ARCH_AVX512 = 8,
    };

#define KFR_HEADERS_VERSION 40200

    KFR_API_SPEC const char* kfr_version_string();
    KFR_API_SPEC uint32_t kfr_version();
    KFR_API_SPEC const char* kfr_enabled_archs();
    KFR_API_SPEC int kfr_current_arch();

    typedef float kfr_f32;
    typedef double kfr_f64;
#if defined __STDC_IEC_559_COMPLEX__ && !defined KFR_NO_C_COMPLEX_TYPES
    typedef float _Complex kfr_c32;
    typedef double _Complex kfr_c64;
#define KFR_COMPLEX_SIZE_MULTIPLIER 1
#else
typedef float kfr_c32;
typedef double kfr_c64;
#define KFR_COMPLEX_SIZE_MULTIPLIER 2
#endif
    typedef size_t kfr_size_t;
    typedef int32_t kfr_int32_t;

#define KFR_OPAQUE_STRUCT(NAME)                                                                              \
    typedef struct NAME                                                                                      \
    {                                                                                                        \
        int opaque;                                                                                          \
    } NAME;

    KFR_OPAQUE_STRUCT(KFR_DFT_PLAN_F32)
    KFR_OPAQUE_STRUCT(KFR_DFT_PLAN_F64)

    KFR_OPAQUE_STRUCT(KFR_DFT_REAL_PLAN_F32)
    KFR_OPAQUE_STRUCT(KFR_DFT_REAL_PLAN_F64)

    KFR_OPAQUE_STRUCT(KFR_DCT_PLAN_F32)
    KFR_OPAQUE_STRUCT(KFR_DCT_PLAN_F64)

    KFR_OPAQUE_STRUCT(KFR_FILTER_F32)
    KFR_OPAQUE_STRUCT(KFR_FILTER_F64)

    KFR_OPAQUE_STRUCT(KFR_FILTER_C32)
    KFR_OPAQUE_STRUCT(KFR_FILTER_C64)

    // Memory allocation

#define KFR_DEFAULT_ALIGNMENT 64

    KFR_API_SPEC void* kfr_allocate(size_t size);
    KFR_API_SPEC void* kfr_allocate_aligned(size_t size, size_t alignment);
    KFR_API_SPEC void* kfr_reallocate(void* ptr, size_t new_size);
    KFR_API_SPEC void* kfr_reallocate_aligned(void* ptr, size_t new_size, size_t alignment);
    KFR_API_SPEC void* kfr_add_ref(void* ptr);
    KFR_API_SPEC void kfr_release(void* ptr);
    KFR_API_SPEC void kfr_deallocate(void* ptr);
    KFR_API_SPEC size_t kfr_allocated_size(void* ptr);

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

    KFR_API_SPEC size_t kfr_dft_get_size_f32(KFR_DFT_PLAN_F32* plan);
    KFR_API_SPEC size_t kfr_dft_get_size_f64(KFR_DFT_PLAN_F64* plan);

    KFR_API_SPEC size_t kfr_dft_get_temp_size_f32(KFR_DFT_PLAN_F32* plan);
    KFR_API_SPEC size_t kfr_dft_get_temp_size_f64(KFR_DFT_PLAN_F64* plan);

    KFR_API_SPEC void kfr_dft_execute_f32(KFR_DFT_PLAN_F32* plan, kfr_c32* out, const kfr_c32* in,
                                          uint8_t* temp);
    KFR_API_SPEC void kfr_dft_execute_f64(KFR_DFT_PLAN_F64* plan, kfr_c64* out, const kfr_c64* in,
                                          uint8_t* temp);

    KFR_API_SPEC void kfr_dft_execute_inverse_f32(KFR_DFT_PLAN_F32* plan, kfr_c32* out, const kfr_c32* in,
                                                  uint8_t* temp);
    KFR_API_SPEC void kfr_dft_execute_inverse_f64(KFR_DFT_PLAN_F64* plan, kfr_c64* out, const kfr_c64* in,
                                                  uint8_t* temp);

    KFR_API_SPEC void kfr_dft_delete_plan_f32(KFR_DFT_PLAN_F32* plan);
    KFR_API_SPEC void kfr_dft_delete_plan_f64(KFR_DFT_PLAN_F64* plan);

    // Real DFT plans

    KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_plan_f32(size_t size,
                                                                     KFR_DFT_PACK_FORMAT pack_format);
    KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_plan_f64(size_t size,
                                                                     KFR_DFT_PACK_FORMAT pack_format);

    KFR_API_SPEC void kfr_dft_real_dump_f32(KFR_DFT_REAL_PLAN_F32* plan);
    KFR_API_SPEC void kfr_dft_real_dump_f64(KFR_DFT_REAL_PLAN_F64* plan);

    KFR_API_SPEC size_t kfr_dft_real_get_size_f32(KFR_DFT_REAL_PLAN_F32* plan);
    KFR_API_SPEC size_t kfr_dft_real_get_size_f64(KFR_DFT_REAL_PLAN_F64* plan);

    KFR_API_SPEC size_t kfr_dft_real_get_temp_size_f32(KFR_DFT_REAL_PLAN_F32* plan);
    KFR_API_SPEC size_t kfr_dft_real_get_temp_size_f64(KFR_DFT_REAL_PLAN_F64* plan);

    KFR_API_SPEC void kfr_dft_real_execute_f32(KFR_DFT_REAL_PLAN_F32* plan, kfr_c32* out, const kfr_f32* in,
                                               uint8_t* temp);
    KFR_API_SPEC void kfr_dft_real_execute_f64(KFR_DFT_REAL_PLAN_F64* plan, kfr_c64* out, const kfr_f64* in,
                                               uint8_t* temp);

    KFR_API_SPEC void kfr_dft_real_execute_inverse_f32(KFR_DFT_REAL_PLAN_F32* plan, kfr_f32* out,
                                                       const kfr_c32* in, uint8_t* temp);
    KFR_API_SPEC void kfr_dft_real_execute_inverse_f64(KFR_DFT_REAL_PLAN_F64* plan, kfr_f64* out,
                                                       const kfr_c64* in, uint8_t* temp);

    KFR_API_SPEC void kfr_dft_real_delete_plan_f32(KFR_DFT_REAL_PLAN_F32* plan);
    KFR_API_SPEC void kfr_dft_real_delete_plan_f64(KFR_DFT_REAL_PLAN_F64* plan);

    // Discrete Cosine Transform type II plans

    KFR_API_SPEC KFR_DCT_PLAN_F32* kfr_dct_create_plan_f32(size_t size);
    KFR_API_SPEC KFR_DCT_PLAN_F64* kfr_dct_create_plan_f64(size_t size);

    KFR_API_SPEC void kfr_dct_dump_f32(KFR_DCT_PLAN_F32* plan);
    KFR_API_SPEC void kfr_dct_dump_f64(KFR_DCT_PLAN_F64* plan);

    KFR_API_SPEC size_t kfr_dct_get_size_f32(KFR_DCT_PLAN_F32* plan);
    KFR_API_SPEC size_t kfr_dct_get_size_f64(KFR_DCT_PLAN_F64* plan);

    KFR_API_SPEC size_t kfr_dct_get_temp_size_f32(KFR_DCT_PLAN_F32* plan);
    KFR_API_SPEC size_t kfr_dct_get_temp_size_f64(KFR_DCT_PLAN_F64* plan);

    KFR_API_SPEC void kfr_dct_execute_f32(KFR_DCT_PLAN_F32* plan, kfr_f32* out, const kfr_f32* in,
                                          uint8_t* temp);
    KFR_API_SPEC void kfr_dct_execute_f64(KFR_DCT_PLAN_F64* plan, kfr_f64* out, const kfr_f64* in,
                                          uint8_t* temp);

    KFR_API_SPEC void kfr_dct_execute_inverse_f32(KFR_DCT_PLAN_F32* plan, kfr_f32* out, const kfr_f32* in,
                                                  uint8_t* temp);
    KFR_API_SPEC void kfr_dct_execute_inverse_f64(KFR_DCT_PLAN_F64* plan, kfr_f64* out, const kfr_f64* in,
                                                  uint8_t* temp);

    KFR_API_SPEC void kfr_dct_delete_plan_f32(KFR_DCT_PLAN_F32* plan);
    KFR_API_SPEC void kfr_dct_delete_plan_f64(KFR_DCT_PLAN_F64* plan);

    // Filters: FIR, IIR

    KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_fir_plan_f32(const kfr_f32* taps, size_t size);
    KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_fir_plan_f64(const kfr_f64* taps, size_t size);

    KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_convolution_plan_f32(const kfr_f32* taps, size_t size,
                                                                        size_t block_size);
    KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_convolution_plan_f64(const kfr_f64* taps, size_t size,
                                                                        size_t block_size);

    KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_iir_plan_f32(const kfr_f32* sos, size_t sos_count);
    KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_iir_plan_f64(const kfr_f64* sos, size_t sos_count);

    KFR_API_SPEC void kfr_filter_process_f32(KFR_FILTER_F32* plan, kfr_f32* output, const kfr_f32* input,
                                             size_t size);
    KFR_API_SPEC void kfr_filter_process_f64(KFR_FILTER_F64* plan, kfr_f64* output, const kfr_f64* input,
                                             size_t size);

    KFR_API_SPEC void kfr_filter_reset_f32(KFR_FILTER_F32* plan);
    KFR_API_SPEC void kfr_filter_reset_f64(KFR_FILTER_F64* plan);

    KFR_API_SPEC void kfr_filter_delete_plan_f32(KFR_FILTER_F32* plan);
    KFR_API_SPEC void kfr_filter_delete_plan_f64(KFR_FILTER_F64* plan);

#ifdef __cplusplus
}
#endif
