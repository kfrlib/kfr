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

#include "../base/basic_expressions.hpp"
#include "../base/memory.hpp"
#include "../base/univector.hpp"
#include "../math/sin_cos.hpp"
#include "../simd/complex.hpp"
#include "../simd/constants.hpp"
#include <bitset>

CMT_PRAGMA_GNU(GCC diagnostic push)
#if CMT_HAS_WARNING("-Wshadow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
#endif
#if CMT_HAS_WARNING("-Wundefined-inline")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wundefined-inline")
#endif

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4100))

namespace kfr
{

#define DFT_MAX_STAGES 32

using cdirect_t = cfalse_t;
using cinvert_t = ctrue_t;

template <typename T>
struct dft_stage
{
    size_t radix      = 0;
    size_t stage_size = 0;
    size_t data_size  = 0;
    size_t temp_size  = 0;
    u8* data          = nullptr;
    size_t repeats    = 1;
    size_t out_offset = 0;
    size_t blocks     = 0;
    size_t user       = 0;
    const char* name  = nullptr;
    bool recursion    = false;
    bool can_inplace  = true;
    bool need_reorder = true;

    void initialize(size_t size) { do_initialize(size); }

    virtual void dump() const
    {
        printf("%s: %zu, %zu, %zu, %zu, %zu, %zu, %zu, %d, %d\n", name ? name : "unnamed", radix, stage_size,
               data_size, temp_size, repeats, out_offset, blocks, recursion, can_inplace);
    }

    KFR_MEM_INTRINSIC void execute(cdirect_t, complex<T>* out, const complex<T>* in, u8* temp)
    {
        do_execute(cdirect_t(), out, in, temp);
    }
    KFR_MEM_INTRINSIC void execute(cinvert_t, complex<T>* out, const complex<T>* in, u8* temp)
    {
        do_execute(cinvert_t(), out, in, temp);
    }
    virtual ~dft_stage() {}

protected:
    virtual void do_initialize(size_t) {}
    virtual void do_execute(cdirect_t, complex<T>*, const complex<T>*, u8* temp) = 0;
    virtual void do_execute(cinvert_t, complex<T>*, const complex<T>*, u8* temp) = 0;
};

enum class dft_type
{
    both,
    direct,
    inverse
};

enum class dft_order
{
    normal,
    internal, // possibly bit/digit-reversed, implementation-defined, may be faster to compute
};

enum class dft_pack_format
{
    Perm, // {X[0].r, X[N].r}, ... {X[i].r, X[i].i}, ... {X[N-1].r, X[N-1].i}
    CCs // {X[0].r, 0}, ... {X[i].r, X[i].i}, ... {X[N-1].r, X[N-1].i},  {X[N].r, 0}
};

template <typename T>
struct dft_plan;

template <typename T>
struct dft_plan_real;

template <typename T>
struct dft_stage;

template <typename T>
using dft_stage_ptr = std::unique_ptr<dft_stage<T>>;

template <typename T>
void dft_initialize(dft_plan<T>& plan);
template <typename T>
void dft_real_initialize(dft_plan_real<T>& plan);

/// @brief 1D DFT/FFT
template <typename T>
struct dft_plan
{
    size_t size;
    size_t temp_size;

    dft_plan()
        : size(0), temp_size(0), data_size(0), arblen(false), disposition_inplace{}, disposition_outofplace{}
    {
    }

    dft_plan(const dft_plan&)            = delete;
    dft_plan(dft_plan&&)                 = default;
    dft_plan& operator=(const dft_plan&) = delete;
    dft_plan& operator=(dft_plan&&)      = default;

    bool is_initialized() const { return size != 0; }

    [[deprecated("cpu parameter is deprecated. Runtime dispatch is used if built with "
                 "KFR_ENABLE_MULTIARCH")]] explicit dft_plan(cpu_t cpu, size_t size,
                                                             dft_order order = dft_order::normal)
        : dft_plan(size, order)
    {
        (void)cpu;
    }
    explicit dft_plan(size_t size, dft_order order = dft_order::normal)
        : size(size), temp_size(0), data_size(0), arblen(false)
    {
        dft_initialize(*this);
    }

    void dump() const
    {
        for (const std::unique_ptr<dft_stage<T>>& s : all_stages)
        {
            s->dump();
        }
    }

    KFR_MEM_INTRINSIC void execute(complex<T>* out, const complex<T>* in, u8* temp,
                                   bool inverse = false) const
    {
        if (inverse)
            execute_dft(ctrue, out, in, temp);
        else
            execute_dft(cfalse, out, in, temp);
    }
    ~dft_plan() {}
    template <bool inverse>
    KFR_MEM_INTRINSIC void execute(complex<T>* out, const complex<T>* in, u8* temp,
                                   cbool_t<inverse> inv) const
    {
        execute_dft(inv, out, in, temp);
    }

    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in,
                                   univector<u8, Tag3>& temp, bool inverse = false) const
    {
        if (inverse)
            execute_dft(ctrue, out.data(), in.data(), temp.data());
        else
            execute_dft(cfalse, out.data(), in.data(), temp.data());
    }
    template <bool inverse, univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in,
                                   univector<u8, Tag3>& temp, cbool_t<inverse> inv) const
    {
        execute_dft(inv, out.data(), in.data(), temp.data());
    }

    autofree<u8> data;
    size_t data_size;

    std::vector<dft_stage_ptr<T>> all_stages;
    std::array<std::vector<dft_stage<T>*>, 2> stages;
    bool arblen;
    using bitset = std::bitset<DFT_MAX_STAGES>;
    std::array<bitset, 2> disposition_inplace;
    std::array<bitset, 2> disposition_outofplace;

    void calc_disposition()
    {
        for (bool inverse : { false, true })
        {
            auto&& stages = this->stages[inverse];
            bitset can_inplace_per_stage;
            for (int i = 0; i < stages.size(); ++i)
            {
                can_inplace_per_stage[i] = stages[i]->can_inplace;
            }

            disposition_inplace[static_cast<int>(inverse)] =
                precompute_disposition(stages.size(), can_inplace_per_stage, true);
            disposition_outofplace[static_cast<int>(inverse)] =
                precompute_disposition(stages.size(), can_inplace_per_stage, false);
        }
    }

    static bitset precompute_disposition(int num_stages, bitset can_inplace_per_stage, bool inplace_requested)
    {
        static bitset even{ 0x5555555555555555ull };
        bitset mask = ~bitset() >> (DFT_MAX_STAGES - num_stages);
        bitset result;
        // disposition indicates where is input for corresponding stage
        // first bit : 0 - input,  1 - scratch
        // other bits: 0 - output, 1 - scratch

        // build disposition that works always
        if (num_stages % 2 == 0)
        { // even
            result = ~even & mask;
        }
        else
        { // odd
            result = even & mask;
        }

        int num_inplace = can_inplace_per_stage.count();

#ifdef KFR_DFT_ELIMINATE_MEMCPY
        if (num_inplace > 0 && inplace_requested)
        {
            if (result.test(0)) // input is in scratch
            {
                // num_inplace must be odd
                if (num_inplace % 2 == 0)
                    --num_inplace;
            }
            else
            {
                // num_inplace must be even
                if (num_inplace % 2 != 0)
                    --num_inplace;
            }
        }
#endif
        if (num_inplace > 0)
        {
            for (int i = num_stages - 1; i >= 0; --i)
            {
                if (can_inplace_per_stage.test(i))
                {
                    result ^= ~bitset() >> (DFT_MAX_STAGES - (i + 1));

                    if (--num_inplace == 0)
                        break;
                }
            }
        }

        if (!inplace_requested) // out-of-place first stage; IN->OUT
            result.reset(0);

        return result;
    }

protected:
    struct noinit
    {
    };
    explicit dft_plan(noinit, size_t size, dft_order order = dft_order::normal)
        : size(size), temp_size(0), data_size(0), arblen(false)
    {
    }
    const complex<T>* select_in(bitset disposition, size_t stage, const complex<T>* out, const complex<T>* in,
                                const complex<T>* scratch) const
    {
        return disposition.test(stage) ? scratch : stage == 0 ? in : out;
    }
    complex<T>* select_out(bitset disposition, size_t stage, size_t total_stages, complex<T>* out,
                           complex<T>* scratch) const
    {
        return stage == total_stages - 1 ? out : disposition.test(stage + 1) ? scratch : out;
    }

    template <bool inverse>
    void execute_dft(cbool_t<inverse>, complex<T>* out, const complex<T>* in, u8* temp) const
    {
        auto&& stages = this->stages[inverse];
        if (stages.size() == 1 && (stages[0]->can_inplace || in != out))
        {
            return stages[0]->execute(cbool<inverse>, out, in, temp);
        }
        size_t stack[DFT_MAX_STAGES] = { 0 };

        bitset disposition =
            in == out ? this->disposition_inplace[inverse] : this->disposition_outofplace[inverse];

        complex<T>* scratch = ptr_cast<complex<T>>(
            temp + this->temp_size -
            align_up(sizeof(complex<T>) * this->size, platform<>::native_cache_alignment));

        bool in_scratch = disposition.test(0);
        if (in_scratch)
        {
            builtin_memcpy(scratch, in, sizeof(complex<T>) * this->size);
        }

        const size_t count = stages.size();

        for (size_t depth = 0; depth < count;)
        {
            if (stages[depth]->recursion)
            {
                size_t offset   = 0;
                size_t rdepth   = depth;
                size_t maxdepth = depth;
                do
                {
                    if (stack[rdepth] == stages[rdepth]->repeats)
                    {
                        stack[rdepth] = 0;
                        rdepth--;
                    }
                    else
                    {
                        complex<T>* rout      = select_out(disposition, rdepth, stages.size(), out, scratch);
                        const complex<T>* rin = select_in(disposition, rdepth, out, in, scratch);
                        stages[rdepth]->execute(cbool<inverse>, rout + offset, rin + offset, temp);
                        offset += stages[rdepth]->out_offset;
                        stack[rdepth]++;
                        if (rdepth < count - 1 && stages[rdepth + 1]->recursion)
                            rdepth++;
                        else
                            maxdepth = rdepth;
                    }
                } while (rdepth != depth);
                depth = maxdepth + 1;
            }
            else
            {
                size_t offset = 0;
                while (offset < this->size)
                {
                    stages[depth]->execute(
                        cbool<inverse>, select_out(disposition, depth, stages.size(), out, scratch) + offset,
                        select_in(disposition, depth, out, in, scratch) + offset, temp);
                    offset += stages[depth]->stage_size;
                }
                depth++;
            }
        }
    }
};

/// @brief Real-to-complex and Complex-to-real 1D DFT
template <typename T>
struct dft_plan_real : dft_plan<T>
{
    size_t size;
    dft_pack_format fmt;

    dft_plan_real() : size(0), fmt(dft_pack_format::CCs) {}

    dft_plan_real(const dft_plan_real&)            = delete;
    dft_plan_real(dft_plan_real&&)                 = default;
    dft_plan_real& operator=(const dft_plan_real&) = delete;
    dft_plan_real& operator=(dft_plan_real&&)      = default;

    bool is_initialized() const { return size != 0; }

    [[deprecated("cpu parameter is deprecated. Runtime dispatch is used if built with "
                 "KFR_ENABLE_MULTIARCH")]] explicit dft_plan_real(cpu_t cpu, size_t size,
                                                                  dft_pack_format fmt = dft_pack_format::CCs)
        : dft_plan_real(size, fmt)
    {
        (void)cpu;
    }

    explicit dft_plan_real(size_t size, dft_pack_format fmt = dft_pack_format::CCs)
        : dft_plan<T>(typename dft_plan<T>::noinit{}, size / 2), size(size), fmt(fmt)
    {
        KFR_LOGIC_CHECK(is_even(size), "dft_plan_real requires size to be even");
        dft_real_initialize(*this);
    }

    void execute(complex<T>*, const complex<T>*, u8*, bool = false) const = delete;

    template <bool inverse>
    void execute(complex<T>*, const complex<T>*, u8*, cbool_t<inverse>) const = delete;

    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    void execute(univector<complex<T>, Tag1>&, const univector<complex<T>, Tag2>&, univector<u8, Tag3>&,
                 bool = false) const = delete;

    template <bool inverse, univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    void execute(univector<complex<T>, Tag1>&, const univector<complex<T>, Tag2>&, univector<u8, Tag3>&,
                 cbool_t<inverse>) const = delete;

    KFR_MEM_INTRINSIC void execute(complex<T>* out, const T* in, u8* temp, cdirect_t = {}) const
    {
        this->execute_dft(cfalse, out, ptr_cast<complex<T>>(in), temp);
    }
    KFR_MEM_INTRINSIC void execute(T* out, const complex<T>* in, u8* temp, cinvert_t = {}) const
    {
        this->execute_dft(ctrue, ptr_cast<complex<T>>(out), in, temp);
    }

    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<T, Tag2>& in,
                                   univector<u8, Tag3>& temp, cdirect_t = {}) const
    {
        this->execute_dft(cfalse, out.data(), ptr_cast<complex<T>>(in.data()), temp.data());
    }
    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    KFR_MEM_INTRINSIC void execute(univector<T, Tag1>& out, const univector<complex<T>, Tag2>& in,
                                   univector<u8, Tag3>& temp, cinvert_t = {}) const
    {
        this->execute_dft(ctrue, ptr_cast<complex<T>>(out.data()), in.data(), temp.data());
    }

    // Deprecated. fmt must be passed to constructor instead
    void execute(complex<T>*, const T*, u8*, dft_pack_format) const = delete;
    void execute(T*, const complex<T>*, u8*, dft_pack_format) const = delete;

    // Deprecated. fmt must be passed to constructor instead
    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    void execute(univector<complex<T>, Tag1>&, const univector<T, Tag2>&, univector<u8, Tag3>&,
                 dft_pack_format) const = delete;
    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    void execute(univector<T, Tag1>&, const univector<complex<T>, Tag2>&, univector<u8, Tag3>&,
                 dft_pack_format) const = delete;
};

/// @brief DCT type 2 (unscaled)
template <typename T>
struct dct_plan : dft_plan<T>
{
    dct_plan(size_t size) : dft_plan<T>(size) { this->temp_size += sizeof(complex<T>) * size * 2; }

    [[deprecated("cpu parameter is deprecated. Runtime dispatch is used if built with "
                 "KFR_ENABLE_MULTIARCH")]] dct_plan(cpu_t cpu, size_t size)
        : dct_plan(size)
    {
    }

    KFR_MEM_INTRINSIC void execute(T* out, const T* in, u8* temp, bool inverse = false) const
    {
        const size_t size                  = this->size;
        const size_t halfSize              = size / 2;
        univector_ref<complex<T>> mirrored = make_univector(
            ptr_cast<complex<T>>(temp + this->temp_size - sizeof(complex<T>) * size * 2), size);
        univector_ref<complex<T>> mirrored_dft =
            make_univector(ptr_cast<complex<T>>(temp + this->temp_size - sizeof(complex<T>) * size), size);
        auto t = counter() * c_pi<T> / (size * 2);
        if (!inverse)
        {
            for (size_t i = 0; i < halfSize; i++)
            {
                mirrored[i]            = in[i * 2];
                mirrored[size - 1 - i] = in[i * 2 + 1];
            }
            if (size % 2)
            {
                mirrored[halfSize] = in[size - 1];
            }
            dft_plan<T>::execute(mirrored_dft.data(), mirrored.data(), temp, cfalse);
            make_univector(out, size) = real(mirrored_dft) * cos(t) + imag(mirrored_dft) * sin(t);
        }
        else
        {
            mirrored    = make_complex(make_univector(in, size) * cos(t), make_univector(in, size) * -sin(t));
            mirrored[0] = mirrored[0] * T(0.5);
            dft_plan<T>::execute(mirrored_dft.data(), mirrored.data(), temp, cfalse);
            for (size_t i = 0; i < halfSize; i++)
            {
                out[i * 2 + 0] = mirrored_dft[i].real();
                out[i * 2 + 1] = mirrored_dft[size - 1 - i].real();
            }
            if (size % 2)
            {
                out[size - 1] = mirrored_dft[halfSize].real();
            }
        }
    }

    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    KFR_MEM_INTRINSIC void execute(univector<T, Tag1>& out, const univector<T, Tag2>& in,
                                   univector<u8, Tag3>& temp, bool inverse = false) const
    {
        execute(out.data(), in.data(), temp.data(), inverse);
    }
};

inline namespace CMT_ARCH_NAME
{

template <typename T, univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
void fft_multiply(univector<complex<T>, Tag1>& dest, const univector<complex<T>, Tag2>& src1,
                  const univector<complex<T>, Tag3>& src2, dft_pack_format fmt = dft_pack_format::CCs)
{
    const complex<T> f0(src1[0].real() * src2[0].real(), src1[0].imag() * src2[0].imag());

    dest = src1 * src2;

    if (fmt == dft_pack_format::Perm)
        dest[0] = f0;
}

template <typename T, univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
void fft_multiply_accumulate(univector<complex<T>, Tag1>& dest, const univector<complex<T>, Tag2>& src1,
                             const univector<complex<T>, Tag3>& src2,
                             dft_pack_format fmt = dft_pack_format::CCs)
{
    const complex<T> f0(dest[0].real() + src1[0].real() * src2[0].real(),
                        dest[0].imag() + src1[0].imag() * src2[0].imag());

    dest = dest + src1 * src2;

    if (fmt == dft_pack_format::Perm)
        dest[0] = f0;
}
template <typename T, univector_tag Tag1, univector_tag Tag2, univector_tag Tag3, univector_tag Tag4>
void fft_multiply_accumulate(univector<complex<T>, Tag1>& dest, const univector<complex<T>, Tag2>& src1,
                             const univector<complex<T>, Tag3>& src2, const univector<complex<T>, Tag4>& src3,
                             dft_pack_format fmt = dft_pack_format::CCs)
{
    const complex<T> f0(src1[0].real() + src2[0].real() * src3[0].real(),
                        src1[0].imag() + src2[0].imag() * src3[0].imag());

    dest = src1 + src2 * src3;

    if (fmt == dft_pack_format::Perm)
        dest[0] = f0;
}
} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)

CMT_PRAGMA_MSVC(warning(pop))
