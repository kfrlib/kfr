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

#include "../../base/math_expressions.hpp"
#include "../../base/simd_expressions.hpp"
#include "dft-fft.hpp"

CMT_PRAGMA_GNU(GCC diagnostic push)
#if CMT_HAS_WARNING("-Wshadow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
#endif
#if CMT_HAS_WARNING("-Wunused-lambda-capture")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wunused-lambda-capture")
#endif
#if CMT_HAS_WARNING("-Wpass-failed")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpass-failed")
#endif

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4100))

namespace kfr
{

inline namespace CMT_ARCH_NAME
{
constexpr csizes_t<2, 3, 4, 5, 6, 7, 8, 9, 10> dft_radices{};

namespace intrinsics
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
        CMT_LOOP_NOUNROLL
        for (; i < Nord / width * width; i += width)
        {
            CMT_LOOP_NOUNROLL
            for (size_t j = 1; j < stage->radix; j++)
            {
                CMT_LOOP_NOUNROLL
                for (size_t k = 0; k < width; k++)
                {
                    cvec<T, 1> xx = cossin_conj(broadcast<2, T>(c_pi<T, 2> * (i + k) * j / N));
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
        CMT_LOOP_NOUNROLL
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
inline E& apply_conj(E& e, cfalse_t)
{
    return e;
}

template <typename E>
inline auto apply_conj(E& e, ctrue_t)
{
    return cconj(e);
}

/// [0, N - 1, N - 2, N - 3, ..., 3, 2, 1]
template <typename E>
struct fft_inverse : expression_with_traits<E>
{
    using value_type = typename expression_with_traits<E>::value_type;

    KFR_MEM_INTRINSIC fft_inverse(E&& expr) CMT_NOEXCEPT : expression_with_traits<E>(std::forward<E>(expr)) {}

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
    dft_arblen_stage_impl(size_t size)
        : size(size), fftsize(next_poweroftwo(size) * 2), plan(fftsize, dft_order::internal)
    {
        this->name        = dft_name(this);
        this->radix       = size;
        this->blocks      = 1;
        this->repeats     = 1;
        this->recursion   = false;
        this->can_inplace = false;
        this->temp_size   = plan.temp_size;
        this->stage_size  = size;

        chirp_ = render(cexp(sqr(linspace(T(1) - size, size - T(1), size * 2 - 1, true, ctrue)) *
                             complex<T>(0, -1) * c_pi<T> / size));

        ichirpp_ = render(truncate(padded(1 / slice(chirp_, 0, 2 * size - 1)), fftsize));

        univector<u8> temp(plan.temp_size);
        plan.execute(ichirpp_, ichirpp_, temp);
        xp.resize(fftsize, 0);
        xp_fft.resize(fftsize);
        invN2 = T(1) / fftsize;
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        const size_t n = this->size;

        auto&& chirp = apply_conj(chirp_, cbool<inverse>);

        xp.slice(0, n) = make_univector(in, n) * slice(chirp, n - 1);

        plan.execute(xp_fft.data(), xp.data(), temp);

        if (inverse)
            xp_fft = xp_fft * cconj(apply_fft_inverse(ichirpp_));
        else
            xp_fft = xp_fft * ichirpp_;
        plan.execute(xp_fft.data(), xp_fft.data(), temp, ctrue);

        make_univector(out, n) = xp_fft.slice(n - 1, n) * slice(chirp, n - 1, n) * invN2;
    }

    const size_t size;
    const size_t fftsize;
    T invN2;
    dft_plan<T> plan;
    univector<complex<T>> chirp_;
    univector<complex<T>> ichirpp_;
    univector<complex<T>> xp;
    univector<complex<T>> xp_fft;
};

template <typename T, size_t radix1, size_t radix2, size_t size = radix1* radix2>
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
        this->data_size =
            align_up(sizeof(complex<T>) * sqr(this->radix / 2), platform<>::native_cache_alignment);
    }

protected:
    virtual void do_initialize(size_t) override final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        CMT_LOOP_NOUNROLL
        for (size_t i = 0; i < this->radix / 2; i++)
        {
            CMT_LOOP_NOUNROLL
            for (size_t j = 0; j < this->radix / 2; j++)
            {
                cwrite<1>(twiddle++, cossin_conj(broadcast<2>((i + 1) * (j + 1) * c_pi<T, 2> / this->radix)));
            }
        }
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        const complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        const size_t bl           = this->blocks;

        CMT_LOOP_NOUNROLL
        for (size_t b = 0; b < bl; b++)
            generic_butterfly(this->radix, cbool<inverse>, out + b, in + b * this->radix,
                              ptr_cast<complex<T>>(temp), twiddle, bl);
    }
};

template <typename T, typename Tr2>
inline void dft_permute(complex<T>* out, const complex<T>* in, size_t r0, size_t r1, Tr2 first_radix)
{
    CMT_ASSUME(r0 > 1);
    CMT_ASSUME(r1 > 1);

    CMT_LOOP_NOUNROLL
    for (size_t p = 0; p < r0; p++)
    {
        const complex<T>* in1 = in;
        CMT_LOOP_NOUNROLL
        for (size_t i = 0; i < r1; i++)
        {
            const complex<T>* in2 = in1;
            CMT_LOOP_UNROLL
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
inline void dft_permute_deep(complex<T>*& out, const complex<T>* in, const size_t* radices, size_t count,
                             size_t index, size_t inscale, size_t inner_size, Tr2 first_radix)
{
    const bool b       = index == 1;
    const size_t radix = radices[index];
    if (b)
    {
        CMT_LOOP_NOUNROLL
        for (size_t i = 0; i < radix; i++)
        {
            const complex<T>* in1 = in;
            CMT_LOOP_UNROLL
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
        CMT_LOOP_NOUNROLL
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
    dft_reorder_stage_impl(const int* radices, size_t count) : count(count)
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
    size_t radices[32];
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
} // namespace intrinsics

template <bool is_final, typename T>
void prepare_dft_stage(dft_plan<T>* self, size_t radix, size_t iterations, size_t blocks, cbool_t<is_final>)
{
    return cswitch(
        dft_radices, radix,
        [self, iterations, blocks](auto radix) CMT_INLINE_LAMBDA
        {
            add_stage<std::conditional_t<is_final, intrinsics::dft_stage_fixed_final_impl<T, val_of(radix)>,
                                         intrinsics::dft_stage_fixed_impl<T, val_of(radix)>>>(
                self, radix, iterations, blocks);
        },
        [self, radix, iterations, blocks]()
        { add_stage<intrinsics::dft_stage_generic_impl<T, is_final>>(self, radix, iterations, blocks); });
}

template <typename T>
void init_dft(dft_plan<T>* self, size_t size, dft_order)
{
    if (size == 60)
    {
        add_stage<intrinsics::dft_special_stage_impl<T, 6, 10>>(self);
    }
    else if (size == 48)
    {
        add_stage<intrinsics::dft_special_stage_impl<T, 6, 8>>(self);
    }
    else
    {
        size_t cur_size                = size;
        constexpr size_t radices_count = dft_radices.back() + 1;
        u8 count[radices_count]        = { 0 };
        int radices[32]                = { 0 };
        size_t radices_size            = 0;

        cforeach(dft_radices[csizeseq<dft_radices.size(), dft_radices.size() - 1, -1>],
                 [&](auto radix)
                 {
                     while (cur_size && cur_size % val_of(radix) == 0)
                     {
                         count[val_of(radix)]++;
                         cur_size /= val_of(radix);
                     }
                 });

        if (cur_size >= 101)
        {
            add_stage<intrinsics::dft_arblen_stage_impl<T>>(self, size);
            self->arblen = true;
        }
        else
        {
            size_t blocks     = 1;
            size_t iterations = size;

            for (size_t r = dft_radices.front(); r <= dft_radices.back(); r++)
            {
                for (size_t i = 0; i < count[r]; i++)
                {
                    iterations /= r;
                    radices[radices_size++] = static_cast<int>(r);
                    if (iterations == 1)
                        prepare_dft_stage(self, r, iterations, blocks, ctrue);
                    else
                        prepare_dft_stage(self, r, iterations, blocks, cfalse);
                    blocks *= r;
                }
            }

            if (cur_size > 1)
            {
                iterations /= cur_size;
                radices[radices_size++] = static_cast<int>(cur_size);
                if (iterations == 1)
                    prepare_dft_stage(self, cur_size, iterations, blocks, ctrue);
                else
                    prepare_dft_stage(self, cur_size, iterations, blocks, cfalse);
            }

            if (self->stages.size() > 2)
                add_stage<intrinsics::dft_reorder_stage_impl<T>>(self, radices, radices_size);
        }
    }
}

} // namespace CMT_ARCH_NAME

} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)

CMT_PRAGMA_MSVC(warning(pop))
