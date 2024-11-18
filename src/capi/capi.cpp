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
#include <variant>
#define KFR_NO_C_COMPLEX_TYPES 1

#include <kfr/capi.h>
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>
#include <kfr/multiarch.h>

namespace kfr
{
static thread_local std::array<char, 256> error;

void reset_error() { std::fill(error.begin(), error.end(), 0); }
void set_error(std::string_view s)
{
    size_t n = std::min(s.size(), error.size() - 1);
    auto end = std::copy_n(s.begin(), n, error.begin());
    std::fill(end, error.end(), 0);
}

template <typename Fn, typename R = std::invoke_result_t<Fn>, typename T>
static R try_fn(Fn&& fn, T fallback)
{
    try
    {
        auto result = fn();
        reset_error();
        return result;
    }
    catch (std::exception& e)
    {
        set_error(e.what());
        return fallback;
    }
    catch (...)
    {
        set_error("(unknown exception)");
        return fallback;
    }
}

template <typename Fn>
static void try_fn(Fn&& fn)
{
    try
    {
        fn();
        reset_error();
    }
    catch (std::exception& e)
    {
        set_error(e.what());
    }
    catch (...)
    {
        set_error("(unknown exception)");
    }
}

template <typename T>
class var_dft_plan
{
public:
    virtual ~var_dft_plan() {}
    virtual void dump()                             = 0;
    virtual size_t size()                           = 0;
    virtual size_t temp_size()                      = 0;
    virtual void execute(T*, const T*, u8*)         = 0;
    virtual void execute_inverse(T*, const T*, u8*) = 0;
};

static shape<dynamic_shape> init_shape(size_t dims, const unsigned* shape_)
{
    shape<dynamic_shape> sh(dims);
    for (size_t i = 0; i < dims; ++i)
    {
        sh[i] = shape_[i];
    }
    return sh;
}

template <typename T, size_t Dims>
struct var_dft_plan_select
{
    using complex = dft_plan_md<T, dynamic_shape>;
    using real    = dft_plan_md_real<T, dynamic_shape>;

    static size_t size(const complex& plan) { return plan.size.product(); }
    static size_t size(const real& plan) { return plan.size.product(); }
};
template <typename T>
struct var_dft_plan_select<T, 1>
{
    using complex = dft_plan<T>;
    using real    = dft_plan_real<T>;

    static size_t size(const complex& plan) { return plan.size; }
    static size_t size(const real& plan) { return plan.size; }
};

template <typename T, size_t Dims>
class var_dft_plan_impl final : public var_dft_plan<T>
{
public:
    template <typename... Args>
    CMT_ALWAYS_INLINE var_dft_plan_impl(Args&&... args) : plan(std::forward<Args>(args)...)
    {
    }
    typename var_dft_plan_select<T, Dims>::complex plan;
    void dump() { plan.dump(); }
    size_t size() { return var_dft_plan_select<T, Dims>::size(plan); }
    size_t temp_size() { return plan.temp_size; }
    void execute(T* out, const T* in, u8* temp)
    {
        plan.execute(reinterpret_cast<complex<T>*>(out), reinterpret_cast<const complex<T>*>(in), temp,
                     cfalse);
    }
    void execute_inverse(T* out, const T* in, u8* temp)
    {
        plan.execute(reinterpret_cast<complex<T>*>(out), reinterpret_cast<const complex<T>*>(in), temp,
                     ctrue);
    }
};

template <typename T, size_t Dims>
class var_dft_plan_real_impl final : public var_dft_plan<T>
{
public:
    template <typename... Args>
    CMT_ALWAYS_INLINE var_dft_plan_real_impl(Args&&... args) : plan(std::forward<Args>(args)...)
    {
    }
    typename var_dft_plan_select<T, Dims>::real plan;
    void dump() { plan.dump(); }
    size_t size() { return var_dft_plan_select<T, Dims>::size(plan); }
    size_t temp_size() { return plan.temp_size; }
    void execute(T* out, const T* in, u8* temp)
    {
        plan.execute(reinterpret_cast<complex<T>*>(out), reinterpret_cast<const T*>(in), temp, cfalse);
    }
    void execute_inverse(T* out, const T* in, u8* temp)
    {
        plan.execute(reinterpret_cast<T*>(out), reinterpret_cast<const complex<T>*>(in), temp, ctrue);
    }
};

extern "C"
{
KFR_API_SPEC const char* kfr_version_string()
{
    return "KFR " KFR_VERSION_STRING KFR_DEBUG_STR " " KFR_ENABLED_ARCHS_LIST " " CMT_ARCH_BITNESS_NAME
           " (" CMT_COMPILER_FULL_NAME "/" CMT_OS_NAME ")" KFR_BUILD_DETAILS_1 KFR_BUILD_DETAILS_2;
}
KFR_API_SPEC uint32_t kfr_version() { return KFR_VERSION; }
KFR_API_SPEC const char* kfr_enabled_archs() { return KFR_ENABLED_ARCHS_LIST; }
KFR_API_SPEC int kfr_current_arch() { return static_cast<int>(get_cpu()); }

KFR_API_SPEC const char* kfr_last_error() { return error.data(); }

KFR_API_SPEC void* kfr_allocate(size_t size) { return details::aligned_malloc(size, KFR_DEFAULT_ALIGNMENT); }
KFR_API_SPEC void* kfr_allocate_aligned(size_t size, size_t alignment)
{
    return details::aligned_malloc(size, alignment);
}
KFR_API_SPEC void kfr_deallocate(void* ptr) { return details::aligned_free(ptr); }
KFR_API_SPEC size_t kfr_allocated_size(void* ptr) { return details::aligned_size(ptr); }

KFR_API_SPEC void* kfr_add_ref(void* ptr)
{
    details::aligned_add_ref(ptr);
    return ptr;
}
KFR_API_SPEC void kfr_release(void* ptr) { details::aligned_release(ptr); }

KFR_API_SPEC void* kfr_reallocate(void* ptr, size_t new_size)
{
    return details::aligned_reallocate(ptr, new_size, KFR_DEFAULT_ALIGNMENT);
}
KFR_API_SPEC void* kfr_reallocate_aligned(void* ptr, size_t new_size, size_t alignment)
{
    return details::aligned_reallocate(ptr, new_size, alignment);
}

KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_plan_f32(size_t size)
{
    return try_fn([&]()
                  { return reinterpret_cast<KFR_DFT_PLAN_F32*>(new var_dft_plan_impl<float, 1>(size)); },
                  nullptr);
}

KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_2d_plan_f32(size_t size1, size_t size2)
{
    return try_fn(
        [&]() {
            return reinterpret_cast<KFR_DFT_PLAN_F32*>(
                new var_dft_plan_impl<float, 2>(shape{ size1, size2 }));
        },
        nullptr);
}
KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_3d_plan_f32(size_t size1, size_t size2, size_t size3)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_PLAN_F32*>(
                new var_dft_plan_impl<float, 3>(shape{ size1, size2, size3 }));
        },
        nullptr);
}
KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_md_plan_f32(size_t dims, const unsigned* shape)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_PLAN_F32*>(
                new var_dft_plan_impl<float, dynamic_shape>(init_shape(dims, shape)));
        },
        nullptr);
}
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_plan_f64(size_t size)
{
    return try_fn([&]()
                  { return reinterpret_cast<KFR_DFT_PLAN_F64*>(new var_dft_plan_impl<double, 1>(size)); },
                  nullptr);
}
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_2d_plan_f64(size_t size1, size_t size2)
{
    return try_fn(
        [&]() {
            return reinterpret_cast<KFR_DFT_PLAN_F64*>(
                new var_dft_plan_impl<double, 2>(shape{ size1, size2 }));
        },
        nullptr);
}
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_3d_plan_f64(size_t size1, size_t size2, size_t size3)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_PLAN_F64*>(
                new var_dft_plan_impl<double, 3>(shape{ size1, size2, size3 }));
        },
        nullptr);
}
KFR_API_SPEC KFR_DFT_PLAN_F64* kfr_dft_create_md_plan_f64(size_t dims, const unsigned* shape)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_PLAN_F64*>(
                new var_dft_plan_impl<double, dynamic_shape>(init_shape(dims, shape)));
        },
        nullptr);
}

KFR_API_SPEC void kfr_dft_dump_f32(KFR_DFT_PLAN_F32* plan)
{
    try_fn([&] { reinterpret_cast<var_dft_plan<float>*>(plan)->dump(); });
}
KFR_API_SPEC void kfr_dft_dump_f64(KFR_DFT_PLAN_F64* plan)
{
    try_fn([&] { reinterpret_cast<var_dft_plan<double>*>(plan)->dump(); });
}

KFR_API_SPEC size_t kfr_dft_get_size_f32(KFR_DFT_PLAN_F32* plan)
{
    return try_fn([&]() { return reinterpret_cast<var_dft_plan<float>*>(plan)->size(); }, 0);
}
KFR_API_SPEC size_t kfr_dft_get_size_f64(KFR_DFT_PLAN_F64* plan)
{
    return try_fn([&]() { return reinterpret_cast<var_dft_plan<double>*>(plan)->size(); }, 0);
}

KFR_API_SPEC size_t kfr_dft_get_temp_size_f32(KFR_DFT_PLAN_F32* plan)
{
    return try_fn([&]() { return reinterpret_cast<var_dft_plan<float>*>(plan)->temp_size(); }, 0);
}
KFR_API_SPEC size_t kfr_dft_get_temp_size_f64(KFR_DFT_PLAN_F64* plan)
{
    return try_fn([&]() { return reinterpret_cast<var_dft_plan<double>*>(plan)->temp_size(); }, 0);
}

KFR_API_SPEC void kfr_dft_execute_f32(KFR_DFT_PLAN_F32* plan, kfr_c32* out, const kfr_c32* in, uint8_t* temp)
{
    try_fn(
        [&]()
        {
            reinterpret_cast<var_dft_plan<float>*>(plan)->execute(reinterpret_cast<float*>(out),
                                                                  reinterpret_cast<const float*>(in), temp);
        });
}
KFR_API_SPEC void kfr_dft_execute_f64(KFR_DFT_PLAN_F64* plan, kfr_c64* out, const kfr_c64* in, uint8_t* temp)
{
    try_fn(
        [&]()
        {
            reinterpret_cast<var_dft_plan<double>*>(plan)->execute(reinterpret_cast<double*>(out),
                                                                   reinterpret_cast<const double*>(in), temp);
        });
}
KFR_API_SPEC void kfr_dft_execute_inverse_f32(KFR_DFT_PLAN_F32* plan, kfr_c32* out, const kfr_c32* in,
                                              uint8_t* temp)
{
    try_fn(
        [&]()
        {
            reinterpret_cast<var_dft_plan<float>*>(plan)->execute_inverse(
                reinterpret_cast<float*>(out), reinterpret_cast<const float*>(in), temp);
        });
}
KFR_API_SPEC void kfr_dft_execute_inverse_f64(KFR_DFT_PLAN_F64* plan, kfr_c64* out, const kfr_c64* in,
                                              uint8_t* temp)
{
    try_fn(
        [&]()
        {
            reinterpret_cast<var_dft_plan<double>*>(plan)->execute_inverse(
                reinterpret_cast<double*>(out), reinterpret_cast<const double*>(in), temp);
        });
}

KFR_API_SPEC void kfr_dft_delete_plan_f32(KFR_DFT_PLAN_F32* plan)
{
    try_fn([&]() { delete reinterpret_cast<var_dft_plan<float>*>(plan); });
}
KFR_API_SPEC void kfr_dft_delete_plan_f64(KFR_DFT_PLAN_F64* plan)
{
    try_fn([&]() { delete reinterpret_cast<var_dft_plan<double>*>(plan); });
}

// Real DFT plans

KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_plan_f32(size_t size, KFR_DFT_PACK_FORMAT pack_format)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_REAL_PLAN_F32*>(
                new var_dft_plan_real_impl<float, 1>(size, static_cast<dft_pack_format>(pack_format)));
        },
        nullptr);
}

KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_2d_plan_f32(size_t size1, size_t size2,
                                                                    bool real_out_is_enough)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_REAL_PLAN_F32*>(
                new var_dft_plan_real_impl<float, 2>(shape{ size1, size2 }, real_out_is_enough));
        },
        nullptr);
}
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_3d_plan_f32(size_t size1, size_t size2, size_t size3,
                                                                    bool real_out_is_enough)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_REAL_PLAN_F32*>(
                new var_dft_plan_real_impl<float, 3>(shape{ size1, size2, size3 }, real_out_is_enough));
        },
        nullptr);
}
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_md_plan_f32(size_t dims, const unsigned* shape,
                                                                    bool real_out_is_enough)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_REAL_PLAN_F32*>(
                new var_dft_plan_real_impl<float, dynamic_shape>(init_shape(dims, shape), real_out_is_enough));
        },
        nullptr);
}

KFR_API_SPEC KFR_DFT_REAL_PLAN_F64* kfr_dft_real_create_plan_f64(size_t size, KFR_DFT_PACK_FORMAT pack_format)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_REAL_PLAN_F64*>(
                new var_dft_plan_real_impl<double, 1>(size, static_cast<dft_pack_format>(pack_format)));
        },
        nullptr);
}

KFR_API_SPEC void kfr_dft_real_dump_f32(KFR_DFT_REAL_PLAN_F32* plan)
{
    try_fn([&]() { reinterpret_cast<var_dft_plan<float>*>(plan)->dump(); });
}
KFR_API_SPEC void kfr_dft_real_dump_f64(KFR_DFT_REAL_PLAN_F64* plan)
{
    try_fn([&]() { reinterpret_cast<var_dft_plan<double>*>(plan)->dump(); });
}

KFR_API_SPEC size_t kfr_dft_real_get_size_f32(KFR_DFT_REAL_PLAN_F32* plan)
{
    return try_fn([&]() { return reinterpret_cast<var_dft_plan<float>*>(plan)->size(); }, 0);
}
KFR_API_SPEC size_t kfr_dft_real_get_size_f64(KFR_DFT_REAL_PLAN_F64* plan)
{
    return try_fn([&]() { return reinterpret_cast<var_dft_plan<double>*>(plan)->size(); }, 0);
}

KFR_API_SPEC size_t kfr_dft_real_get_temp_size_f32(KFR_DFT_REAL_PLAN_F32* plan)
{
    return try_fn([&]() { return reinterpret_cast<var_dft_plan<float>*>(plan)->temp_size(); }, 0);
}
KFR_API_SPEC size_t kfr_dft_real_get_temp_size_f64(KFR_DFT_REAL_PLAN_F64* plan)
{
    return try_fn([&]() { return reinterpret_cast<var_dft_plan<double>*>(plan)->temp_size(); }, 0);
}

KFR_API_SPEC void kfr_dft_real_execute_f32(KFR_DFT_REAL_PLAN_F32* plan, kfr_c32* out, const float* in,
                                           uint8_t* temp)
{
    try_fn(
        [&]()
        { reinterpret_cast<var_dft_plan<float>*>(plan)->execute(reinterpret_cast<float*>(out), in, temp); });
}
KFR_API_SPEC void kfr_dft_real_execute_f64(KFR_DFT_REAL_PLAN_F64* plan, kfr_c64* out, const double* in,
                                           uint8_t* temp)
{
    try_fn(
        [&]() {
            reinterpret_cast<var_dft_plan<double>*>(plan)->execute(reinterpret_cast<double*>(out), in, temp);
        });
}
KFR_API_SPEC void kfr_dft_real_execute_inverse_f32(KFR_DFT_REAL_PLAN_F32* plan, float* out, const kfr_c32* in,
                                                   uint8_t* temp)
{
    try_fn(
        [&]()
        {
            reinterpret_cast<var_dft_plan<float>*>(plan)->execute_inverse(
                out, reinterpret_cast<const float*>(in), temp);
        });
}
KFR_API_SPEC void kfr_dft_real_execute_inverse_f64(KFR_DFT_REAL_PLAN_F64* plan, double* out,
                                                   const kfr_c64* in, uint8_t* temp)
{
    try_fn(
        [&]()
        {
            reinterpret_cast<var_dft_plan<double>*>(plan)->execute_inverse(
                out, reinterpret_cast<const double*>(in), temp);
        });
}

KFR_API_SPEC void kfr_dft_real_delete_plan_f32(KFR_DFT_REAL_PLAN_F32* plan)
{
    try_fn([&]() { delete reinterpret_cast<var_dft_plan<float>*>(plan); });
}
KFR_API_SPEC void kfr_dft_real_delete_plan_f64(KFR_DFT_REAL_PLAN_F64* plan)
{
    try_fn([&]() { delete reinterpret_cast<var_dft_plan<double>*>(plan); });
}

// Discrete Cosine Transform

KFR_API_SPEC KFR_DCT_PLAN_F32* kfr_dct_create_plan_f32(size_t size)
{
    return try_fn([&]() { return reinterpret_cast<KFR_DCT_PLAN_F32*>(new dct_plan<float>(size)); }, nullptr);
}
KFR_API_SPEC KFR_DCT_PLAN_F64* kfr_dct_create_plan_f64(size_t size)
{
    return try_fn([&]() { return reinterpret_cast<KFR_DCT_PLAN_F64*>(new dct_plan<double>(size)); }, nullptr);
}

KFR_API_SPEC void kfr_dct_dump_f32(KFR_DCT_PLAN_F32* plan)
{
    try_fn([&]() { reinterpret_cast<dct_plan<float>*>(plan)->dump(); });
}
KFR_API_SPEC void kfr_dct_dump_f64(KFR_DCT_PLAN_F64* plan)
{
    try_fn([&]() { reinterpret_cast<dct_plan<double>*>(plan)->dump(); });
}

KFR_API_SPEC size_t kfr_dct_get_size_f32(KFR_DCT_PLAN_F32* plan)
{
    return try_fn([&]() { return reinterpret_cast<dft_plan<float>*>(plan)->size; }, 0);
}
KFR_API_SPEC size_t kfr_dct_get_size_f64(KFR_DCT_PLAN_F64* plan)
{
    return try_fn([&]() { return reinterpret_cast<dft_plan<double>*>(plan)->size; }, 0);
}

KFR_API_SPEC size_t kfr_dct_get_temp_size_f32(KFR_DCT_PLAN_F32* plan)
{
    return try_fn([&]() { return reinterpret_cast<dft_plan<float>*>(plan)->temp_size; }, 0);
}
KFR_API_SPEC size_t kfr_dct_get_temp_size_f64(KFR_DCT_PLAN_F64* plan)
{
    return try_fn([&]() { return reinterpret_cast<dft_plan<double>*>(plan)->temp_size; }, 0);
}

KFR_API_SPEC void kfr_dct_execute_f32(KFR_DCT_PLAN_F32* plan, float* out, const float* in, uint8_t* temp)
{
    try_fn([&]() { reinterpret_cast<dct_plan<float>*>(plan)->execute(out, in, temp, cfalse); });
}
KFR_API_SPEC void kfr_dct_execute_f64(KFR_DCT_PLAN_F64* plan, double* out, const double* in, uint8_t* temp)
{
    try_fn([&]() { reinterpret_cast<dct_plan<double>*>(plan)->execute(out, in, temp, cfalse); });
}
KFR_API_SPEC void kfr_dct_execute_inverse_f32(KFR_DCT_PLAN_F32* plan, float* out, const float* in,
                                              uint8_t* temp)
{
    try_fn([&]() { reinterpret_cast<dct_plan<float>*>(plan)->execute(out, in, temp, ctrue); });
}
KFR_API_SPEC void kfr_dct_execute_inverse_f64(KFR_DCT_PLAN_F64* plan, double* out, const double* in,
                                              uint8_t* temp)
{
    try_fn([&]() { reinterpret_cast<dct_plan<double>*>(plan)->execute(out, in, temp, ctrue); });
}

KFR_API_SPEC void kfr_dct_delete_plan_f32(KFR_DCT_PLAN_F32* plan)
{
    try_fn([&]() { delete reinterpret_cast<dct_plan<float>*>(plan); });
}
KFR_API_SPEC void kfr_dct_delete_plan_f64(KFR_DCT_PLAN_F64* plan)
{
    try_fn([&]() { delete reinterpret_cast<dct_plan<double>*>(plan); });
}

// Filters

KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_fir_plan_f32(const kfr_f32* taps, size_t size)
{
    return try_fn(
        [&]()
        { return reinterpret_cast<KFR_FILTER_F32*>(new fir_filter<float>(make_univector(taps, size))); },
        nullptr);
}
KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_fir_plan_f64(const kfr_f64* taps, size_t size)
{
    return try_fn(
        [&]()
        { return reinterpret_cast<KFR_FILTER_F64*>(new fir_filter<double>(make_univector(taps, size))); },
        nullptr);
}

KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_convolution_plan_f32(const kfr_f32* taps, size_t size,
                                                                    size_t block_size)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_FILTER_F32*>(
                new convolve_filter<float>(make_univector(taps, size), block_size ? block_size : 1024));
        },
        nullptr);
}
KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_convolution_plan_f64(const kfr_f64* taps, size_t size,
                                                                    size_t block_size)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_FILTER_F64*>(
                new convolve_filter<double>(make_univector(taps, size), block_size ? block_size : 1024));
        },
        nullptr);
}

KFR_API_SPEC KFR_FILTER_F32* kfr_filter_create_iir_plan_f32(const kfr_f32* sos, size_t sos_count)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_FILTER_F32*>(new iir_filter<float>(
                iir_params{ reinterpret_cast<const biquad_section<float>*>(sos), sos_count }));
        },
        nullptr);
}
KFR_API_SPEC KFR_FILTER_F64* kfr_filter_create_iir_plan_f64(const kfr_f64* sos, size_t sos_count)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_FILTER_F64*>(new iir_filter<double>(
                iir_params{ reinterpret_cast<const biquad_section<double>*>(sos), sos_count }));
        },
        nullptr);
}

KFR_API_SPEC void kfr_filter_process_f32(KFR_FILTER_F32* plan, kfr_f32* output, const kfr_f32* input,
                                         size_t size)
{
    try_fn([&]() { reinterpret_cast<filter<float>*>(plan)->apply(output, input, size); });
}
KFR_API_SPEC void kfr_filter_process_f64(KFR_FILTER_F64* plan, kfr_f64* output, const kfr_f64* input,
                                         size_t size)
{
    try_fn([&]() { reinterpret_cast<filter<double>*>(plan)->apply(output, input, size); });
}

KFR_API_SPEC void kfr_filter_reset_f32(KFR_FILTER_F32* plan)
{
    try_fn([&]() { reinterpret_cast<filter<float>*>(plan)->reset(); });
}
KFR_API_SPEC void kfr_filter_reset_f64(KFR_FILTER_F64* plan)
{
    try_fn([&]() { reinterpret_cast<filter<double>*>(plan)->reset(); });
}

KFR_API_SPEC void kfr_filter_delete_plan_f32(KFR_FILTER_F32* plan)
{
    try_fn([&]() { delete reinterpret_cast<filter<f32>*>(plan); });
}
KFR_API_SPEC void kfr_filter_delete_plan_f64(KFR_FILTER_F64* plan)
{
    try_fn([&]() { delete reinterpret_cast<filter<f64>*>(plan); });
}
}

} // namespace kfr
