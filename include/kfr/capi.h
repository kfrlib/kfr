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

#include <stddef.h>
#include <stdint.h>

#if defined __STDC_IEC_559_COMPLEX__ && !defined KFR_NO_C_COMPLEX_TYPES
#include <complex.h>
#endif

#include "config.h"

/** @cond INTERNAL */

/* Architecture detection */
#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__)
/** Defined to 1 when compiling for an x86 or x86-64 target. */
#define KFR_ARCH_IS_X86 1
#elif defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(__aarch64__)
/** Defined to 1 when compiling for an ARM (32- or 64-bit) target. */
#define KFR_ARCH_IS_ARM 1
#elif defined(__riscv)
/** Defined to 1 when compiling for a RISC-V target. */
#define KFR_ARCH_IS_RISCV 1
#endif

#ifndef KFR_CDECL

/* Calling convention definition */
#if defined KFR_ARCH_IS_X86
#if defined(_M_X64) || defined(__x86_64__)
/* 64-bit systems use the same calling convention */
#define KFR_CDECL
#else
#if defined(_MSC_VER)
#define KFR_CDECL __cdecl
#else
#define KFR_CDECL __attribute__((cdecl))
#endif
#endif
#else
#define KFR_CDECL
#endif

#endif

/** @endcond */

/**
 * Calling-convention and visibility specifier applied to every public CAPI
 * function. Expands to the appropriate `__declspec(dllexport)` /
 * `__declspec(dllimport)` / visibility attribute depending on whether the
 * library is being built (`KFR_BUILDING_DLL`) or consumed.
 */
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

/* Check for C99 or later */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
/* C99 or later - use _Bool */
/** Boolean type used throughout the CAPI. Maps to `_Bool` in C99. */
typedef _Bool kfr_bool;
/** Boolean true value for `kfr_bool`. */
#define KFR_TRUE 1
/** Boolean false value for `kfr_bool`. */
#define KFR_FALSE 0
#elif defined(__cplusplus)
/* C++ - use bool */
/** Boolean type used throughout the CAPI. Maps to `bool` in C++. */
typedef bool kfr_bool;
/** Boolean true value for `kfr_bool`. */
#define KFR_TRUE true
/** Boolean false value for `kfr_bool`. */
#define KFR_FALSE false
#else
/* C89 - fallback to char */
/** Boolean type used throughout the CAPI. Maps to `char` in C89. */
typedef char kfr_bool;
/** Boolean true value for `kfr_bool`. */
#define KFR_TRUE 1
/** Boolean false value for `kfr_bool`. */
#define KFR_FALSE 0
#endif

/** Supported architectures enumeration. */
enum
{
    KFR_ARCH_X86    = 0, /**< Baseline x86 (no SIMD). */
    KFR_ARCH_SSE2   = 1, /**< SSE2. */
    KFR_ARCH_SSE3   = 2, /**< SSE3. */
    KFR_ARCH_SSSE3  = 3, /**< SSSE3. */
    KFR_ARCH_SSE41  = 4, /**< SSE4.1. */
    KFR_ARCH_SSE42  = 5, /**< SSE4.2. */
    KFR_ARCH_AVX    = 6, /**< AVX. */
    KFR_ARCH_AVX2   = 7, /**< AVX2. */
    KFR_ARCH_AVX512 = 8, /**< AVX-512. */
};

/** Library headers version, encoded as `major * 10000 + minor * 100 + patch`. */
#define KFR_HEADERS_VERSION 70000

/** Returns the library version as a string. */
KFR_API_SPEC const char* kfr_version_string();

/** Returns the library version as an integer. */
KFR_API_SPEC uint32_t kfr_version();

/** Returns the list of enabled architectures as a string. */
KFR_API_SPEC const char* kfr_enabled_archs();

/** Returns the current architecture in use. */
KFR_API_SPEC int kfr_current_arch();

/**
 * Returns the last error message.
 *
 * CAPI functions that can fail store a description of the most recent exception
 * in a thread-local buffer. Returns an empty string when no error has occurred
 * since the last successful call.
 */
KFR_API_SPEC const char* kfr_last_error();

/** Single-precision floating-point type. */
typedef float kfr_f32;
/** Double-precision floating-point type. */
typedef double kfr_f64;

#if defined __STDC_IEC_559_COMPLEX__ && !defined KFR_NO_C_COMPLEX_TYPES
/** Single-precision complex type. */
typedef float _Complex kfr_c32;
/** Double-precision complex type. */
typedef double _Complex kfr_c64;
/** Multiplier applied to a complex element count to obtain the underlying scalar count. */
#define KFR_COMPLEX_SIZE_MULTIPLIER 1
#else
/** Single-precision complex type (stored as interleaved real/imaginary scalars). */
typedef float kfr_c32;
/** Double-precision complex type (stored as interleaved real/imaginary scalars). */
typedef double kfr_c64;
/** Multiplier applied to a complex element count to obtain the underlying scalar count. */
#define KFR_COMPLEX_SIZE_MULTIPLIER 2
#endif

/**
 * Defines an opaque plan handle type named `NAME`.
 *
 * The handle is a small struct wrapping a single integer; the integer is never
 * used by clients and only exists to give each plan type a distinct, non-void
 * pointer identity. The actual implementation object is reinterpreted from the
 * handle pointer inside the library.
 */
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

/** Default alignment, in bytes, applied by `kfr_allocate`. */
#define KFR_DEFAULT_ALIGNMENT 64

/**
 * Allocates `size` bytes aligned to `KFR_DEFAULT_ALIGNMENT`.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated block, or `NULL` on failure.
 */
KFR_API_SPEC void* kfr_allocate(size_t size);

/**
 * Allocates `size` bytes aligned to `alignment`.
 *
 * @param size Number of bytes to allocate.
 * @param alignment Required alignment in bytes. Must be a power of two.
 * @return Pointer to the allocated block, or `NULL` on failure.
 */
KFR_API_SPEC void* kfr_allocate_aligned(size_t size, size_t alignment);

/**
 * Deallocates a block previously returned by `kfr_allocate` or
 * `kfr_allocate_aligned`.
 *
 * @param ptr Pointer returned by a matching allocation call. May be `NULL`.
 */
KFR_API_SPEC void kfr_deallocate(void* ptr);

#ifdef KFR_MANAGED_ALLOCATION
/**
 * Reallocates `ptr` to `new_size` bytes, preserving existing contents up to the
 * smaller of the old and new sizes. Uses `KFR_DEFAULT_ALIGNMENT`.
 *
 * @param ptr Pointer previously returned by `kfr_allocate` or
 * `kfr_reallocate`. May be `NULL`, in which case a new allocation is performed.
 * @param new_size Requested size in bytes.
 * @return Pointer to the reallocated block, or `NULL` on failure.
 */
KFR_API_SPEC void* kfr_reallocate(void* ptr, size_t new_size);

/**
 * Reallocates `ptr` to `new_size` bytes with the given `alignment`, preserving
 * existing contents up to the smaller of the old and new sizes.
 *
 * @param ptr Pointer previously returned by `kfr_allocate_aligned` or
 * `kfr_reallocate_aligned`. May be `NULL`, in which case a new allocation is performed.
 * @param new_size Requested size in bytes.
 * @param alignment Required alignment in bytes. Must be a power of two.
 * @return Pointer to the reallocated block, or `NULL` on failure.
 */
KFR_API_SPEC void* kfr_reallocate_aligned(void* ptr, size_t new_size, size_t alignment);

/**
 * Increments the reference count of `ptr` and returns it.
 *
 * @param ptr Pointer previously returned by a managed allocation call.
 * @return The same pointer `ptr`.
 */
KFR_API_SPEC void* kfr_add_ref(void* ptr);

/**
 * Decrements the reference count of `ptr`; when it reaches zero the block is freed.
 *
 * @param ptr Pointer previously returned by a managed allocation call.
 */
KFR_API_SPEC void kfr_release(void* ptr);

/**
 * Returns the usable size, in bytes, of the block at `ptr`.
 *
 * @param ptr Pointer previously returned by a managed allocation call.
 * @return Allocated size in bytes.
 */
KFR_API_SPEC size_t kfr_allocated_size(void* ptr);
#endif

/**
 * DFT packing format for real DFTs.
 * See https://www.kfr.dev/docs/latest/dft_format/ for details.
 */
typedef enum KFR_DFT_PACK_FORMAT
{
    Perm = 0, /**< Permuted format (N/2 complex values). */
    CCs  = 1 /**< Conjugate-complex symmetric format (N/2 + 1 complex values). */
} KFR_DFT_PACK_FORMAT;

/**
 * Creates a complex DFT plan (single precision).
 *
 * @param size Size of the DFT.
 * @return Pointer to the created DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_plan_f32(size_t size);

/**
 * Creates a 2D complex DFT plan (single precision).
 *
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @return Pointer to the created 2D DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_2d_plan_f32(size_t size1, size_t size2);

/**
 * Creates a 3D complex DFT plan (single precision).
 *
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @param size3 Size of the third dimension.
 * @return Pointer to the created 3D DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_3d_plan_f32(size_t size1, size_t size2, size_t size3);

/**
 * Creates an N-dimensional complex DFT plan (single precision).
 *
 * @param dims Number of dimensions.
 * @param shape Array of sizes for each dimension.
 * @return Pointer to the created N-dimensional DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_md_plan_f32(size_t dims, const unsigned* shape);

/**
 * Creates a complex DFT plan (double precision).
 *
 * @param size Size of the DFT.
 * @return Pointer to the created DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_plan_f64(size_t size);

/**
 * Creates a 2D complex DFT plan (double precision).
 *
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @return Pointer to the created 2D DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_2d_plan_f64(size_t size1, size_t size2);

/**
 * Creates a 3D complex DFT plan (double precision).
 *
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @param size3 Size of the third dimension.
 * @return Pointer to the created 3D DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_3d_plan_f64(size_t size1, size_t size2, size_t size3);

/**
 * Creates an N-dimensional complex DFT plan (double precision).
 *
 * @param dims Number of dimensions.
 * @param shape Array of sizes for each dimension.
 * @return Pointer to the created N-dimensional DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_md_plan_f64(size_t dims, const unsigned* shape);

/**
 * Dumps details of the DFT plan to stdout for inspection.
 *
 * @param plan Pointer to the DFT plan.
 */
KFR_API_SPEC void kfr_dft_dump_f32(KFR_DFT_PLAN_F32* plan);

/**
 * Dumps details of the DFT plan to stdout for inspection.
 *
 * @param plan Pointer to the DFT plan.
 */
KFR_API_SPEC void kfr_dft_dump_f64(KFR_DFT_PLAN_F64* plan);

/**
 * Returns the size of the DFT plan, in complex numbers.
 *
 * @param plan Pointer to the DFT plan.
 * @return Size of the DFT plan as passed to `kfr_dft_create_plan_f32`.
 */
KFR_API_SPEC size_t kfr_dft_get_size_f32(KFR_DFT_PLAN_F32* plan);

/**
 * Returns the size of the DFT plan, in complex numbers.
 *
 * @param plan Pointer to the DFT plan.
 * @return Size of the DFT plan as passed to `kfr_dft_create_plan_f64`.
 */
KFR_API_SPEC size_t kfr_dft_get_size_f64(KFR_DFT_PLAN_F64* plan);

/**
 * Returns the temporary (scratch) buffer size required by the DFT plan.
 *
 * @param plan Pointer to the DFT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to
 * `kfr_dft_execute_f32` and `kfr_dft_execute_inverse_f32` may improve performance.
 */
KFR_API_SPEC size_t kfr_dft_get_temp_size_f32(KFR_DFT_PLAN_F32* plan);

/**
 * Returns the temporary (scratch) buffer size required by the DFT plan.
 *
 * @param plan Pointer to the DFT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to
 * `kfr_dft_execute_f64` and `kfr_dft_execute_inverse_f64` may improve performance.
 */
KFR_API_SPEC size_t kfr_dft_get_temp_size_f64(KFR_DFT_PLAN_F64* plan);

/**
 * Executes the complex forward DFT on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to the output data (frequency domain). May point to the same
 *        memory as `in` for in-place execution.
 * @param in Pointer to the input data (time domain).
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dft_get_temp_size_f32(plan)` will be allocated on the stack or heap.
 * @note No scaling is applied. This function reads $N$ complex values from `in` and
 * writes $N$ complex values to `out`, where $N$ is the size passed to
 * `kfr_dft_create_plan_f32`.
 */
KFR_API_SPEC void kfr_dft_execute_f32(KFR_DFT_PLAN_F32* plan, kfr_c32* out, const kfr_c32* in, uint8_t* temp);

/**
 * Executes the complex forward DFT on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to the output data (frequency domain). May point to the same
 *        memory as `in` for in-place execution.
 * @param in Pointer to the input data (time domain).
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dft_get_temp_size_f64(plan)` will be allocated on the stack or heap.
 * @note No scaling is applied. This function reads $N$ complex values from `in` and
 * writes $N$ complex values to `out`, where $N$ is the size passed to
 * `kfr_dft_create_plan_f64`.
 */
KFR_API_SPEC void kfr_dft_execute_f64(KFR_DFT_PLAN_F64* plan, kfr_c64* out, const kfr_c64* in, uint8_t* temp);

/**
 * Executes the inverse complex DFT on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data (time domain). May point to the same memory as
 *        `in` for in-place execution.
 * @param in Pointer to input data (frequency domain).
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dft_get_temp_size_f32(plan)` will be allocated on the stack or heap.
 * @note No scaling is applied. This function reads $N$ complex values from `in` and
 * writes $N$ complex values to `out`, where $N$ is the size passed to
 * `kfr_dft_create_plan_f32`.
 */
KFR_API_SPEC void kfr_dft_execute_inverse_f32(KFR_DFT_PLAN_F32* plan, kfr_c32* out, const kfr_c32* in,
                                              uint8_t* temp);

/**
 * Executes the inverse complex DFT on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data (time domain). May point to the same memory as
 *        `in` for in-place execution.
 * @param in Pointer to input data (frequency domain).
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dft_get_temp_size_f64(plan)` will be allocated on the stack or heap.
 * @note No scaling is applied. This function reads $N$ complex values from `in` and
 * writes $N$ complex values to `out`, where $N$ is the size passed to
 * `kfr_dft_create_plan_f64`.
 */
KFR_API_SPEC void kfr_dft_execute_inverse_f64(KFR_DFT_PLAN_F64* plan, kfr_c64* out, const kfr_c64* in,
                                              uint8_t* temp);

/**
 * Deletes a complex DFT plan.
 *
 * @param plan Pointer to the DFT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dft_delete_plan_f32(KFR_DFT_PLAN_F32* plan);

/**
 * Deletes a complex DFT plan.
 *
 * @param plan Pointer to the DFT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dft_delete_plan_f64(KFR_DFT_PLAN_F64* plan);

/**
 * Creates a real DFT plan (single precision).
 *
 * @param size Size of the real DFT. Must be even.
 * @param pack_format Packing format for the DFT.
 * @return Pointer to the created DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_real_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_plan_f32(size_t size,
                                                                 KFR_DFT_PACK_FORMAT pack_format);

/**
 * Creates a 2D real DFT plan (single precision).
 *
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @param real_out_is_enough If true, the inverse transform may write directly into
 *        the real output buffer without an extra complex working region, reducing
 *        the required scratch size.
 * @return Pointer to the created 2D DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_real_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_2d_plan_f32(size_t size1, size_t size2,
                                                                    kfr_bool real_out_is_enough);
/**
 * Creates a 3D real DFT plan (single precision).
 *
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @param size3 Size of the third dimension.
 * @param real_out_is_enough If true, the inverse transform may write directly into
 *        the real output buffer without an extra complex working region, reducing
 *        the required scratch size.
 * @return Pointer to the created 3D DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_real_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_3d_plan_f32(size_t size1, size_t size2, size_t size3,
                                                                    kfr_bool real_out_is_enough);
/**
 * Creates an N-dimensional real DFT plan (single precision).
 *
 * @param dims Number of dimensions.
 * @param shape Array of sizes for each dimension.
 * @param real_out_is_enough If true, the inverse transform may write directly into
 *        the real output buffer without an extra complex working region, reducing
 *        the required scratch size.
 * @return Pointer to the created N-dimensional DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_real_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_md_plan_f32(size_t dims, const unsigned* shape,
                                                                    kfr_bool real_out_is_enough);

/**
 * Creates a real DFT plan (double precision).
 *
 * @param size Size of the real DFT. Must be even.
 * @param pack_format Packing format for the DFT.
 * @return Pointer to the created DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_real_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_plan_f64(size_t size,
                                                                 KFR_DFT_PACK_FORMAT pack_format);

/**
 * Creates a 2D real DFT plan (double precision).
 *
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @param real_out_is_enough If true, the inverse transform may write directly into
 *        the real output buffer without an extra complex working region, reducing
 *        the required scratch size.
 * @return Pointer to the created 2D DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_real_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_2d_plan_f64(size_t size1, size_t size2,
                                                                    kfr_bool real_out_is_enough);
/**
 * Creates a 3D real DFT plan (double precision).
 *
 * @param size1 Size of the first dimension.
 * @param size2 Size of the second dimension.
 * @param size3 Size of the third dimension.
 * @param real_out_is_enough If true, the inverse transform may write directly into
 *        the real output buffer without an extra complex working region, reducing
 *        the required scratch size.
 * @return Pointer to the created 3D DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_real_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_3d_plan_f64(size_t size1, size_t size2, size_t size3,
                                                                    kfr_bool real_out_is_enough);
/**
 * Creates an N-dimensional real DFT plan (double precision).
 *
 * @param dims Number of dimensions.
 * @param shape Array of sizes for each dimension.
 * @param real_out_is_enough If true, the inverse transform may write directly into
 *        the real output buffer without an extra complex working region, reducing
 *        the required scratch size.
 * @return Pointer to the created N-dimensional DFT plan, or `NULL` on failure.
 *         Use `kfr_dft_real_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_md_plan_f64(size_t dims, const unsigned* shape,
                                                                    kfr_bool real_out_is_enough);

/**
 * Dumps details of the real DFT plan to stdout for inspection.
 *
 * @param plan Pointer to the DFT plan.
 */
KFR_API_SPEC void kfr_dft_real_dump_f32(KFR_DFT_REAL_PLAN_F32* plan);

/**
 * Dumps details of the real DFT plan to stdout for inspection.
 *
 * @param plan Pointer to the DFT plan.
 */
KFR_API_SPEC void kfr_dft_real_dump_f64(KFR_DFT_REAL_PLAN_F64* plan);

/**
 * Returns the size of a real DFT plan.
 *
 * @param plan Pointer to the DFT plan.
 * @return Size of the DFT as passed to `kfr_dft_real_create_plan_f32`.
 */
KFR_API_SPEC size_t kfr_dft_real_get_size_f32(KFR_DFT_REAL_PLAN_F32* plan);

/**
 * Returns the size of a real DFT plan.
 *
 * @param plan Pointer to the DFT plan.
 * @return Size of the DFT as passed to `kfr_dft_real_create_plan_f64`.
 */
KFR_API_SPEC size_t kfr_dft_real_get_size_f64(KFR_DFT_REAL_PLAN_F64* plan);

/**
 * Returns the temporary (scratch) buffer size required by the real DFT plan
 * (single precision).
 *
 * @param plan Pointer to the DFT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to
 * `kfr_dft_real_execute_f32` and `kfr_dft_real_execute_inverse_f32` may improve
 * performance.
 */
KFR_API_SPEC size_t kfr_dft_real_get_temp_size_f32(KFR_DFT_REAL_PLAN_F32* plan);

/**
 * Returns the temporary (scratch) buffer size required by the real DFT plan
 * (double precision).
 *
 * @param plan Pointer to the DFT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to
 * `kfr_dft_real_execute_f64` and `kfr_dft_real_execute_inverse_f64` may improve
 * performance.
 */
KFR_API_SPEC size_t kfr_dft_real_get_temp_size_f64(KFR_DFT_REAL_PLAN_F64* plan);

/**
 * Executes the real DFT on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data. May point to the same memory as `in` for
 *        in-place execution.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dft_real_get_temp_size_f32(plan)` will be allocated on the stack or heap.
 * @note This function reads $N$ real values from `in` and writes $\frac{N}{2}$
 * (`Perm` format) or $\frac{N}{2}+1$ (`CCs` format) complex values to `out`, where
 * $N$ is the size passed to `kfr_dft_real_create_plan_f32`.
 */
KFR_API_SPEC void kfr_dft_real_execute_f32(KFR_DFT_REAL_PLAN_F32* plan, kfr_c32* out, const kfr_f32* in,
                                           uint8_t* temp);

/**
 * Executes the real DFT on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data. May point to the same memory as `in` for
 *        in-place execution.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dft_real_get_temp_size_f64(plan)` will be allocated on the stack or heap.
 * @note This function reads $N$ real values from `in` and writes $\frac{N}{2}$
 * (`Perm` format) or $\frac{N}{2}+1$ (`CCs` format) complex values to `out`, where
 * $N$ is the size passed to `kfr_dft_real_create_plan_f64`.
 */
KFR_API_SPEC void kfr_dft_real_execute_f64(KFR_DFT_REAL_PLAN_F64* plan, kfr_c64* out, const kfr_f64* in,
                                           uint8_t* temp);

/**
 * Executes the inverse real DFT on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data. May point to the same memory as `in` for
 *        in-place execution.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dft_real_get_temp_size_f32(plan)` will be allocated on the stack or heap.
 * @note This function reads $\frac{N}{2}$ (`Perm` format) or $\frac{N}{2}+1$ (`CCs`
 * format) complex values from `in` and writes $N$ real values to `out`, where $N$ is
 * the size passed to `kfr_dft_real_create_plan_f32`.
 */
KFR_API_SPEC void kfr_dft_real_execute_inverse_f32(KFR_DFT_REAL_PLAN_F32* plan, kfr_f32* out,
                                                   const kfr_c32* in, uint8_t* temp);

/**
 * Executes the inverse real DFT on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DFT plan.
 * @param out Pointer to output data. May point to the same memory as `in` for
 *        in-place execution.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dft_real_get_temp_size_f64(plan)` will be allocated on the stack or heap.
 * @note This function reads $\frac{N}{2}$ (`Perm` format) or $\frac{N}{2}+1$ (`CCs`
 * format) complex values from `in` and writes $N$ real values to `out`, where $N$ is
 * the size passed to `kfr_dft_real_create_plan_f64`.
 */
KFR_API_SPEC void kfr_dft_real_execute_inverse_f64(KFR_DFT_REAL_PLAN_F64* plan, kfr_f64* out,
                                                   const kfr_c64* in, uint8_t* temp);

/**
 * Deletes a real DFT plan.
 *
 * @param plan Pointer to the DFT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dft_real_delete_plan_f32(KFR_DFT_REAL_PLAN_F32* plan);

/**
 * Deletes a real DFT plan.
 *
 * @param plan Pointer to the DFT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dft_real_delete_plan_f64(KFR_DFT_REAL_PLAN_F64* plan);

/**
 * Computes the forward complex DFT in one call (single precision). No scaling is applied.
 *
 * @param out Output buffer of `size` complex values. May point to the same memory as `in`.
 * @param in Input buffer of `size` complex values.
 * @param size Number of complex samples.
 */
KFR_API_SPEC void kfr_dft_f32(kfr_c32* out, const kfr_c32* in, size_t size);

/**
 * Computes the forward complex DFT in one call (double precision). No scaling is applied.
 *
 * @param out Output buffer of `size` complex values. May point to the same memory as `in`.
 * @param in Input buffer of `size` complex values.
 * @param size Number of complex samples.
 */
KFR_API_SPEC void kfr_dft_f64(kfr_c64* out, const kfr_c64* in, size_t size);

/**
 * Computes the inverse complex DFT in one call (single precision). No scaling is applied.
 *
 * @param out Output buffer of `size` complex values. May point to the same memory as `in`.
 * @param in Input buffer of `size` complex values.
 * @param size Number of complex samples.
 */
KFR_API_SPEC void kfr_idft_f32(kfr_c32* out, const kfr_c32* in, size_t size);

/**
 * Computes the inverse complex DFT in one call (double precision). No scaling is applied.
 *
 * @param out Output buffer of `size` complex values. May point to the same memory as `in`.
 * @param in Input buffer of `size` complex values.
 * @param size Number of complex samples.
 */
KFR_API_SPEC void kfr_idft_f64(kfr_c64* out, const kfr_c64* in, size_t size);

/**
 * Computes the forward real DFT in one call (single precision). No scaling is applied.
 *
 * @param out Output buffer of `size / 2 + 1` complex values (`CCs` format).
 * @param in Input buffer of `size` real samples.
 * @param size Number of real samples.
 */
KFR_API_SPEC void kfr_realdft_f32(kfr_c32* out, const kfr_f32* in, size_t size);

/**
 * Computes the forward real DFT in one call (double precision). No scaling is applied.
 *
 * @param out Output buffer of `size / 2 + 1` complex values (`CCs` format).
 * @param in Input buffer of `size` real samples.
 * @param size Number of real samples.
 */
KFR_API_SPEC void kfr_realdft_f64(kfr_c64* out, const kfr_f64* in, size_t size);

/**
 * Computes the inverse real DFT in one call (single precision). No scaling is applied.
 *
 * @param out Output buffer of `size` real samples.
 * @param in Input buffer of `size / 2 + 1` complex values (`CCs` format).
 * @param size Number of real output samples.
 */
KFR_API_SPEC void kfr_irealdft_f32(kfr_f32* out, const kfr_c32* in, size_t size);

/**
 * Computes the inverse real DFT in one call (double precision). No scaling is applied.
 *
 * @param out Output buffer of `size` real samples.
 * @param in Input buffer of `size / 2 + 1` complex values (`CCs` format).
 * @param size Number of real output samples.
 */
KFR_API_SPEC void kfr_irealdft_f64(kfr_f64* out, const kfr_c64* in, size_t size);

/**
 * Creates a DCT-II plan (single precision).
 *
 * @param size Size of the DCT. Must be even.
 * @return Pointer to the created DCT plan, or `NULL` on failure.
 *         Use `kfr_dct_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_DCT_PLAN_F32* kfr_dct_create_plan_f32(size_t size);

/**
 * Creates a DCT-II plan (double precision).
 *
 * @param size Size of the DCT. Must be even.
 * @return Pointer to the created DCT plan, or `NULL` on failure.
 *         Use `kfr_dct_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_DCT_PLAN_F64* kfr_dct_create_plan_f64(size_t size);

/**
 * Dumps details of the DCT plan to stdout for inspection.
 *
 * @param plan Pointer to the DCT plan.
 */
KFR_API_SPEC void kfr_dct_dump_f32(KFR_DCT_PLAN_F32* plan);

/**
 * Dumps details of the DCT plan to stdout for inspection.
 *
 * @param plan Pointer to the DCT plan.
 */
KFR_API_SPEC void kfr_dct_dump_f64(KFR_DCT_PLAN_F64* plan);

/**
 * Returns the size of a DCT plan.
 *
 * @param plan Pointer to the DCT plan.
 * @return Size of the DCT as passed to `kfr_dct_create_plan_f32`.
 */
KFR_API_SPEC size_t kfr_dct_get_size_f32(KFR_DCT_PLAN_F32* plan);

/**
 * Returns the size of a DCT plan.
 *
 * @param plan Pointer to the DCT plan.
 * @return Size of the DCT as passed to `kfr_dct_create_plan_f64`.
 */
KFR_API_SPEC size_t kfr_dct_get_size_f64(KFR_DCT_PLAN_F64* plan);

/**
 * Returns the temporary (scratch) buffer size required by the DCT plan.
 *
 * @param plan Pointer to the DCT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to
 * `kfr_dct_execute_f32` and `kfr_dct_execute_inverse_f32` may improve performance.
 */
KFR_API_SPEC size_t kfr_dct_get_temp_size_f32(KFR_DCT_PLAN_F32* plan);

/**
 * Returns the temporary (scratch) buffer size required by the DCT plan.
 *
 * @param plan Pointer to the DCT plan.
 * @return Temporary buffer size in bytes.
 * @note Preallocating a byte buffer of the returned size and passing its pointer to
 * `kfr_dct_execute_f64` and `kfr_dct_execute_inverse_f64` may improve performance.
 */
KFR_API_SPEC size_t kfr_dct_get_temp_size_f64(KFR_DCT_PLAN_F64* plan);

/**
 * Executes DCT-II on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DCT plan.
 * @param out Pointer to output data.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dct_get_temp_size_f32(plan)` will be allocated on the stack or heap.
 * @note No scaling is applied. This function reads $N$ values from `in` and writes $N$
 * values to `out`, where $N$ is the size passed to `kfr_dct_create_plan_f32`.
 */
KFR_API_SPEC void kfr_dct_execute_f32(KFR_DCT_PLAN_F32* plan, kfr_f32* out, const kfr_f32* in, uint8_t* temp);

/**
 * Executes DCT-II on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DCT plan.
 * @param out Pointer to output data.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dct_get_temp_size_f64(plan)` will be allocated on the stack or heap.
 * @note No scaling is applied. This function reads $N$ values from `in` and writes $N$
 * values to `out`, where $N$ is the size passed to `kfr_dct_create_plan_f64`.
 */
KFR_API_SPEC void kfr_dct_execute_f64(KFR_DCT_PLAN_F64* plan, kfr_f64* out, const kfr_f64* in, uint8_t* temp);

/**
 * Executes the inverse DCT-II (aka DCT-III) on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DCT plan.
 * @param out Pointer to output data.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dct_get_temp_size_f32(plan)` will be allocated on the stack or heap.
 * @note No scaling is applied. This function reads $N$ values from `in` and writes $N$
 * values to `out`, where $N$ is the size passed to `kfr_dct_create_plan_f32`.
 */
KFR_API_SPEC void kfr_dct_execute_inverse_f32(KFR_DCT_PLAN_F32* plan, kfr_f32* out, const kfr_f32* in,
                                              uint8_t* temp);

/**
 * Executes the inverse DCT-II (aka DCT-III) on `in` and writes the result to `out`.
 *
 * @param plan Pointer to the DCT plan.
 * @param out Pointer to output data.
 * @param in Pointer to input data.
 * @param temp Temporary (scratch) buffer. If `NULL`, a scratch buffer of size
 *        `kfr_dct_get_temp_size_f64(plan)` will be allocated on the stack or heap.
 * @note No scaling is applied. This function reads $N$ values from `in` and writes $N$
 * values to `out`, where $N$ is the size passed to `kfr_dct_create_plan_f64`.
 */
KFR_API_SPEC void kfr_dct_execute_inverse_f64(KFR_DCT_PLAN_F64* plan, kfr_f64* out, const kfr_f64* in,
                                              uint8_t* temp);

/**
 * Deletes a DCT plan.
 *
 * @param plan Pointer to the DCT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dct_delete_plan_f32(KFR_DCT_PLAN_F32* plan);

/**
 * Deletes a DCT plan.
 *
 * @param plan Pointer to the DCT plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_dct_delete_plan_f64(KFR_DCT_PLAN_F64* plan);

/**
 * Designs an all-pass biquad section (single precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_allpass_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32* sos);

/**
 * Designs an all-pass biquad section (double precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_allpass_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64* sos);

/**
 * Designs a low-pass biquad section (single precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_lowpass_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32* sos);

/**
 * Designs a low-pass biquad section (double precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_lowpass_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64* sos);

/**
 * Designs a high-pass biquad section (single precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_highpass_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32* sos);

/**
 * Designs a high-pass biquad section (double precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_highpass_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64* sos);

/**
 * Designs a band-pass biquad section (single precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_bandpass_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32* sos);

/**
 * Designs a band-pass biquad section (double precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_bandpass_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64* sos);

/**
 * Designs a notch biquad section (single precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_notch_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32* sos);

/**
 * Designs a notch biquad section (double precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_notch_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64* sos);

/**
 * Designs a peaking EQ biquad section (single precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param gain_db Gain in dB.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_peak_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32 gain_db, kfr_f32* sos);

/**
 * Designs a peaking EQ biquad section (double precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param Q Q factor.
 * @param gain_db Gain in dB.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_peak_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64 gain_db, kfr_f64* sos);

/**
 * Designs a low-shelf biquad section (single precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param gain_db Gain in dB.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_lowshelf_f32(kfr_f32 frequency, kfr_f32 gain_db, kfr_f32* sos);

/**
 * Designs a low-shelf biquad section (double precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param gain_db Gain in dB.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_lowshelf_f64(kfr_f64 frequency, kfr_f64 gain_db, kfr_f64* sos);

/**
 * Designs a high-shelf biquad section (single precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param gain_db Gain in dB.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_highshelf_f32(kfr_f32 frequency, kfr_f32 gain_db, kfr_f32* sos);

/**
 * Designs a high-shelf biquad section (double precision).
 * @param frequency Normalized frequency (Hz / sample rate).
 * @param gain_db Gain in dB.
 * @param sos Output buffer receiving 6 scalars `(a0, a1, a2, b0, b1, b2)`.
 */
KFR_API_SPEC void kfr_biquad_highshelf_f64(kfr_f64 frequency, kfr_f64 gain_db, kfr_f64* sos);

/**
 * Analog prototype family for IIR design. Values match the `kfr::zpk` prototype functions.
 */
typedef enum KFR_IIR_PROTOTYPE
{
    KFR_IIR_BUTTERWORTH = 1, /**< Butterworth. `order` must be 1..24. */
    KFR_IIR_BESSEL      = 2, /**< Bessel/Thomson. `order` must be 1..24. */
    KFR_IIR_CHEBYSHEV1  = 3, /**< Chebyshev type I. Uses `rp`. */
    KFR_IIR_CHEBYSHEV2  = 4, /**< Chebyshev type II. Uses `rs`. */
    KFR_IIR_ELLIPTIC    = 5 /**< Elliptic (Cauer). Uses `rp` and `rs`. Requires a build with Boost.Math. */
} KFR_IIR_PROTOTYPE;

/**
 * IIR filter response type.
 */
typedef enum KFR_IIR_RESPONSE
{
    KFR_IIR_LOWPASS  = 1, /**< Low-pass. Uses `frequency`. */
    KFR_IIR_HIGHPASS = 2, /**< High-pass. Uses `frequency`. */
    KFR_IIR_BANDPASS = 3, /**< Band-pass. Uses `frequency` and `high_frequency`. */
    KFR_IIR_BANDSTOP = 4 /**< Band-stop. Uses `frequency` and `high_frequency`. */
} KFR_IIR_RESPONSE;

/**
 * Designs a digital IIR filter and returns it as second-order sections (single precision).
 *
 * The design runs the analog prototype, frequency transform, bilinear transform and
 * conversion to SOS. The section count is returned, so call with `sos` set to `NULL` to
 * query the number of sections before allocating the output buffer.
 *
 * @param prototype Analog prototype family (see `KFR_IIR_PROTOTYPE`).
 * @param response Filter response type (see `KFR_IIR_RESPONSE`).
 * @param order Prototype order. Must be 1..24 for Butterworth and Bessel, and at least 1 otherwise.
 * @param rp Passband ripple in dB. Used by Chebyshev I and elliptic only. Must be greater than zero.
 * @param rs Stopband attenuation in dB. Used by Chebyshev II and elliptic only. Must be greater than zero.
 * @param frequency Cutoff frequency in Hz for low-pass and high-pass, or lower edge for band-pass and
 *        band-stop. Must satisfy `0 < frequency < fs / 2`.
 * @param high_frequency Upper edge in Hz for band-pass and band-stop. Must satisfy
 *        `frequency < high_frequency < fs / 2`. Ignored otherwise.
 * @param fs Sample rate in Hz. Must be greater than zero.
 * @param sos Output buffer receiving `6 * sections` scalars `(a0, a1, a2, b0, b1, b2)`, one group per
 * section. May be `NULL`. Nothing is written if `NULL` or if `sos_capacity` is smaller than the returned
 * count.
 * @param sos_capacity Capacity of `sos` in biquad sections.
 * @return Number of biquad sections, or 0 on error (see `kfr_last_error()`).
 */
KFR_API_SPEC size_t kfr_iir_design_f32(KFR_IIR_PROTOTYPE prototype, KFR_IIR_RESPONSE response, int order,
                                       kfr_f32 rp, kfr_f32 rs, kfr_f32 frequency, kfr_f32 high_frequency,
                                       kfr_f32 fs, kfr_f32* sos, size_t sos_capacity);

/**
 * Designs a digital IIR filter and returns it as second-order sections (double precision).
 *
 * See `kfr_iir_design_f32()` for parameter details.
 *
 * @param prototype Analog prototype family (see `KFR_IIR_PROTOTYPE`).
 * @param response Filter response type (see `KFR_IIR_RESPONSE`).
 * @param order Prototype order. Must be 1..24 for Butterworth and Bessel, and at least 1 otherwise.
 * @param rp Passband ripple in dB. Used by Chebyshev I and elliptic only. Must be greater than zero.
 * @param rs Stopband attenuation in dB. Used by Chebyshev II and elliptic only. Must be greater than zero.
 * @param frequency Cutoff frequency in Hz, or lower edge for band-pass and band-stop.
 * @param high_frequency Upper edge in Hz for band-pass and band-stop. Ignored otherwise.
 * @param fs Sample rate in Hz. Must be greater than zero.
 * @param sos Output buffer receiving `6 * sections` scalars `(a0, a1, a2, b0, b1, b2)`, one group per
 * section. May be `NULL`. Nothing is written if `NULL` or if `sos_capacity` is smaller than the returned
 * count.
 * @param sos_capacity Capacity of `sos` in biquad sections.
 * @return Number of biquad sections, or 0 on error (see `kfr_last_error()`).
 */
KFR_API_SPEC size_t kfr_iir_design_f64(KFR_IIR_PROTOTYPE prototype, KFR_IIR_RESPONSE response, int order,
                                       kfr_f64 rp, kfr_f64 rs, kfr_f64 frequency, kfr_f64 high_frequency,
                                       kfr_f64 fs, kfr_f64* sos, size_t sos_capacity);

/**
 * Window function type. Values match the `kfr::window_type` enumeration.
 */
typedef enum KFR_WINDOW_TYPE
{
    KFR_WINDOW_RECTANGULAR     = 1, /**< Rectangular window. */
    KFR_WINDOW_TRIANGULAR      = 2, /**< Triangular window. */
    KFR_WINDOW_BARTLETT        = 3, /**< Bartlett window. */
    KFR_WINDOW_COSINE          = 4, /**< Cosine window. */
    KFR_WINDOW_HANN            = 5, /**< Hann window. */
    KFR_WINDOW_BARTLETT_HANN   = 6, /**< Bartlett-Hann window. */
    KFR_WINDOW_HAMMING         = 7, /**< Hamming window. Uses `param` as alpha. */
    KFR_WINDOW_BOHMAN          = 8, /**< Bohman window. */
    KFR_WINDOW_BLACKMAN        = 9, /**< Blackman window. Uses `param` as alpha. */
    KFR_WINDOW_BLACKMAN_HARRIS = 10, /**< Blackman-Harris window. */
    KFR_WINDOW_KAISER          = 11, /**< Kaiser window. Uses `param` as beta. */
    KFR_WINDOW_FLATTOP         = 12, /**< Flat-top window. */
    KFR_WINDOW_GAUSSIAN        = 13, /**< Gaussian window. Uses `param` as alpha. */
    KFR_WINDOW_LANCZOS         = 14, /**< Lanczos window. */
    KFR_WINDOW_COSINE_NP       = 15, /**< Non-periodic cosine window. */
    KFR_WINDOW_PLANCK_TAPER    = 16, /**< Planck-taper window. Uses `param` as epsilon. */
    KFR_WINDOW_TUKEY           = 17 /**< Tukey window. Uses `param` as alpha. */
} KFR_WINDOW_TYPE;

/**
 * Generates a window function (single precision).
 *
 * @param type Window type.
 * @param size Number of samples to generate.
 * @param param Window-specific parameter (alpha, beta or epsilon, see `KFR_WINDOW_TYPE`).
 *        Ignored by windows without a parameter.
 * @param symmetric Non-zero for a symmetric window, zero for a periodic window.
 * @param output Output buffer receiving `size` samples. Must hold at least `size` elements.
 */
KFR_API_SPEC void kfr_window_f32(KFR_WINDOW_TYPE type, size_t size, kfr_f32 param, kfr_bool symmetric,
                                 kfr_f32* output);

/**
 * Generates a window function (double precision).
 *
 * @param type Window type.
 * @param size Number of samples to generate.
 * @param param Window-specific parameter (alpha, beta or epsilon, see `KFR_WINDOW_TYPE`).
 *        Ignored by windows without a parameter.
 * @param symmetric Non-zero for a symmetric window, zero for a periodic window.
 * @param output Output buffer receiving `size` samples. Must hold at least `size` elements.
 */
KFR_API_SPEC void kfr_window_f64(KFR_WINDOW_TYPE type, size_t size, kfr_f64 param, kfr_bool symmetric,
                                 kfr_f64* output);

/**
 * Response type for FIR design. Values match the `KFR_IIR_RESPONSE` enumeration.
 */
typedef enum KFR_FIR_RESPONSE
{
    KFR_FIR_LOWPASS  = 1, /**< Low-pass. Uses `frequency`. */
    KFR_FIR_HIGHPASS = 2, /**< High-pass. Uses `frequency`. */
    KFR_FIR_BANDPASS = 3, /**< Band-pass. Uses `frequency` and `high_frequency`. */
    KFR_FIR_BANDSTOP = 4 /**< Band-stop. Uses `frequency` and `high_frequency`. */
} KFR_FIR_RESPONSE;

/**
 * Designs a linear-phase FIR filter by the window method (single precision).
 *
 * The taps are the windowed ideal impulse response. The window is always symmetric, which makes the
 * taps linear-phase. The taps can be passed to `kfr_filter_create_fir_plan_f32()`.
 *
 * @param response Filter response type (see `KFR_FIR_RESPONSE`).
 * @param type Window function applied to the impulse response (see `KFR_WINDOW_TYPE`).
 * @param param Window-specific parameter (alpha, beta or epsilon, see `KFR_WINDOW_TYPE`).
 *        Ignored by windows without a parameter.
 * @param frequency Cutoff frequency in Hz for low-pass and high-pass, or lower edge for band-pass and
 *        band-stop. Must satisfy `0 < frequency < fs / 2`.
 * @param high_frequency Upper edge in Hz for band-pass and band-stop. Must satisfy
 *        `frequency < high_frequency < fs / 2`. Ignored otherwise.
 * @param fs Sample rate in Hz. Must be greater than zero.
 * @param normalize Non-zero to normalize the taps the same way as the C++ `kfr::fir_*()` family does.
 *        Low-pass and band-stop then have unit DC gain. High-pass and band-pass use the C++ scaling.
 * @param taps Output buffer receiving `size` taps. Must hold at least `size` elements.
 * @param size Number of taps. Must be greater than zero. The filter order is `size - 1`.
 * @return Non-zero on success, or 0 on error (see `kfr_last_error()`).
 */
KFR_API_SPEC kfr_bool kfr_fir_design_f32(KFR_FIR_RESPONSE response, KFR_WINDOW_TYPE type, kfr_f32 param,
                                         kfr_f32 frequency, kfr_f32 high_frequency, kfr_f32 fs,
                                         kfr_bool normalize, kfr_f32* taps, size_t size);

/**
 * Designs a linear-phase FIR filter by the window method (double precision).
 *
 * See `kfr_fir_design_f32()` for parameter details.
 *
 * @param response Filter response type (see `KFR_FIR_RESPONSE`).
 * @param type Window function applied to the impulse response (see `KFR_WINDOW_TYPE`).
 * @param param Window-specific parameter (alpha, beta or epsilon, see `KFR_WINDOW_TYPE`).
 * @param frequency Cutoff frequency in Hz, or lower edge for band-pass and band-stop.
 * @param high_frequency Upper edge in Hz for band-pass and band-stop. Ignored otherwise.
 * @param fs Sample rate in Hz. Must be greater than zero.
 * @param normalize Non-zero to normalize the taps the same way as the C++ `kfr::fir_*()` family does.
 *        Low-pass and band-stop then have unit DC gain. High-pass and band-pass use the C++ scaling.
 * @param taps Output buffer receiving `size` taps. Must hold at least `size` elements.
 * @param size Number of taps. Must be greater than zero. The filter order is `size - 1`.
 * @return Non-zero on success, or 0 on error (see `kfr_last_error()`).
 */
KFR_API_SPEC kfr_bool kfr_fir_design_f64(KFR_FIR_RESPONSE response, KFR_WINDOW_TYPE type, kfr_f64 param,
                                         kfr_f64 frequency, kfr_f64 high_frequency, kfr_f64 fs,
                                         kfr_bool normalize, kfr_f64* taps, size_t size);

/**
 * Creates a FIR filter plan (single precision).
 *
 * @param taps Pointer to filter taps.
 * @param size Number of filter taps.
 * @return Pointer to the created FIR filter plan, or `NULL` on failure.
 *         Use `kfr_filter_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_fir_plan_f32(const kfr_f32* taps, size_t size);

/**
 * Creates a FIR filter plan (double precision).
 *
 * @param taps Pointer to filter taps.
 * @param size Number of filter taps.
 * @return Pointer to the created FIR filter plan, or `NULL` on failure.
 *         Use `kfr_filter_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_fir_plan_f64(const kfr_f64* taps, size_t size);

/**
 * Creates a convolution filter plan (single precision).
 *
 * @param taps Pointer to filter taps.
 * @param size Number of filter taps.
 * @param block_size Size of the processing block. Must be a power of two. If zero,
 *        a default of 1024 is used.
 * @return Pointer to the created convolution filter plan, or `NULL` on failure.
 *         Use `kfr_filter_delete_plan_f32` to free.
 * @note Mathematically, this produces the same result as an FIR filter, but it uses
 * the FFT overlap-add technique internally to improve performance with larger filter
 * lengths.
 */
KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_convolution_plan_f32(const kfr_f32* taps, size_t size,
                                                                    size_t block_size);

/**
 * Creates a convolution filter plan (double precision).
 *
 * @param taps Pointer to filter taps.
 * @param size Number of filter taps.
 * @param block_size Size of the processing block. Must be a power of two. If zero,
 *        a default of 1024 is used.
 * @return Pointer to the created convolution filter plan, or `NULL` on failure.
 *         Use `kfr_filter_delete_plan_f64` to free.
 * @note Mathematically, this produces the same result as an FIR filter, but it uses
 * the FFT overlap-add technique internally to improve performance with larger filter
 * lengths.
 */
KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_convolution_plan_f64(const kfr_f64* taps, size_t size,
                                                                    size_t block_size);

/**
 * Creates an IIR filter plan (single precision) from a cascade of second-order
 * sections.
 *
 * @param sos Pointer to an array of `sos_count` second-order sections. Each section
 *        is laid out as 6 consecutive scalars `(a0, a1, a2, b0, b1, b2)` matching
 *        the `biquad_section` coefficient order.
 * @param sos_count Number of second-order sections.
 * @return Pointer to the created IIR filter plan, or `NULL` on failure.
 *         Use `kfr_filter_delete_plan_f32` to free.
 */
KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_iir_plan_f32(const kfr_f32* sos, size_t sos_count);

/**
 * Creates an IIR filter plan (double precision) from a cascade of second-order
 * sections.
 *
 * @param sos Pointer to an array of `sos_count` second-order sections. Each section
 *        is laid out as 6 consecutive scalars `(a0, a1, a2, b0, b1, b2)` matching
 *        the `biquad_section` coefficient order.
 * @param sos_count Number of second-order sections.
 * @return Pointer to the created IIR filter plan, or `NULL` on failure.
 *         Use `kfr_filter_delete_plan_f64` to free.
 */
KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_iir_plan_f64(const kfr_f64* sos, size_t sos_count);

/**
 * Processes input data with a filter.
 *
 * @param plan Pointer to the filter plan.
 * @param output Pointer to output data. May point to the same memory as `input` for
 *        in-place execution.
 * @param input Pointer to input data.
 * @param size Number of samples to process.
 */
KFR_API_SPEC void kfr_filter_process_f32(KFR_FILTER_F32* plan, kfr_f32* output, const kfr_f32* input,
                                         size_t size);

/**
 * Processes input data with a filter.
 *
 * @param plan Pointer to the filter plan.
 * @param output Pointer to output data. May point to the same memory as `input` for
 *        in-place execution.
 * @param input Pointer to input data.
 * @param size Number of samples to process.
 */
KFR_API_SPEC void kfr_filter_process_f64(KFR_FILTER_F64* plan, kfr_f64* output, const kfr_f64* input,
                                         size_t size);

/**
 * Resets the internal state of a filter plan, including its delay line.
 *
 * @param plan Pointer to the filter plan.
 */
KFR_API_SPEC void kfr_filter_reset_f32(KFR_FILTER_F32* plan);

/**
 * Resets the internal state of a filter plan, including its delay line.
 *
 * @param plan Pointer to the filter plan.
 */
KFR_API_SPEC void kfr_filter_reset_f64(KFR_FILTER_F64* plan);

/**
 * Deletes a filter plan.
 *
 * @param plan Pointer to the filter plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_filter_delete_plan_f32(KFR_FILTER_F32* plan);

/**
 * Deletes a filter plan.
 *
 * @param plan Pointer to the filter plan. May be `NULL`.
 */
KFR_API_SPEC void kfr_filter_delete_plan_f64(KFR_FILTER_F64* plan);

/**
 * Computes the linear convolution of two signals (single precision).
 *
 * @param out Output buffer of `a_size + b_size - 1` samples. Nothing is written if either input is empty.
 * @param a First input signal.
 * @param a_size Number of samples in `a`.
 * @param b Second input signal.
 * @param b_size Number of samples in `b`.
 */
KFR_API_SPEC void kfr_convolve_f32(kfr_f32* out, const kfr_f32* a, size_t a_size, const kfr_f32* b,
                                   size_t b_size);

/**
 * Computes the linear convolution of two signals (double precision).
 *
 * @param out Output buffer of `a_size + b_size - 1` samples. Nothing is written if either input is empty.
 * @param a First input signal.
 * @param a_size Number of samples in `a`.
 * @param b Second input signal.
 * @param b_size Number of samples in `b`.
 */
KFR_API_SPEC void kfr_convolve_f64(kfr_f64* out, const kfr_f64* a, size_t a_size, const kfr_f64* b,
                                   size_t b_size);

/**
 * Applies zero-phase forward-backward IIR filtering in place (single precision).
 *
 * @param sos Pointer to `sos_count` second-order sections, 6 scalars each `(a0, a1, a2, b0, b1, b2)`.
 * @param sos_count Number of second-order sections.
 * @param data Signal to filter, modified in place.
 * @param size Number of samples in `data`.
 */
KFR_API_SPEC void kfr_filtfilt_f32(const kfr_f32* sos, size_t sos_count, kfr_f32* data, size_t size);

/**
 * Applies zero-phase forward-backward IIR filtering in place (double precision).
 *
 * @param sos Pointer to `sos_count` second-order sections, 6 scalars each `(a0, a1, a2, b0, b1, b2)`.
 * @param sos_count Number of second-order sections.
 * @param data Signal to filter, modified in place.
 * @param size Number of samples in `data`.
 */
KFR_API_SPEC void kfr_filtfilt_f64(const kfr_f64* sos, size_t sos_count, kfr_f64* data, size_t size);

#ifdef __cplusplus
}
#endif
