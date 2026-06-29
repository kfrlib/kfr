/** @addtogroup dft
 *  @{
 */
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

#include <kfr/base/math_expressions.hpp>
#include <kfr/base/simd_expressions.hpp>
#include "dft-fft.hpp"

KFR_PRAGMA_GNU(GCC diagnostic push)
#if KFR_HAS_WARNING("-Wshadow")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
#endif
#if KFR_HAS_WARNING("-Wunused-lambda-capture")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wunused-lambda-capture")
#endif
#if KFR_HAS_WARNING("-Wpass-failed")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wpass-failed")
#endif

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4100))

namespace kfr
{

inline namespace KFR_ARCH_NAME
{
constexpr csizes_t<2, 3, 4, 5, 6, 7, 8, 9, 10> dft_radices{};

namespace intr
{

template <typename T>
void dft_stage_fixed_initialize(dft_stage<T>* stage, size_t width)
{
    complex<T>* twiddle = ptr_cast<complex<T>>(stage->data);
    const size_t N      = stage->repeats * stage->radix;
    const size_t Nord   = stage->repeats;
    size_t i            = 0;

    while (width > 0)
    {
        KFR_LOOP_NOUNROLL
        for (; i < Nord / width * width; i += width)
        {
            KFR_LOOP_NOUNROLL
            for (size_t j = 1; j < stage->radix; j++)
            {
                KFR_LOOP_NOUNROLL
                for (size_t k = 0; k < width; k++)
                {
                    cvec<T, 1> xx                    = calculate_twiddle<T>((i + k) * j, N);
                    ref_cast<cvec<T, 1>>(twiddle[k]) = xx;
                }
                twiddle += width;
            }
        }
        width = width / 2;
    }
}

template <typename T, size_t fixed_radix>
struct dft_stage_fixed_impl : dft_stage<T>
{
    dft_stage_fixed_impl(size_t, size_t iterations, size_t blocks)
    {
        this->name       = dft_name(this);
        this->radix      = fixed_radix;
        this->blocks     = blocks;
        this->repeats    = iterations;
        this->recursion  = false; // true;
        this->stage_size = fixed_radix * iterations * blocks;
        this->data_size  = align_up((this->repeats * (fixed_radix - 1)) * sizeof(complex<T>),
                                    platform<>::native_cache_alignment);
    }

    constexpr static size_t rradix = fixed_radix;

    constexpr static size_t width = fixed_radix >= 7   ? fft_vector_width<T> / 2
                                    : fixed_radix >= 4 ? fft_vector_width<T>
                                                       : fft_vector_width<T> * 2;
    virtual void do_initialize(size_t) override final { dft_stage_fixed_initialize(this, width); }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        const size_t Nord         = this->repeats;
        const complex<T>* twiddle = ptr_cast<complex<T>>(this->data);

        const size_t N = Nord * fixed_radix;
        KFR_LOOP_NOUNROLL
        for (size_t b = 0; b < this->blocks; b++)
        {
            butterflies(Nord, csize<width>, csize<fixed_radix>, cbool<inverse>, out, in, twiddle, Nord);
            in += N;
            out += N;
        }
    }
};

template <typename T, size_t fixed_radix>
struct dft_stage_fixed_final_impl : dft_stage<T>
{
    dft_stage_fixed_final_impl(size_t, size_t iterations, size_t blocks)
    {
        this->name        = dft_name(this);
        this->radix       = fixed_radix;
        this->blocks      = blocks;
        this->repeats     = iterations;
        this->stage_size  = fixed_radix * iterations * blocks;
        this->recursion   = false;
        this->can_inplace = false;
    }
    constexpr static size_t width = fixed_radix >= 7   ? fft_vector_width<T> / 2
                                    : fixed_radix >= 4 ? fft_vector_width<T>
                                                       : fft_vector_width<T> * 2;

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        const size_t b = this->blocks;

        butterflies(b, csize<width>, csize<fixed_radix>, cbool<inverse>, out, in, b);
    }
};

template <typename E>
inline decltype(auto) apply_conj(E&& e, cfalse_t)
{
    return std::forward<E>(e);
}

template <typename E>
inline auto apply_conj(E&& e, ctrue_t)
{
    return cconj(std::forward<E>(e));
}

/// [0, N - 1, N - 2, N - 3, ..., 3, 2, 1]
template <typename E>
struct fft_inverse : expression_with_traits<E>
{
    using value_type = typename expression_with_traits<E>::value_type;

    KFR_MEM_INTRINSIC fft_inverse(E&& expr) noexcept : expression_with_traits<E>(std::forward<E>(expr)) {}

    friend KFR_INTRINSIC vec<value_type, 1> get_elements(const fft_inverse& self, shape<1> index,
                                                         axis_params<0, 1>)
    {
        const size_t size = get_shape(self).front();
        return get_elements(self.first(), index.front() == 0 ? 0 : size - index, axis_params<0, 1>());
    }

    template <size_t N>
    friend KFR_MEM_INTRINSIC vec<value_type, N> get_elements(const fft_inverse& self, shape<1> index,
                                                             axis_params<0, N>)
    {
        const size_t size = get_shape(self).front();
        if (index.front() == 0)
        {
            return concat(get_elements(self.first(), index, axis_params<0, 1>()),
                          reverse(get_elements(self.first(), size - (N - 1), axis_params<0, N - 1>())));
        }
        return reverse(get_elements(self.first(), size - index - (N - 1), axis_params<0, N>()));
    }
};

template <typename E>
inline auto apply_fft_inverse(E&& e)
{
    return fft_inverse<E>(std::forward<E>(e));
}

template <typename T>
struct dft_arblen_stage_impl : dft_stage<T>
{
    static complex<T> accurate_cexp(size_t k, size_t N)
    {
        size_t kk = k % (N / 2);
        complex<T> result{ std::cos(kk * c_pi<T, 2> / N), -std::sin(kk * c_pi<T, 2> / N) };

        if (k >= N / 2)
            result = -result;
        return result;
    }

    dft_arblen_stage_impl(size_t size)
        : size(size), fftsize(next_poweroftwo(size * 2 - 1)), plan(fftsize, dft_order::internal)
    {
        this->name        = dft_name(this);
        this->radix       = size;
        this->blocks      = 1;
        this->repeats     = 1;
        this->recursion   = false;
        this->can_inplace = false;
        this->temp_size   = plan.temp_size + fftsize * sizeof(complex<T>);
        this->stage_size  = size;

        chirp_.resize(size);
        chirp_[0] = complex<T>(1, 0);
        size_t k  = 0;
        for (size_t m = 1; m < size; ++m)
        {
            k += 2 * m - 1;
            if (k >= 2 * size)
                k -= 2 * size;
            chirp_[m] = accurate_cexp(k, size * 2);
        }

        T fct = T(1) / fftsize;
        ichirpp_.resize(fftsize, 0);
        ichirpp_.slice(0, size)                      = chirp_ * fct;
        ichirpp_.slice(fftsize - size + 1, size - 1) = reverse(chirp_.slice(1) * fct);

        univector<u8> temp(plan.temp_size);
        plan.execute(ichirpp_, ichirpp_, temp);

        ichirpp_.resize(fftsize / 2 + 1);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        const size_t n = this->size;

        auto xp = make_univector(ptr_cast<complex<T>>(temp), fftsize);
        u8* tmp = ptr_cast<u8>(ptr_cast<complex<T>>(temp) + fftsize);

        auto&& chirp = apply_conj(chirp_, cbool<inverse>);

        xp.slice(0, n)           = make_univector(in, n) * chirp;
        xp.slice(n, fftsize - n) = scalar(complex<T>(0, 0));

        plan.execute(xp.data(), xp.data(), tmp);

        xp[0] *= apply_conj(ichirpp_[0], cbool<!inverse>);
        xp.slice(1, fftsize / 2 - 1) *= apply_conj(ichirpp_.slice(1, fftsize / 2 - 1), cbool<!inverse>);
        xp.slice(fftsize / 2 + 1, fftsize / 2 - 1) *=
            apply_conj(reverse(ichirpp_.slice(1, fftsize / 2 - 1)), cbool<!inverse>);
        xp[fftsize / 2] *= apply_conj(ichirpp_[fftsize / 2], cbool<!inverse>);

        plan.execute(xp.data(), xp.data(), tmp, ctrue);

        make_univector(out, n) = xp.slice(0, n) * chirp;
    }

    const size_t size;
    const size_t fftsize;
    dft_plan<T> plan;
    univector<complex<T>> chirp_;
    univector<complex<T>> ichirpp_;
};

template <typename T, size_t radix1, size_t radix2, size_t size = radix1 * radix2>
struct dft_special_stage_impl : dft_stage<T>
{
    dft_special_stage_impl() : stage1(radix1, size / radix1, 1), stage2(radix2, 1, size / radix2)
    {
        this->name        = dft_name(this);
        this->radix       = size;
        this->blocks      = 1;
        this->repeats     = 1;
        this->recursion   = false;
        this->can_inplace = false;
        this->stage_size  = size;
        this->temp_size   = stage1.temp_size + stage2.temp_size + sizeof(complex<T>) * size;
        this->data_size   = stage1.data_size + stage2.data_size;
    }
    void dump() const override
    {
        dft_stage<T>::dump();
        printf("    ");
        stage1.dump();
        printf("    ");
        stage2.dump();
    }
    void do_initialize(size_t stage_size) override
    {
        stage1.data = this->data;
        stage2.data = this->data + stage1.data_size;
        stage1.initialize(stage_size);
        stage2.initialize(stage_size);
    }
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        complex<T>* scratch = ptr_cast<complex<T>>(temp + stage1.temp_size + stage2.temp_size);
        stage1.do_execute(cbool<inverse>, scratch, in, temp);
        stage2.do_execute(cbool<inverse>, out, scratch, temp + stage1.temp_size);
    }
    dft_stage_fixed_impl<T, radix1> stage1;
    dft_stage_fixed_final_impl<T, radix2> stage2;
};

template <typename T, bool final>
struct dft_stage_generic_impl : dft_stage<T>
{
    dft_stage_generic_impl(size_t radix, size_t iterations, size_t blocks)
    {
        this->name        = dft_name(this);
        this->radix       = radix;
        this->blocks      = blocks;
        this->repeats     = iterations;
        this->recursion   = false; // true;
        this->can_inplace = false;
        this->stage_size  = radix * iterations * blocks;
        this->temp_size   = align_up(sizeof(complex<T>) * radix, platform<>::native_cache_alignment);
        // Intra-butterfly DFT twiddles, followed (for non-final stages) by the per-output
        // stage twiddles that twist the butterfly outputs between passes (DIF).
        // The final stage applies no stage twiddles, so its iterations count is 1.
        this->bfly_data_size = align_up(sizeof(complex<T>) * sqr(this->radix / 2), bfly_data_alignment);
        this->data_size      = this->bfly_data_size;
        if (!final)
            this->data_size += align_up(sizeof(complex<T>) * (this->repeats * (this->radix - 1)),
                                        platform<>::native_cache_alignment);
    }

protected:
    constexpr static size_t bfly_data_alignment = platform<>::native_cache_alignment;
    size_t bfly_data_size                       = 0;

    virtual void do_initialize(size_t) override final
    {
        // Intra-butterfly twiddles consumed by generic_butterfly.
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        KFR_LOOP_NOUNROLL
        for (size_t i = 0; i < this->radix / 2; i++)
        {
            KFR_LOOP_NOUNROLL
            for (size_t j = 0; j < this->radix / 2; j++)
            {
                cwrite<1>(twiddle++, calculate_twiddle<T>((i + 1) * (j + 1), this->radix));
            }
        }

        // Stage twiddles for non-final stages: output j (1..radix-1) of butterfly i is
        // multiplied by W_N^{i*j}, with N = repeats * radix.
        if (!final)
        {
            const size_t N       = this->repeats * this->radix;
            complex<T>* stage_tw = ptr_cast<complex<T>>(this->data + this->bfly_data_size);
            KFR_LOOP_NOUNROLL
            for (size_t i = 0; i < this->repeats; i++)
            {
                KFR_LOOP_NOUNROLL
                for (size_t j = 1; j < this->radix; j++)
                {
                    cwrite<1>(stage_tw++, calculate_twiddle<T>(i * j, N));
                }
            }
        }
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        const complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        const size_t bl           = this->blocks;
        const size_t radix        = this->radix;

        if constexpr (final)
        {
            // Final stage: inputs are contiguous per butterfly, outputs strided by blocks,
            // no stage twiddles. repeats == 1.
            KFR_LOOP_NOUNROLL
            for (size_t b = 0; b < bl; b++)
                generic_butterfly(radix, cbool<inverse>, out + b, in + b * radix, ptr_cast<complex<T>>(temp),
                                  twiddle, bl);
        }
        else
        {
            // Non-final stage (DIF): mirror dft_stage_fixed_impl. Each block holds repeats
            // butterflies whose radix points are interleaved with stride `repeats`; after the
            // butterfly, twist outputs by the stage twiddles.
            const size_t Nord          = this->repeats;
            const size_t N             = Nord * radix;
            const complex<T>* stage_tw = ptr_cast<complex<T>>(this->data + this->bfly_data_size);
            KFR_LOOP_NOUNROLL
            for (size_t b = 0; b < bl; b++)
            {
                complex<T>* out_b      = out + b * N;
                const complex<T>* in_b = in + b * N;
                KFR_LOOP_NOUNROLL
                for (size_t i = 0; i < Nord; i++)
                {
                    generic_butterfly(radix, cbool<inverse>, out_b + i, in_b + i, ptr_cast<complex<T>>(temp),
                                      twiddle, Nord, Nord);

                    const complex<T>* tw = stage_tw + i * (radix - 1);
                    for (size_t j = 1; j < radix; j++)
                    {
                        cvec<T, 1> x = cread<1>(out_b + i + j * Nord);
                        cvec<T, 1> w = cread<1>(tw + (j - 1));
                        if constexpr (inverse)
                            x = cmul_conj(x, w);
                        else
                            x = cmul(x, w);
                        cwrite<1>(out_b + i + j * Nord, x);
                    }
                }
            }
        }
    }
};

template <typename T, typename Tr2>
inline void dft_permute(complex<T>* out, const complex<T>* in, size_t r0, size_t r1, Tr2 first_radix)
{
    KFR_ASSUME(r0 > 1);
    KFR_ASSUME(r1 > 1);

    KFR_LOOP_NOUNROLL
    for (size_t p = 0; p < r0; p++)
    {
        const complex<T>* in1 = in;
        KFR_LOOP_NOUNROLL
        for (size_t i = 0; i < r1; i++)
        {
            const complex<T>* in2 = in1;
            KFR_LOOP_UNROLL
            for (size_t j = 0; j < first_radix; j++)
            {
                *out++ = *in2;
                in2 += r1;
            }
            in1++;
            in += first_radix;
        }
    }
}

template <typename T, typename Tr2>
inline void dft_permute_deep(complex<T>*& out, const complex<T>* in, const uint32_t* radices, size_t count,
                             size_t index, size_t inscale, size_t inner_size, Tr2 first_radix)
{
    const bool b       = index == 1;
    const size_t radix = radices[index];
    if (b)
    {
        KFR_LOOP_NOUNROLL
        for (size_t i = 0; i < radix; i++)
        {
            const complex<T>* in1 = in;
            KFR_LOOP_UNROLL
            for (size_t j = 0; j < first_radix; j++)
            {
                *out++ = *in1;
                in1 += inner_size;
            }
            in += inscale;
        }
    }
    else
    {
        const size_t steps        = radix;
        const size_t inscale_next = inscale * radix;
        KFR_LOOP_NOUNROLL
        for (size_t i = 0; i < steps; i++)
        {
            dft_permute_deep(out, in, radices, count, index - 1, inscale_next, inner_size, first_radix);
            in += inscale;
        }
    }
}

template <typename T>
struct dft_reorder_stage_impl : dft_stage<T>
{
    dft_reorder_stage_impl(const uint32_t* radices, size_t count) : count(count)
    {
        this->name        = dft_name(this);
        this->can_inplace = false;
        this->data_size   = 0;
        std::copy(radices, radices + count, this->radices);
        this->inner_size = 1;
        this->size       = 1;
        for (size_t r = 0; r < count; r++)
        {
            if (r != 0 && r != count - 1)
                this->inner_size *= radices[r];
            this->size *= radices[r];
        }
        this->stage_size = this->size;
    }

protected:
    uint32_t radices[32];
    size_t count      = 0;
    size_t size       = 0;
    size_t inner_size = 0;
    virtual void do_initialize(size_t) override final {}

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        cswitch(
            dft_radices, radices[0],
            [&](auto first_radix)
            {
                if (count == 3)
                {
                    dft_permute(out, in, radices[2], radices[1], first_radix);
                }
                else
                {
                    const size_t rlast = radices[count - 1];
                    for (size_t p = 0; p < rlast; p++)
                    {
                        dft_permute_deep(out, in, radices, count, count - 2, 1, inner_size, first_radix);
                        in += size / rlast;
                    }
                }
            },
            [&]()
            {
                if (count == 3)
                {
                    dft_permute(out, in, radices[2], radices[1], radices[0]);
                }
                else
                {
                    const size_t rlast = radices[count - 1];
                    for (size_t p = 0; p < rlast; p++)
                    {
                        dft_permute_deep(out, in, radices, count, count - 2, 1, inner_size, radices[0]);
                        in += size / rlast;
                    }
                }
            });
    }
};
} // namespace intr

template <bool is_final, typename T>
void prepare_dft_stage(dft_plan<T>* self, size_t radix, size_t iterations, size_t blocks, cbool_t<is_final>)
{
    return cswitch(
        dft_radices, radix,
        [self, iterations, blocks](auto radix) KFR_INLINE_LAMBDA
        {
            add_stage<std::conditional_t<is_final, intr::dft_stage_fixed_final_impl<T, val_of(radix)>,
                                         intr::dft_stage_fixed_impl<T, val_of(radix)>>>(self, radix,
                                                                                        iterations, blocks);
        },
        [self, radix, iterations, blocks]()
        { add_stage<intr::dft_stage_generic_impl<T, is_final>>(self, radix, iterations, blocks); });
}

static size_t factorize(uint32_t number, uint32_t* factors)
{
    size_t count = 0;

    KFR_FOR(i, 0, dft_radices.size())
    {
        constexpr size_t r = dft_radices.get<i>();
        while (number % r == 0)
        {
            factors[count++] = r;
            number /= r;
        }
    };

    static constexpr uint32_t increments[] = { 4, 2, 4, 2, 4, 6, 2, 6 };

    uint32_t f = 7;
    int i      = 0;
    while (f * f <= number)
    {
        if (number % f == 0)
        {
            factors[count++] = f;
            number /= f;
        }
        else
        {
            f += increments[i];
            i = (i + 1) % 8;
        }
    }

    if (number > 1)
        factors[count++] = number;

    return count;
}

template <typename T>
void init_dft(dft_plan<T>* self, size_t size, dft_order)
{
    if (size == 60)
    {
        add_stage<intr::dft_special_stage_impl<T, 6, 10>>(self);
    }
    else if (size == 48)
    {
        add_stage<intr::dft_special_stage_impl<T, 6, 8>>(self);
    }
    else
    {
        uint32_t factors[32]           = { 0 };
        size_t factors_size            = factorize(static_cast<uint32_t>(size), factors);

        uint32_t largest_factor = factors[factors_size - 1];

        int num_stages = 0;
        if (largest_factor >= 101)
        {
            add_stage<intr::dft_arblen_stage_impl<T>>(self, size);
            ++num_stages;
            self->arblen = true;
        }
        else
        {
            size_t blocks     = 1;
            size_t iterations = size;

            for (size_t fi = 0; fi < factors_size; fi++)
            {
                uint32_t r = factors[fi];
                iterations /= r;
                if (iterations == 1)
                    prepare_dft_stage(self, r, iterations, blocks, ctrue);
                else
                    prepare_dft_stage(self, r, iterations, blocks, cfalse);
                ++num_stages;
                blocks *= r;
            }

            if (num_stages > 2)
                add_stage<intr::dft_reorder_stage_impl<T>>(self, factors, factors_size);
        }
    }
}

} // namespace KFR_ARCH_NAME

} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)

KFR_PRAGMA_MSVC(warning(pop))
