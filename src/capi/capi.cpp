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
#include <variant>
#define KFR_NO_C_COMPLEX_TYPES 1

#include <kfr/capi.h>
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>
#include <kfr/multiarch.h>

namespace kfr
{
static thread_local std::array<char, 256> latest_error;

void reset_error() { std::fill(latest_error.begin(), latest_error.end(), 0); }
void set_error(std::string_view s)
{
    size_t n = std::min(s.size(), latest_error.size() - 1);
    auto end = std::copy_n(s.begin(), n, latest_error.begin());
    std::fill(end, latest_error.end(), 0);
}

template <typename Fn, typename R = std::invoke_result_t<Fn>, typename T>
static R try_fn(Fn&& fn, T fallback)
{
#if KFR_HAS_EXCEPTIONS
    try
#endif
    {
        auto result = fn();
        reset_error();
        return result;
    }
#if KFR_HAS_EXCEPTIONS
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
#endif
}

template <typename Fn>
static void try_fn(Fn&& fn)
{
#if KFR_HAS_EXCEPTIONS
    try
#endif
    {
        fn();
        reset_error();
    }
#if KFR_HAS_EXCEPTIONS
    catch (std::exception& e)
    {
        set_error(e.what());
    }
    catch (...)
    {
        set_error("(unknown exception)");
    }
#endif
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
    KFR_ALWAYS_INLINE var_dft_plan_impl(Args&&... args) : plan(std::forward<Args>(args)...)
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
    KFR_ALWAYS_INLINE var_dft_plan_real_impl(Args&&... args) : plan(std::forward<Args>(args)...)
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

template <typename T>
static void store_biquad(T* sos, const biquad_section<T>& bq)
{
    const T coeffs[6] = { bq.a0, bq.a1, bq.a2, bq.b0, bq.b1, bq.b2 };
    std::copy_n(coeffs, 6, sos);
}

template <typename T>
static void generate_window(int type, size_t size, T param, bool symmetric, T* output)
{
    if (size == 0)
    {
        set_error("kfr_window: size must be greater than zero");
        return;
    }
    if (type < static_cast<int>(window_type::rectangular) || type > static_cast<int>(window_type::tukey))
    {
        set_error("kfr_window: unknown window type");
        return;
    }
    const window_symmetry sym    = symmetric ? window_symmetry::symmetric : window_symmetry::periodic;
    make_univector(output, size) = window<T>(size, static_cast<window_type>(type), param, sym);
}

// Validates IIR design arguments before try_fn so that the error is not cleared by reset_error().
static bool check_iir_design(int prototype, int response, int order, double rp, double rs, double frequency,
                             double high_frequency, double fs)
{
    if (prototype < KFR_IIR_BUTTERWORTH || prototype > KFR_IIR_ELLIPTIC)
    {
        set_error("kfr_iir: unknown prototype");
        return false;
    }
    if (response < KFR_IIR_LOWPASS || response > KFR_IIR_BANDSTOP)
    {
        set_error("kfr_iir: unknown response type");
        return false;
    }
#ifndef KFR_HAVE_ELLIPTIC
    if (prototype == KFR_IIR_ELLIPTIC)
    {
        set_error("kfr_iir: elliptic prototype requires a build with Boost.Math");
        return false;
    }
#endif
    const bool order_limited = prototype == KFR_IIR_BUTTERWORTH || prototype == KFR_IIR_BESSEL;
    if (order < 1 || (order_limited && order > 24))
    {
        set_error("kfr_iir: order must be 1..24 for Butterworth and Bessel, and at least 1 otherwise");
        return false;
    }
    if ((prototype == KFR_IIR_CHEBYSHEV1 || prototype == KFR_IIR_ELLIPTIC) && !(rp > 0))
    {
        set_error("kfr_iir: rp must be greater than zero");
        return false;
    }
    if ((prototype == KFR_IIR_CHEBYSHEV2 || prototype == KFR_IIR_ELLIPTIC) && !(rs > 0))
    {
        set_error("kfr_iir: rs must be greater than zero");
        return false;
    }
    if (!(fs > 0))
    {
        set_error("kfr_iir: fs must be greater than zero");
        return false;
    }
    const double nyquist = fs / 2;
    if (response == KFR_IIR_BANDPASS || response == KFR_IIR_BANDSTOP)
    {
        if (!(frequency > 0 && frequency < high_frequency && high_frequency < nyquist))
        {
            set_error("kfr_iir: band edges must satisfy 0 < frequency < high_frequency < fs / 2");
            return false;
        }
    }
    else if (!(frequency > 0 && frequency < nyquist))
    {
        set_error("kfr_iir: frequency must satisfy 0 < frequency < fs / 2");
        return false;
    }
    return true;
}

static zpk iir_prototype_zpk(int prototype, int order, double rp, double rs)
{
    switch (static_cast<KFR_IIR_PROTOTYPE>(prototype))
    {
    case KFR_IIR_BESSEL:
        return bessel(order);
    case KFR_IIR_CHEBYSHEV1:
        return chebyshev1(order, rp);
    case KFR_IIR_CHEBYSHEV2:
        return chebyshev2(order, rs);
#ifdef KFR_HAVE_ELLIPTIC
    case KFR_IIR_ELLIPTIC:
        return elliptic(order, rp, rs);
#endif
    default: // KFR_IIR_BUTTERWORTH; the prototype was validated by check_iir_design
        return butterworth(order);
    }
}

static zpk iir_response_zpk(const zpk& prototype, int response, double frequency, double high_frequency,
                            double fs)
{
    switch (static_cast<KFR_IIR_RESPONSE>(response))
    {
    case KFR_IIR_HIGHPASS:
        return iir_highpass(prototype, frequency, fs);
    case KFR_IIR_BANDPASS:
        return iir_bandpass(prototype, frequency, high_frequency, fs);
    case KFR_IIR_BANDSTOP:
        return iir_bandstop(prototype, frequency, high_frequency, fs);
    default: // KFR_IIR_LOWPASS; the response was validated by check_iir_design
        return iir_lowpass(prototype, frequency, fs);
    }
}

template <typename T>
static size_t design_iir(int prototype, int response, int order, double rp, double rs, double frequency,
                         double high_frequency, double fs, T* sos, size_t sos_capacity)
{
    const zpk analog           = iir_prototype_zpk(prototype, order, rp, rs);
    const zpk digital          = iir_response_zpk(analog, response, frequency, high_frequency, fs);
    const iir_params<T> params = to_sos<T>(digital);
    const size_t count         = params.size();
    if (sos != nullptr && sos_capacity >= count)
    {
        for (size_t i = 0; i < count; i++)
            store_biquad(sos + 6 * i, params[i]);
    }
    return count;
}

template <typename T>
static void one_shot_dft(complex<T>* out, const complex<T>* in, size_t size, bool inverse)
{
    dft_plan_ptr<T> plan = dft_cache::instance().get(ctype_t<T>(), size);
    if (inverse)
        plan->execute(out, in, nullptr, ctrue);
    else
        plan->execute(out, in, nullptr, cfalse);
}

template <typename T>
static void one_shot_realdft(complex<T>* out, const T* in, size_t size)
{
    dft_plan_real_ptr<T> plan = dft_cache::instance().getreal(ctype_t<T>(), size);
    plan->execute(out, in, nullptr);
}

template <typename T>
static void one_shot_irealdft(T* out, const complex<T>* in, size_t size)
{
    dft_plan_real_ptr<T> plan = dft_cache::instance().getreal(ctype_t<T>(), size);
    plan->execute(out, in, nullptr, ctrue);
}

template <typename T>
static void one_shot_convolve(T* out, const T* a, size_t a_size, const T* b, size_t b_size)
{
    if (a_size == 0 || b_size == 0)
        return;
    univector<T> result = convolve(make_univector(a, a_size), make_univector(b, b_size));
    std::copy_n(result.data(), result.size(), out);
}

template <typename T>
static void one_shot_filtfilt(const biquad_section<T>* sos, size_t sos_count, T* data, size_t size)
{
    auto arr = make_univector(data, size);
    filtfilt(arr, iir_params{ sos, sos_count });
}

extern "C"
{
KFR_API_SPEC const char* kfr_version_string()
{
    return "KFR " KFR_VERSION_STRING KFR_DEBUG_STR " " KFR_ENABLED_ARCHS_LIST " " KFR_ARCH_BITNESS_NAME
           " (" KFR_COMPILER_FULL_NAME "/" KFR_OS_NAME ")" KFR_BUILD_DETAILS_1 KFR_BUILD_DETAILS_2;
}
KFR_API_SPEC uint32_t kfr_version() { return KFR_VERSION; }
KFR_API_SPEC const char* kfr_enabled_archs() { return KFR_ENABLED_ARCHS_LIST; }
KFR_API_SPEC int kfr_current_arch() { return static_cast<int>(get_cpu()); }

KFR_API_SPEC const char* kfr_last_error() { return latest_error.data(); }

KFR_API_SPEC void* kfr_allocate(size_t size) { return details::aligned_malloc(size, KFR_DEFAULT_ALIGNMENT); }
KFR_API_SPEC void* kfr_allocate_aligned(size_t size, size_t alignment)
{
    return details::aligned_malloc(size, alignment);
}
KFR_API_SPEC void kfr_deallocate(void* ptr) { return details::aligned_free(ptr); }
#ifdef KFR_MANAGED_ALLOCATION
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
#endif

KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_plan_f32(size_t size)
{
    return try_fn([&]()
                  { return reinterpret_cast<KFR_DFT_PLAN_F32*>(new var_dft_plan_impl<float, 1>(size)); },
                  nullptr);
}

KFR_API_SPEC KFR_DFT_PLAN_F32* kfr_dft_create_2d_plan_f32(size_t size1, size_t size2)
{
    return try_fn(
        [&]()
        {
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
        [&]()
        {
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
                                                                    kfr_bool real_out_is_enough)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_REAL_PLAN_F32*>(
                new var_dft_plan_real_impl<float, 2>(shape{ size1, size2 }, bool(real_out_is_enough)));
        },
        nullptr);
}
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_3d_plan_f32(size_t size1, size_t size2, size_t size3,
                                                                    kfr_bool real_out_is_enough)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_REAL_PLAN_F32*>(
                new var_dft_plan_real_impl<float, 3>(shape{ size1, size2, size3 }, bool(real_out_is_enough)));
        },
        nullptr);
}
KFR_API_SPEC KFR_DFT_REAL_PLAN_F32* kfr_dft_real_create_md_plan_f32(size_t dims, const unsigned* shape,
                                                                    kfr_bool real_out_is_enough)
{
    return try_fn(
        [&]()
        {
            return reinterpret_cast<KFR_DFT_REAL_PLAN_F32*>(new var_dft_plan_real_impl<float, dynamic_shape>(
                init_shape(dims, shape), bool(real_out_is_enough)));
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
        [&]()
        {
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

// One-shot transforms

KFR_API_SPEC void kfr_dft_f32(kfr_c32* out, const kfr_c32* in, size_t size)
{
    try_fn(
        [&]()
        {
            one_shot_dft(reinterpret_cast<complex<float>*>(out), reinterpret_cast<const complex<float>*>(in),
                         size, false);
        });
}
KFR_API_SPEC void kfr_dft_f64(kfr_c64* out, const kfr_c64* in, size_t size)
{
    try_fn(
        [&]()
        {
            one_shot_dft(reinterpret_cast<complex<double>*>(out),
                         reinterpret_cast<const complex<double>*>(in), size, false);
        });
}
KFR_API_SPEC void kfr_idft_f32(kfr_c32* out, const kfr_c32* in, size_t size)
{
    try_fn(
        [&]()
        {
            one_shot_dft(reinterpret_cast<complex<float>*>(out), reinterpret_cast<const complex<float>*>(in),
                         size, true);
        });
}
KFR_API_SPEC void kfr_idft_f64(kfr_c64* out, const kfr_c64* in, size_t size)
{
    try_fn(
        [&]()
        {
            one_shot_dft(reinterpret_cast<complex<double>*>(out),
                         reinterpret_cast<const complex<double>*>(in), size, true);
        });
}

KFR_API_SPEC void kfr_realdft_f32(kfr_c32* out, const kfr_f32* in, size_t size)
{
    try_fn([&]() { one_shot_realdft(reinterpret_cast<complex<float>*>(out), in, size); });
}
KFR_API_SPEC void kfr_realdft_f64(kfr_c64* out, const kfr_f64* in, size_t size)
{
    try_fn([&]() { one_shot_realdft(reinterpret_cast<complex<double>*>(out), in, size); });
}
KFR_API_SPEC void kfr_irealdft_f32(kfr_f32* out, const kfr_c32* in, size_t size)
{
    try_fn([&]() { one_shot_irealdft(out, reinterpret_cast<const complex<float>*>(in), size); });
}
KFR_API_SPEC void kfr_irealdft_f64(kfr_f64* out, const kfr_c64* in, size_t size)
{
    try_fn([&]() { one_shot_irealdft(out, reinterpret_cast<const complex<double>*>(in), size); });
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
    return try_fn([&]() { return reinterpret_cast<dct_plan<float>*>(plan)->size; }, 0);
}
KFR_API_SPEC size_t kfr_dct_get_size_f64(KFR_DCT_PLAN_F64* plan)
{
    return try_fn([&]() { return reinterpret_cast<dct_plan<double>*>(plan)->size; }, 0);
}

KFR_API_SPEC size_t kfr_dct_get_temp_size_f32(KFR_DCT_PLAN_F32* plan)
{
    return try_fn([&]() { return reinterpret_cast<dct_plan<float>*>(plan)->temp_size; }, 0);
}
KFR_API_SPEC size_t kfr_dct_get_temp_size_f64(KFR_DCT_PLAN_F64* plan)
{
    return try_fn([&]() { return reinterpret_cast<dct_plan<double>*>(plan)->temp_size; }, 0);
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

// Biquad design

KFR_API_SPEC void kfr_biquad_allpass_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_allpass<float>(frequency, Q)); });
}
KFR_API_SPEC void kfr_biquad_allpass_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_allpass<double>(frequency, Q)); });
}

KFR_API_SPEC void kfr_biquad_lowpass_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_lowpass<float>(frequency, Q)); });
}
KFR_API_SPEC void kfr_biquad_lowpass_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_lowpass<double>(frequency, Q)); });
}

KFR_API_SPEC void kfr_biquad_highpass_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_highpass<float>(frequency, Q)); });
}
KFR_API_SPEC void kfr_biquad_highpass_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_highpass<double>(frequency, Q)); });
}

KFR_API_SPEC void kfr_biquad_bandpass_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_bandpass<float>(frequency, Q)); });
}
KFR_API_SPEC void kfr_biquad_bandpass_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_bandpass<double>(frequency, Q)); });
}

KFR_API_SPEC void kfr_biquad_notch_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_notch<float>(frequency, Q)); });
}
KFR_API_SPEC void kfr_biquad_notch_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_notch<double>(frequency, Q)); });
}

KFR_API_SPEC void kfr_biquad_peak_f32(kfr_f32 frequency, kfr_f32 Q, kfr_f32 gain_db, kfr_f32* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_peak<float>(frequency, Q, gain_db)); });
}
KFR_API_SPEC void kfr_biquad_peak_f64(kfr_f64 frequency, kfr_f64 Q, kfr_f64 gain_db, kfr_f64* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_peak<double>(frequency, Q, gain_db)); });
}

KFR_API_SPEC void kfr_biquad_lowshelf_f32(kfr_f32 frequency, kfr_f32 gain_db, kfr_f32* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_lowshelf<float>(frequency, gain_db)); });
}
KFR_API_SPEC void kfr_biquad_lowshelf_f64(kfr_f64 frequency, kfr_f64 gain_db, kfr_f64* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_lowshelf<double>(frequency, gain_db)); });
}

KFR_API_SPEC void kfr_biquad_highshelf_f32(kfr_f32 frequency, kfr_f32 gain_db, kfr_f32* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_highshelf<float>(frequency, gain_db)); });
}
KFR_API_SPEC void kfr_biquad_highshelf_f64(kfr_f64 frequency, kfr_f64 gain_db, kfr_f64* sos)
{
    try_fn([&]() { store_biquad(sos, biquad_highshelf<double>(frequency, gain_db)); });
}

// IIR design

KFR_API_SPEC size_t kfr_iir_design_f32(KFR_IIR_PROTOTYPE prototype, KFR_IIR_RESPONSE response, int order,
                                       kfr_f32 rp, kfr_f32 rs, kfr_f32 frequency, kfr_f32 high_frequency,
                                       kfr_f32 fs, kfr_f32* sos, size_t sos_capacity)
{
    if (!check_iir_design(static_cast<int>(prototype), static_cast<int>(response), order, rp, rs, frequency,
                          high_frequency, fs))
        return 0;
    return try_fn(
        [&]()
        {
            return design_iir<float>(static_cast<int>(prototype), static_cast<int>(response), order, rp, rs,
                                     frequency, high_frequency, fs, sos, sos_capacity);
        },
        size_t(0));
}
KFR_API_SPEC size_t kfr_iir_design_f64(KFR_IIR_PROTOTYPE prototype, KFR_IIR_RESPONSE response, int order,
                                       kfr_f64 rp, kfr_f64 rs, kfr_f64 frequency, kfr_f64 high_frequency,
                                       kfr_f64 fs, kfr_f64* sos, size_t sos_capacity)
{
    if (!check_iir_design(static_cast<int>(prototype), static_cast<int>(response), order, rp, rs, frequency,
                          high_frequency, fs))
        return 0;
    return try_fn(
        [&]()
        {
            return design_iir<double>(static_cast<int>(prototype), static_cast<int>(response), order, rp, rs,
                                      frequency, high_frequency, fs, sos, sos_capacity);
        },
        size_t(0));
}

// Windows

KFR_API_SPEC void kfr_window_f32(KFR_WINDOW_TYPE type, size_t size, kfr_f32 param, kfr_bool symmetric,
                                 kfr_f32* output)
{
    try_fn([&]() { generate_window<float>(static_cast<int>(type), size, param, symmetric, output); });
}
KFR_API_SPEC void kfr_window_f64(KFR_WINDOW_TYPE type, size_t size, kfr_f64 param, kfr_bool symmetric,
                                 kfr_f64* output)
{
    try_fn([&]() { generate_window<double>(static_cast<int>(type), size, param, symmetric, output); });
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

// One-shot convolution and filtfilt

KFR_API_SPEC void kfr_convolve_f32(kfr_f32* out, const kfr_f32* a, size_t a_size, const kfr_f32* b,
                                   size_t b_size)
{
    try_fn([&]() { one_shot_convolve(out, a, a_size, b, b_size); });
}
KFR_API_SPEC void kfr_convolve_f64(kfr_f64* out, const kfr_f64* a, size_t a_size, const kfr_f64* b,
                                   size_t b_size)
{
    try_fn([&]() { one_shot_convolve(out, a, a_size, b, b_size); });
}

KFR_API_SPEC void kfr_filtfilt_f32(const kfr_f32* sos, size_t sos_count, kfr_f32* data, size_t size)
{
    try_fn(
        [&]()
        { one_shot_filtfilt(reinterpret_cast<const biquad_section<float>*>(sos), sos_count, data, size); });
}
KFR_API_SPEC void kfr_filtfilt_f64(const kfr_f64* sos, size_t sos_count, kfr_f64* data, size_t size)
{
    try_fn(
        [&]()
        { one_shot_filtfilt(reinterpret_cast<const biquad_section<double>*>(sos), sos_count, data, size); });
}
}

} // namespace kfr
