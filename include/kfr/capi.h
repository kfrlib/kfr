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

#ifdef __cplusplus
}
#endif
