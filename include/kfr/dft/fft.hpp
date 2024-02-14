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
#include "../base/tensor.hpp"
#include "../base/univector.hpp"
#include "../math/sin_cos.hpp"
#include "../simd/complex.hpp"
#include "../simd/constants.hpp"
#include <bitset>
#include <functional>

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
    virtual void copy_input(bool invert, complex<T>* out, const complex<T>* in, size_t size)
    {
        builtin_memcpy(out, in, sizeof(complex<T>) * size);
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

namespace internal_generic
{
template <typename T>
void dft_initialize(dft_plan<T>& plan);
template <typename T>
void dft_real_initialize(dft_plan_real<T>& plan);
template <typename T, bool inverse>
void dft_execute(const dft_plan<T>& plan, cbool_t<inverse>, complex<T>* out, const complex<T>* in, u8* temp);

template <typename T>
using fn_transpose = void (*)(complex<T>*, const complex<T>*, shape<2>);
template <typename T>
void dft_initialize_transpose(fn_transpose<T>& transpose);

} // namespace internal_generic

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
        internal_generic::dft_initialize(*this);
    }

    void dump() const;

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

    template <univector_tag Tag1, univector_tag Tag2>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in,
                                   u8* temp, bool inverse = false) const
    {
        if (inverse)
            execute_dft(ctrue, out.data(), in.data(), temp);
        else
            execute_dft(cfalse, out.data(), in.data(), temp);
    }
    template <bool inverse, univector_tag Tag1, univector_tag Tag2>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in,
                                   u8* temp, cbool_t<inverse> inv) const
    {
        execute_dft(inv, out.data(), in.data(), temp);
    }

    autofree<u8> data;
    size_t data_size;

    std::vector<dft_stage_ptr<T>> all_stages;
    std::array<std::vector<dft_stage<T>*>, 2> stages;
    bool arblen;
    using bitset = std::bitset<DFT_MAX_STAGES>;
    std::array<bitset, 2> disposition_inplace;
    std::array<bitset, 2> disposition_outofplace;

    void calc_disposition();

    static bitset precompute_disposition(int num_stages, bitset can_inplace_per_stage,
                                         bool inplace_requested);

protected:
    struct noinit
    {
    };
    explicit dft_plan(noinit, size_t size, dft_order order = dft_order::normal)
        : size(size), temp_size(0), data_size(0), arblen(false)
    {
    }

    template <bool inverse>
    KFR_INTRINSIC void execute_dft(cbool_t<inverse>, complex<T>* out, const complex<T>* in, u8* temp) const
    {
        internal_generic::dft_execute(*this, cbool<inverse>, out, in, temp);
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

    size_t complex_size() const { return complex_size_for(size, fmt); }
    constexpr static size_t complex_size_for(size_t size, dft_pack_format fmt)
    {
        return fmt == dft_pack_format::CCs ? size / 2 + 1 : size / 2;
    }

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
        internal_generic::dft_real_initialize(*this);
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

    template <univector_tag Tag1, univector_tag Tag2>
    void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in, u8* temp,
                 bool inverse = false) const = delete;

    template <bool inverse, univector_tag Tag1, univector_tag Tag2>
    void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in, u8* temp,
                 cbool_t<inverse> inv) const = delete;

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

    template <univector_tag Tag1, univector_tag Tag2>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<T, Tag2>& in, u8* temp,
                                   cdirect_t = {}) const
    {
        this->execute_dft(cfalse, out.data(), ptr_cast<complex<T>>(in.data()), temp);
    }
    template <univector_tag Tag1, univector_tag Tag2>
    KFR_MEM_INTRINSIC void execute(univector<T, Tag1>& out, const univector<complex<T>, Tag2>& in, u8* temp,
                                   cinvert_t = {}) const
    {
        this->execute_dft(ctrue, ptr_cast<complex<T>>(out.data()), in.data(), temp);
    }
};

/// @brief Multidimensional DFT
template <typename T, index_t Dims>
struct dft_plan_md
{
    shape<Dims> size;
    size_t temp_size;

    dft_plan_md(const dft_plan_md&)            = delete;
    dft_plan_md(dft_plan_md&&)                 = default;
    dft_plan_md& operator=(const dft_plan_md&) = delete;
    dft_plan_md& operator=(dft_plan_md&&)      = default;

    bool is_initialized() const { return size.product() != 0; }

    void dump() const
    {
        for (const auto& d : dfts)
        {
            d.dump();
        }
    }

    explicit dft_plan_md(shape<Dims> size) : size(std::move(size)), temp_size(0)
    {
        if constexpr (Dims == dynamic_shape)
        {
            dfts.resize(this->size.dims());
        }
        for (index_t i = 0; i < this->size.dims(); ++i)
        {
            dfts[i]   = dft_plan<T>(this->size[i]);
            temp_size = std::max(temp_size, dfts[i].temp_size);
        }
        internal_generic::dft_initialize_transpose(transpose);
    }

    void execute(complex<T>* out, const complex<T>* in, u8* temp, bool inverse = false) const
    {
        if (inverse)
            execute_dft(ctrue, out, in, temp);
        else
            execute_dft(cfalse, out, in, temp);
    }

    template <index_t UDims = Dims, CMT_ENABLE_IF(UDims != dynamic_shape)>
    void execute(const tensor<complex<T>, Dims>& out, const tensor<complex<T>, Dims>& in, u8* temp,
                 bool inverse = false) const
    {
        KFR_LOGIC_CHECK(in.shape() == this->size && out.shape() == this->size,
                        "dft_plan_md: incorrect tensor shapes");
        KFR_LOGIC_CHECK(in.is_contiguous() && out.is_contiguous(), "dft_plan_md: tensors must be contiguous");
        if (inverse)
            execute_dft(ctrue, out.data(), in.data(), temp);
        else
            execute_dft(cfalse, out.data(), in.data(), temp);
    }
    template <bool inverse = false>
    void execute(complex<T>* out, const complex<T>* in, u8* temp, cbool_t<inverse> = {}) const
    {
        execute_dft(cbool<inverse>, out, in, temp);
    }

private:
    template <bool inverse>
    KFR_INTRINSIC void execute_dft(cbool_t<inverse>, complex<T>* out, const complex<T>* in, u8* temp) const
    {
        if (temp == nullptr && temp_size > 0)
        {
            return call_with_temp(temp_size, std::bind(&dft_plan_md<T, Dims>::execute_dft<inverse>, this,
                                                       cbool_t<inverse>{}, out, in, std::placeholders::_1));
        }
        if (size.dims() == 1)
        {
            dfts[0].execute(out, in, temp, cbool<inverse>);
        }
        else
        {
            execute_dim(cbool<inverse>, out, in, temp);
        }
    }
    KFR_INTRINSIC void execute_dim(cfalse_t, complex<T>* out, const complex<T>* in, u8* temp) const
    {
        shape<Dims> sh = size;
        index_t total  = size.product();
        index_t axis   = size.dims() - 1;
        for (;;)
        {
            if (size[axis] > 1)
            {
                for (index_t o = 0; o < total; o += sh.back())
                    dfts[axis].execute(out + o, in + o, temp, cfalse);
            }
            else
            {
                builtin_memcpy(out, in, sizeof(complex<T>) * total);
            }

            transpose(out, out, shape{ sh.remove_back().product(), sh.back() });

            if (axis == 0)
                break;

            sh = sh.rotate_right();
            in = out;
            --axis;
        }
    }
    KFR_INTRINSIC void execute_dim(ctrue_t, complex<T>* out, const complex<T>* in, u8* temp) const
    {
        shape<Dims> sh = size;
        index_t total  = size.product();
        index_t axis   = 0;
        for (;;)
        {
            transpose(out, in, shape{ sh.front(), sh.remove_front().product() });

            if (size[axis] > 1)
            {
                for (index_t o = 0; o < total; o += sh.front())
                    dfts[axis].execute(out + o, out + o, temp, ctrue);
            }

            if (axis == size.dims() - 1)
                break;

            sh = sh.rotate_left();
            in = out;
            ++axis;
        }
    }
    using dft_list =
        std::conditional_t<Dims == dynamic_shape, std::vector<dft_plan<T>>, std::array<dft_plan<T>, Dims>>;
    dft_list dfts;
    internal_generic::fn_transpose<T> transpose;
};

/// @brief Multidimensional DFT
template <typename T, index_t Dims>
struct dft_plan_md_real
{
    shape<Dims> size;
    size_t temp_size;
    bool real_out_is_enough;

    dft_plan_md_real(const dft_plan_md_real&)            = delete;
    dft_plan_md_real(dft_plan_md_real&&)                 = default;
    dft_plan_md_real& operator=(const dft_plan_md_real&) = delete;
    dft_plan_md_real& operator=(dft_plan_md_real&&)      = default;

    bool is_initialized() const { return size.product() != 0; }

    void dump() const
    {
        for (const auto& d : dfts)
        {
            d.dump();
        }
        dft_real.dump();
    }

    shape<Dims> complex_size() const { return complex_size_for(size); }
    constexpr static shape<Dims> complex_size_for(shape<Dims> size)
    {
        if (size.dims() > 0)
            size.back() = dft_plan_real<T>::complex_size_for(size.back(), dft_pack_format::CCs);
        return size;
    }

    size_t real_out_size() const { return real_out_size_for(size); }
    constexpr static size_t real_out_size_for(shape<Dims> size)
    {
        return complex_size_for(size).product() * 2;
    }

    explicit dft_plan_md_real(shape<Dims> size, bool real_out_is_enough = false)
        : size(std::move(size)), temp_size(0), real_out_is_enough(real_out_is_enough)
    {
        if (this->size.dims() > 0)
        {
            if constexpr (Dims == dynamic_shape)
            {
                dfts.resize(this->size.dims());
            }
            for (index_t i = 0; i < this->size.dims() - 1; ++i)
            {
                dfts[i]   = dft_plan<T>(this->size[i]);
                temp_size = std::max(temp_size, dfts[i].temp_size);
            }
            dft_real  = dft_plan_real<T>(this->size.back());
            temp_size = std::max(temp_size, dft_real.temp_size);
        }
        if (!this->real_out_is_enough)
        {
            temp_size += complex_size().product() * sizeof(complex<T>);
        }
        internal_generic::dft_initialize_transpose(transpose);
    }

    void execute(complex<T>* out, const T* in, u8* temp, cdirect_t = {}) const
    {
        execute_dft(cfalse, out, in, temp);
    }
    void execute(T* out, const complex<T>* in, u8* temp, cinvert_t = {}) const
    {
        execute_dft(ctrue, out, in, temp);
    }

    template <index_t UDims = Dims, CMT_ENABLE_IF(UDims != dynamic_shape)>
    void execute(const tensor<complex<T>, Dims>& out, const tensor<T, Dims>& in, u8* temp,
                 cdirect_t = {}) const
    {
        KFR_LOGIC_CHECK(in.shape() == this->size && out.shape() == complex_size(),
                        "dft_plan_md_real: incorrect tensor shapes");
        KFR_LOGIC_CHECK(in.is_contiguous() && out.is_contiguous(),
                        "dft_plan_md_real: tensors must be contiguous");
        execute_dft(cfalse, out.data(), in.data(), temp);
    }
    template <index_t UDims = Dims, CMT_ENABLE_IF(UDims != dynamic_shape)>
    void execute(const tensor<T, Dims>& out, const tensor<complex<T>, Dims>& in, u8* temp,
                 cinvert_t = {}) const
    {
        KFR_LOGIC_CHECK(in.shape() == complex_size() && out.shape() == this->size,
                        "dft_plan_md_real: incorrect tensor shapes");
        KFR_LOGIC_CHECK(in.is_contiguous() && out.is_contiguous(),
                        "dft_plan_md_real: tensors must be contiguous");
        execute_dft(ctrue, out.data(), in.data(), temp);
    }
    void execute(complex<T>* out, const T* in, u8* temp, bool inverse) const
    {
        KFR_LOGIC_CHECK(inverse, "dft_plan_md_real: incorrect usage");
        execute_dft(cfalse, out, in, temp);
    }
    void execute(T* out, const complex<T>* in, u8* temp, bool inverse) const
    {
        KFR_LOGIC_CHECK(!inverse, "dft_plan_md_real: incorrect usage");
        execute_dft(ctrue, out, in, temp);
    }

private:
    template <bool inverse, typename Tout, typename Tin>
    KFR_INTRINSIC void execute_dft(cbool_t<inverse>, Tout* out, const Tin* in, u8* temp) const
    {
        if (temp == nullptr && temp_size > 0)
        {
            return call_with_temp(temp_size,
                                  std::bind(&dft_plan_md_real<T, Dims>::execute_dft<inverse, Tout, Tin>, this,
                                            cbool_t<inverse>{}, out, in, std::placeholders::_1));
        }
        if (this->size.dims() == 1)
        {
            dft_real.execute(out, in, temp, cbool<inverse>);
        }
        else
        {
            execute_dim(cbool<inverse>, out, in, temp);
        }
    }
    void expand(T* out, const T* in, size_t count, size_t last_axis) const
    {
        size_t last_axis_ex = dft_real.complex_size() * 2;
        if (in != out)
        {
            builtin_memmove(out, in, last_axis * sizeof(T));
        }
        in += last_axis * (count - 1);
        out += last_axis_ex * (count - 1);
        for (size_t i = 1; i < count; ++i)
        {
            builtin_memmove(out, in, last_axis * sizeof(T));
            in -= last_axis;
            out -= last_axis_ex;
        }
#ifdef KFR_DEBUG
        for (size_t i = 0; i < count; ++i)
        {
            builtin_memset(out + last_axis, 0xFF, (last_axis_ex - last_axis) * sizeof(T));
            out += last_axis_ex;
        }
#endif
    }
    void contract(T* out, const T* in, size_t count, size_t last_axis) const
    {
        size_t last_axis_ex = dft_real.complex_size() * 2;
        if (in != out)
            builtin_memmove(out, in, last_axis * sizeof(T));
        in += last_axis_ex;
        out += last_axis;
        for (size_t i = 1; i < count; ++i)
        {
            builtin_memmove(out, in, last_axis * sizeof(T));
            in += last_axis_ex;
            out += last_axis;
        }
    }
    KFR_INTRINSIC void execute_dim(cfalse_t, complex<T>* out, const T* in_real, u8* temp) const
    {
        shape<Dims> sh = complex_size();
        index_t total  = sh.product();
        index_t axis   = size.dims() - 1;
        expand(ptr_cast<T>(out), in_real, size.remove_back().product(), size.back());
        for (;;)
        {
            if (size[axis] > 1)
            {
                if (axis == size.dims() - 1)
                    for (index_t o = 0; o < total; o += sh.back())
                        dft_real.execute(out + o, ptr_cast<T>(out + o), temp, cfalse);
                else
                    for (index_t o = 0; o < total; o += sh.back())
                        dfts[axis].execute(out + o, out + o, temp, cfalse);
            }

            transpose(out, out, shape{ sh.remove_back().product(), sh.back() });

            if (axis == 0)
                break;

            sh = sh.rotate_right();
            --axis;
        }
    }
    KFR_INTRINSIC void execute_dim(ctrue_t, T* out_real, const complex<T>* in, u8* temp) const
    {
        shape<Dims> sh  = complex_size();
        index_t total   = sh.product();
        complex<T>* out = real_out_is_enough
                              ? ptr_cast<complex<T>>(out_real)
                              : ptr_cast<complex<T>>(temp + temp_size - total * sizeof(complex<T>));
        index_t axis    = 0;
        for (;;)
        {
            transpose(out, in, shape{ sh.front(), sh.remove_front().product() });

            if (size[axis] > 1)
            {
                if (axis == size.dims() - 1)
                    for (index_t o = 0; o < total; o += sh.front())
                        dft_real.execute(ptr_cast<T>(out + o), out + o, temp, ctrue);
                else
                    for (index_t o = 0; o < total; o += sh.front())
                        dfts[axis].execute(out + o, out + o, temp, ctrue);
            }

            if (axis == size.dims() - 1)
                break;

            sh = sh.rotate_left();
            in = out;
            ++axis;
        }
        contract(out_real, ptr_cast<T>(out), size.remove_back().product(), size.back());
    }
    using dft_list = std::conditional_t<Dims == dynamic_shape, std::vector<dft_plan<T>>,
                                        std::array<dft_plan<T>, const_max(Dims, 1) - 1>>;
    dft_list dfts;
    dft_plan_real<T> dft_real;
    internal_generic::fn_transpose<T> transpose;
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
