/** @addtogroup capi
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

// Architecture detection
#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__)
#define KFR_ARCH_IS_X86 1
#elif defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(__aarch64__)
#define KFR_ARCH_IS_ARM 1
#endif

// Calling convention definition
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

// DLL export/import macros
#ifdef _WIN32
#ifdef KFR_BUILDING_DLL
#define KFR_API_SPEC KFR_CDECL __declspec(dllexport)
#else
#define KFR_API_SPEC KFR_CDECL __declspec(dllimport)
#endif
#else
#ifdef KFR_BUILDING_DLL
#define KFR_API_SPEC KFR_CDECL __attribute__((visibility("default")))
#else
#define KFR_API_SPEC KFR_CDECL
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/// Supported architectures enumeration
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

/// Library version definitions
#define KFR_HEADERS_VERSION 60000

/// @brief Returns the library version as a string.
KFR_API_SPEC const char* kfr_version_string();

/// @brief Returns the library version as an integer.
KFR_API_SPEC uint32_t kfr_version();

/// @brief Returns the list of enabled architectures as a string.
KFR_API_SPEC const char* kfr_enabled_archs();

/// @brief Returns the current architecture in use.
KFR_API_SPEC int kfr_current_arch();

/// @brief Returns the last error message.
KFR_API_SPEC const char* kfr_last_error();

/// Typedefs for single and double precision floating points
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

/// Macro to define opaque structures for different DFT, DCT, and filter plans
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

/// Default memory alignment
#define KFR_DEFAULT_ALIGNMENT 64

/// @brief Allocates memory of specified size.
KFR_API_SPEC void* kfr_allocate(size_t size);

/// @brief Allocates aligned memory of specified size and alignment.
KFR_API_SPEC void* kfr_allocate_aligned(size_t size, size_t alignment);

/// @brief Reallocates memory to new size.
KFR_API_SPEC void* kfr_reallocate(void* ptr, size_t new_size);

/// @brief Reallocates aligned memory to new size and alignment.
KFR_API_SPEC void* kfr_reallocate_aligned(void* ptr, size_t new_size, size_t alignment);

/// @brief Adds a reference to the allocated memory.
KFR_API_SPEC void* kfr_add_ref(void* ptr);

/// @brief Releases a reference to the allocated memory.
KFR_API_SPEC void kfr_release(void* ptr);

/// @brief Deallocates memory.
KFR_API_SPEC void kfr_deallocate(void* ptr);

/// @brief Returns allocated memory size.
KFR_API_SPEC size_t kfr_allocated_size(void* ptr);

/// Enumeration for DFT packing format. See https://www.kfr.dev/docs/latest/dft_format/ for details
typedef enum KFR_DFT_PACK_FORMAT
{
    Perm = 0,
    CCs  = 1
} KFR_DFT_PACK_FORMAT;

/**
 * @brief Create a complex DFT plan (Single precision).
 * @param size Size of the DFT.
 * @return Pointer to the created DFT plan. Use `kfr_dft_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_plan_f32(size_t size);

/**
 * @brief Create a 2D complex DFT plan (Single precision).
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @return Pointer to the created 2D DFT plan. Use `kfr_dft_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_2d_plan_f32(size_t size1, size_t size2);

/**
 * @brief Create a 3D complex DFT plan (Single precision).
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @param size3 Size of the third dimension.
 * @return Pointer to the created 3D DFT plan. Use `kfr_dft_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_3d_plan_f32(size_t size1, size_t size2, size_t size3);

/**
 * @brief Create an N-dimensional complex DFT plan (Single precision).
 * @param dims Number of dimensions.
 * @param shape Array of sizes for each dimension.
 * @return Pointer to the created N-dimensional DFT plan. Use `kfr_dft_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_md_plan_f32(size_t dims, const unsigned* shape);

/**
 * @brief Create a complex DFT plan (Double precision).
 * @param size Size of the DFT.
 * @return Pointer to the created DFT plan. Use `kfr_dft_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_plan_f64(size_t size);

/**
 * @brief Create a 2D complex DFT plan (Double precision).
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @return Pointer to the created 2D DFT plan. Use `kfr_dft_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_2d_plan_f64(size_t size1, size_t size2);

/**
 * @brief Create a 3D complex DFT plan (Double precision).
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @param size3 Size of the third dimension.
 * @return Pointer to the created 3D DFT plan. Use `kfr_dft_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_3d_plan_f64(size_t size1, size_t size2, size_t size3);

/**
 * @brief Create an N-dimensional complex DFT plan (Double precision).
 * @param dims Number of dimensions.
 * @param shape Array of sizes for each dimension.
 * @return Pointer to the created N-dimensional DFT plan. Use `kfr_dft_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_md_plan_f64(size_t dims, const unsigned* shape);

/**
 * @brief Dump details of the DFT plan to stdout for inspection.
 * @param plan Pointer to the DFT plan.
 */
KFR_API_SPEC void kfr_dft_dump_f32(KFR_DFT_PLAN_F32* plan);

/**
 * @brief Dump details of the DFT plan to stdout for inspection.
 * @param plan Pointer to the DFT plan.
 */
KFR_API_SPEC void kfr_dft_dump_f64(KFR_DFT_PLAN_F64* plan);

/**
 * @brief Get the size of the DFT plan, in complex numbers.
 * @param plan Pointer to the DFT plan.
 * @return Size of the DFT plan as passed to kfr_dft_create_plan_f**.
 */
KFR_API_SPEC size_t kfr_dft_get_size_f32(KFR_DFT_PLAN_F32* plan);

/**
 * @brief Get the size of the DFT plan, in complex numbers.
 * @param plan Pointer to the DFT plan.
 * @return Size of the DFT plan as passed to kfr_dft_create_plan_f**.
 */
KFR_API_SPEC size_t kfr_dft_get_size_f64(KFR_DFT_PLAN_F64* plan);

/**
 * @brief Get temporary (scratch) buffer size for DFT plan.
 * @param plan Pointer to the DFT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to the `kfr_dft_execute_f**`
 * and `kfr_dft_execute_inverse_f**` functions may improve performance.
 */
KFR_API_SPEC size_t kfr_dft_get_temp_size_f32(KFR_DFT_PLAN_F32* plan);

/**
 * @brief Get temporary (scratch) buffer size for DFT plan.
 * @param plan Pointer to the DFT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to the `kfr_dft_execute_f**`
 * and `kfr_dft_execute_inverse_f**` functions may improve performance.
 */
KFR_API_SPEC size_t kfr_dft_get_temp_size_f64(KFR_DFT_PLAN_F64* plan);

/**
 * @brief Execute the complex forward DFT on `in` and write the result to `out`.
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to the output data.
 * @param in Pointer to the input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size `kfr_dft_get_temp_size_f**(plan)`
 * will be allocated on stack or heap.
 * @note No scaling is applied. This function reads $N$ complex values from `in` and writes $N$ complex values
 * to `out`, where $N$ is the size passed to `kfr_dft_create_plan_f**`.
 */
KFR_API_SPEC void kfr_dft_execute_f32(KFR_DFT_PLAN_F32* plan, kfr_c32* out, const kfr_c32* in, uint8_t* temp);

/**
 * @brief Execute the complex forward DFT on `in` and write the result to `out`.
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to the output data (frequency domain). May point to the same memory as `in` for in-place
 * execution.
 * @param in Pointer to the input data (time domain).
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size `kfr_dft_get_temp_size_f**(plan)`
 * will be allocated on stack or heap.
 * @note No scaling is applied. This function reads $N$ complex values from `in` and writes $N$ complex values
 * to `out`, where $N$ is the size passed to `kfr_dft_create_plan_f**`.
 */
KFR_API_SPEC void kfr_dft_execute_f64(KFR_DFT_PLAN_F64* plan, kfr_c64* out, const kfr_c64* in, uint8_t* temp);

/**
 * @brief Execute the inverse complex DFT on `in` and write the result to `out` for in-place execution.
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data (time domain). May point to the same memory as `in` for in-place
 * execution.
 * @param in Pointer to input data (frequency domain).
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size `kfr_dft_get_temp_size_f**(plan)`
 * will be allocated on stack or heap.
 * @note No scaling is applied. This function reads $N$ complex values from `in` and writes $N$ complex values
 * to `out`, where $N$ is the size passed to `kfr_dft_create_plan_f**`.
 */
KFR_API_SPEC void kfr_dft_execute_inverse_f32(KFR_DFT_PLAN_F32* plan, kfr_c32* out, const kfr_c32* in,
                                              uint8_t* temp);

/**
 * @brief Execute the inverse complex DFT on `in` and write the result to `out`.
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data (time domain).
 * @param in Pointer to input data (frequency domain).
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size `kfr_dft_get_temp_size_f**(plan)`
 * will be allocated on stack or heap.
 * @note No scaling is applied. This function reads $N$ complex values from `in` and writes $N$ complex values
 * to `out`, where $N$ is the size passed to `kfr_dft_create_plan_f**`.
 */
KFR_API_SPEC void kfr_dft_execute_inverse_f64(KFR_DFT_PLAN_F64* plan, kfr_c64* out, const kfr_c64* in,
                                              uint8_t* temp);

/**
 * @brief Delete a complex DFT plan.
 * @param plan Pointer to the DFT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dft_delete_plan_f32(KFR_DFT_PLAN_F32* plan);

/**
 * @brief Delete a complex DFT plan.
 * @param plan Pointer to the DFT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dft_delete_plan_f64(KFR_DFT_PLAN_F64* plan);

/**
 * @brief Create a real DFT plan (Single precision).
 * @param size Size of the real DFT. Must be even.
 * @param pack_format Packing format for the DFT.
 * @return Pointer to the created DFT plan. Use `kfr_dft_real_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_plan_f32(size_t size,
                                                                 KFR_DFT_PACK_FORMAT pack_format);

KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_2d_plan_f32(size_t size1, size_t size2,
                                                                    bool real_out_is_enough);
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_3d_plan_f32(size_t size1, size_t size2, size_t size3,
                                                                    bool real_out_is_enough);
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_md_plan_f32(size_t dims, const unsigned* shape,
                                                                    bool real_out_is_enough);

/**
 * @brief Create a real DFT plan (Double precision).
 * @param size Size of the real DFT. Must be even.
 * @param pack_format Packing format for the DFT.
 * @return Pointer to the created DFT plan. Use `kfr_dft_real_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_plan_f64(size_t size,
                                                                 KFR_DFT_PACK_FORMAT pack_format);

KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_2d_plan_f64(size_t size1, size_t size2,
                                                                    bool real_out_is_enough);
KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_3d_plan_f64(size_t size1, size_t size2, size_t size3,
                                                                    bool real_out_is_enough);
KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_md_plan_f64(size_t dims, const unsigned* shape,
                                                                    bool real_out_is_enough);

/**
 * @brief Dump details of the real DFT plan to stdout for inspection.
 * @param plan Pointer to the DFT plan.
 */
KFR_API_SPEC void kfr_dft_real_dump_f32(KFR_DFT_REAL_PLAN_F32* plan);

/**
 * @brief Dump details of the real DFT plan to stdout for inspection.
 * @param plan Pointer to the DFT plan.
 */
KFR_API_SPEC void kfr_dft_real_dump_f64(KFR_DFT_REAL_PLAN_F64* plan);

/**
 * @brief Get the size of a real DFT plan.
 * @param plan Pointer to the DFT plan.
 * @return Size of the DFT as passed to `kfr_dft_real_create_plan_f**`.
 */
KFR_API_SPEC size_t kfr_dft_real_get_size_f32(KFR_DFT_REAL_PLAN_F32* plan);

/**
 * @brief Get the size of a real DFT plan.
 * @param plan Pointer to the DFT plan.
 * @return Size of the DFT as passed to `kfr_dft_real_create_plan_f**`.
 */
KFR_API_SPEC size_t kfr_dft_real_get_size_f64(KFR_DFT_REAL_PLAN_F64* plan);

/**
 * @brief Get temporary (scratch) buffer size for real DFT plan (Single precision).
 * @param plan Pointer to the DFT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to the `kfr_dft_execute_f**`
 * and `kfr_dft_execute_inverse_f**` functions may improve performance.
 */
KFR_API_SPEC size_t kfr_dft_real_get_temp_size_f32(KFR_DFT_REAL_PLAN_F32* plan);

/**
 * @brief Get temporary (scratch) buffer size for real DFT plan (Double precision).
 * @param plan Pointer to the DFT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to the `kfr_dft_execute_f**`
 * and `kfr_dft_execute_inverse_f**` functions may improve performance.
 */
KFR_API_SPEC size_t kfr_dft_real_get_temp_size_f64(KFR_DFT_REAL_PLAN_F64* plan);

/**
 * @brief Execute real DFT on `in` and write the result to `out`
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data. May point to the same memory as `in` for in-place execution.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size
 * `kfr_dft_real_get_temp_size_f**(plan)` will be allocated on stack or heap.
 * @note This function reads $N$ real values from `in` and writes $\frac{N}{2}$ (`Perm` format) or
 * $\frac{N}{2}+1$ (`CCs` format) complex values to `out`, where $N$ is the size passed to
 * `kfr_dft_real_create_plan_f**`.
 */
KFR_API_SPEC void kfr_dft_real_execute_f32(KFR_DFT_REAL_PLAN_F32* plan, kfr_c32* out, const kfr_f32* in,
                                           uint8_t* temp);

/**
 * @brief Execute real DFT on `in` and write the result to `out`.
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data. May point to the same memory as `in` for in-place execution.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size
 * `kfr_dft_real_get_temp_size_f**(plan)` will be allocated on stack or heap.
 * @note This function reads $N$ real values from `in` and writes $\frac{N}{2}$ (`Perm` format) or
 * $\frac{N}{2}+1$ (`CCs` format) complex values to `out`, where $N$ is the size passed to
 * `kfr_dft_real_create_plan_f**`.
 */
KFR_API_SPEC void kfr_dft_real_execute_f64(KFR_DFT_REAL_PLAN_F64* plan, kfr_c64* out, const kfr_f64* in,
                                           uint8_t* temp);

/**
 * @brief Execute inverse real DFT on `in` and write the result to `out`.
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data. May point to the same memory as `in` for in-place execution.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size
 * `kfr_dft_real_get_temp_size_f**(plan)` will be allocated on stack or heap.
 * @note This function reads $\frac{N}{2}$ (`Perm` format) or $\frac{N}{2}+1$ (`CCs` format) complex values
 * from `in` and writes $N$ real values to `out`, where $N$ is the size passed to
 * `kfr_dft_real_create_plan_f**`.
 */
KFR_API_SPEC void kfr_dft_real_execute_inverse_f32(KFR_DFT_REAL_PLAN_F32* plan, kfr_f32* out,
                                                   const kfr_c32* in, uint8_t* temp);

/**
 * @brief Execute inverse real DFT on `in` and write the result to `out`.
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data. May point to the same memory as `in` for in-place execution.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size
 * `kfr_dft_real_get_temp_size_f**(plan)` will be allocated on stack or heap.
 * @note This function reads $\frac{N}{2}$ (`Perm` format) or $\frac{N}{2}+1$ (`CCs` format) complex values
 * from `in` and writes $N$ real values to `out`, where $N$ is the size passed to
 * `kfr_dft_real_create_plan_f**`.
 */
KFR_API_SPEC void kfr_dft_real_execute_inverse_f64(KFR_DFT_REAL_PLAN_F64* plan, kfr_f64* out,
                                                   const kfr_c64* in, uint8_t* temp);

/**
 * @brief Delete a real DFT plan.
 * @param plan Pointer to the DFT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dft_real_delete_plan_f32(KFR_DFT_REAL_PLAN_F32* plan);

/**
 * @brief Delete a real DFT plan.
 * @param plan Pointer to the DFT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dft_real_delete_plan_f64(KFR_DFT_REAL_PLAN_F64* plan);

/**
 * @brief Create a DCT-II plan (Single precision).
 * @param size Size of the DCT. Must be even.
 * @return Pointer to the created DCT plan.
 */
KFR_API_SPEC KFR_DCT_PLAN_F32* kfr_dct_create_plan_f32(size_t size);

/**
 * @brief Create a DCT-II plan (Double precision).
 * @param size Size of the DCT. Must be even.
 * @return Pointer to the created DCT plan.
 */
KFR_API_SPEC KFR_DCT_PLAN_F64* kfr_dct_create_plan_f64(size_t size);

/**
 * @brief Dump details of the DCT plan to stdout for inspection.
 * @param plan Pointer to the DCT plan.
 */
KFR_API_SPEC void kfr_dct_dump_f32(KFR_DCT_PLAN_F32* plan);

/**
 * @brief Dump details of the DCT plan to stdout for inspection.
 * @param plan Pointer to the DCT plan.
 */
KFR_API_SPEC void kfr_dct_dump_f64(KFR_DCT_PLAN_F64* plan);

/**
 * @brief Get the size of a DCT plan.
 * @param plan Pointer to the DCT plan.
 * @return Size of the DCT as passed to `kfr_dct_create_plan_f**`.
 */
KFR_API_SPEC size_t kfr_dct_get_size_f32(KFR_DCT_PLAN_F32* plan);

/**
 * @brief Get the size of a DCT plan.
 * @param plan Pointer to the DCT plan.
 * @return Size of the DCT as passed to `kfr_dct_create_plan_f**`.
 */
KFR_API_SPEC size_t kfr_dct_get_size_f64(KFR_DCT_PLAN_F64* plan);

/**
 * @brief Get temporary (scratch) buffer size for DCT plan.
 * @param plan Pointer to the DCT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to the `kfr_dct_execute_f**`
 * and `kfr_dct_execute_inverse_f**` functions may improve performance.
 */
KFR_API_SPEC size_t kfr_dct_get_temp_size_f32(KFR_DCT_PLAN_F32* plan);

/**
 * @brief Get temporary (scratch) buffer size for DCT plan.
 * @param plan Pointer to the DCT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to the `kfr_dct_execute_f**`
 * and `kfr_dct_execute_inverse_f**` functions may improve performance.
 */
KFR_API_SPEC size_t kfr_dct_get_temp_size_f64(KFR_DCT_PLAN_F64* plan);

/**
 * @brief Execute DCT-II on `in` and write the result to `out`.
 * @param plan Pointer to the DCT plan.
 * @param out Pointer to output data.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size
 * `kfr_dct_get_temp_size_f**(plan)` will be allocated on stack or heap.
 * @note No scaling is applied. This function read $N$ values from `in` and writes $N$ values to `out`, where
 * $N$ is the size passed to `kfr_dct_create_plan_f**`..
 */
KFR_API_SPEC void kfr_dct_execute_f32(KFR_DCT_PLAN_F32* plan, kfr_f32* out, const kfr_f32* in, uint8_t* temp);

/**
 * @brief Execute DCT-II on `in` and write the result to `out`.
 * @param plan Pointer to the DCT plan.
 * @param out Pointer to output data.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size
 * `kfr_dct_get_temp_size_f**(plan)` will be allocated on stack or heap.
 * @note No scaling is applied. This function read $N$ values from `in` and writes $N$ values to `out`, where
 * $N$ is the size passed to `kfr_dct_create_plan_f**`..
 */
KFR_API_SPEC void kfr_dct_execute_f64(KFR_DCT_PLAN_F64* plan, kfr_f64* out, const kfr_f64* in, uint8_t* temp);

/**
 * @brief Execute inverse DCT-II (aka DCT-III) on `in` and write the result to `out`.
 * @param plan Pointer to the DCT plan.
 * @param out Pointer to output data.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size
 * `kfr_dct_get_temp_size_f**(plan)` will be allocated on stack or heap.
 * @note No scaling is applied. This function read $N$ values from `in` and writes $N$ values to `out`, where
 * $N$ is the size passed to `kfr_dct_create_plan_f**`..
 */
KFR_API_SPEC void kfr_dct_execute_inverse_f32(KFR_DCT_PLAN_F32* plan, kfr_f32* out, const kfr_f32* in,
                                              uint8_t* temp);

/**
 * @brief Execute inverse DCT-II (aka DCT-III) on `in` and write the result to `out`.
 * @param plan Pointer to the DCT plan.
 * @param out Pointer to output data.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, scratch buffer of size
 * `kfr_dct_get_temp_size_f**(plan)` will be allocated on stack or heap.
 * @note No scaling is applied. This function read $N$ values from `in` and writes $N$ values to `out`, where
 * $N$ is the size passed to `kfr_dct_create_plan_f**`..
 */
KFR_API_SPEC void kfr_dct_execute_inverse_f64(KFR_DCT_PLAN_F64* plan, kfr_f64* out, const kfr_f64* in,
                                              uint8_t* temp);

/**
 * @brief Delete a DCT plan.
 * @param plan Pointer to the DCT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dct_delete_plan_f32(KFR_DCT_PLAN_F32* plan);

/**
 * @brief Delete a DCT plan.
 * @param plan Pointer to the DCT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dct_delete_plan_f64(KFR_DCT_PLAN_F64* plan);

/**
 * @brief Create a FIR filter plan (Single precision).
 * @param taps Pointer to filter taps.
 * @param size Number of filter taps.
 * @return Pointer to the created FIR filter plan. Use `kfr_filter_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_fir_plan_f32(const kfr_f32* taps, size_t size);

/**
 * @brief Create a FIR filter plan (Double precision).
 * @param taps Pointer to filter taps.
 * @param size Number of filter taps.
 * @return Pointer to the created FIR filter plan. Use `kfr_filter_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_fir_plan_f64(const kfr_f64* taps, size_t size);

/**
 * @brief Create a convolution filter plan (Single precision).
 * @param taps Pointer to filter taps.
 * @param size Number of filter taps.
 * @param block_size Size of the processing block. Must be a power of two.
 * @return Pointer to the created convolution filter plan. Use `kfr_filter_delete_plan_f**` to free.
 * @note Mathematically, this produces the same result as an FIR filter, but it uses the FFT overlap-add
 technique internally to improve performance with larger filter lengths.
 */
KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_convolution_plan_f32(const kfr_f32* taps, size_t size,
                                                                    size_t block_size);

/**
 * @brief Create a convolution filter plan (Double precision).
 * @param taps Pointer to filter taps.
 * @param size Number of filter taps.
 * @param block_size Size of the processing block. Must be a power of two.
 * @return Pointer to the created convolution filter plan. Use `kfr_filter_delete_plan_f**` to free.
 * @note Mathematically, this produces the same result as an FIR filter, but it uses the FFT overlap-add
 technique internally to improve performance with larger filter lengths.
 */
KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_convolution_plan_f64(const kfr_f64* taps, size_t size,
                                                                    size_t block_size);

/**
 * @brief Create a IIR filter plan (Single precision).
 * @param sos Pointer to second-order sections (SOS) coefficients.
 * @param sos_count Number of second-order sections.
 * @return Pointer to the created IIR filter plan. Use `kfr_filter_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_iir_plan_f32(const kfr_f32* sos, size_t sos_count);

/**
 * @brief Create a IIR filter plan (Double precision).
 * @param sos Pointer to second-order sections (SOS) coefficients.
 * @param sos_count Number of second-order sections.
 * @return Pointer to the created IIR filter plan. Use `kfr_filter_delete_plan_f**` to free.
 */
KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_iir_plan_f64(const kfr_f64* sos, size_t sos_count);

/**
 * @brief Process input data with a filter.
 * @param plan Pointer to the filter plan.
 * @param output Pointer to output data. May point to the same memory as `input` for in-place execution.
 * @param input Pointer to input data.
 * @param size Number of samples to process.
 */
KFR_API_SPEC void kfr_filter_process_f32(KFR_FILTER_F32* plan, kfr_f32* output, const kfr_f32* input,
                                         size_t size);

/**
 * @brief Process input data with a filter.
 * @param plan Pointer to the filter plan.
 * @param output Pointer to output data. May point to the same memory as `input` for in-place execution.
 * @param input Pointer to input data.
 * @param size Number of samples to process.
 */
KFR_API_SPEC void kfr_filter_process_f64(KFR_FILTER_F64* plan, kfr_f64* output, const kfr_f64* input,
                                         size_t size);

/**
 * @brief Reset the internal state of a filter plan, including delay line.
 * @param plan Pointer to the filter plan.
 */
KFR_API_SPEC void kfr_filter_reset_f32(KFR_FILTER_F32* plan);

/**
 * @brief Reset the internal state of a filter plan, including delay line.
 * @param plan Pointer to the filter plan.
 */
KFR_API_SPEC void kfr_filter_reset_f64(KFR_FILTER_F64* plan);

/**
 * @brief Delete a filter plan.
 * @param plan Pointer to the filter plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_filter_delete_plan_f32(KFR_FILTER_F32* plan);

/**
 * @brief Delete a filter plan.
 * @param plan Pointer to the filter plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_filter_delete_plan_f64(KFR_FILTER_F64* plan);

#ifdef __cplusplus
}
#endif
