/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "../base/complex.hpp"
#include "../base/constants.hpp"
#include "../base/memory.hpp"
#include "../base/read_write.hpp"
#include "../base/small_buffer.hpp"
#include "../base/univector.hpp"
#include "../base/vec.hpp"

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

enum class dft_type
{
    both,
    direct,
    inverse
};

template <typename T>
struct dft_stage;

template <typename T>
struct dft_plan
{
    using dft_stage_ptr = std::unique_ptr<dft_stage<T>>;

    size_t size;
    size_t temp_size;

    dft_plan(size_t size, dft_type = dft_type::both);

    KFR_INTRIN void execute(complex<T>* out, const complex<T>* in, u8* temp, bool inverse = false) const
    {
        if (inverse)
            execute_dft(ctrue, out, in, temp);
        else
            execute_dft(cfalse, out, in, temp);
    }
    ~dft_plan();
    template <bool inverse>
    KFR_INTRIN void execute(complex<T>* out, const complex<T>* in, u8* temp, cbool_t<inverse> inv) const
    {
        execute_dft(inv, out, in, temp);
    }

    template <size_t Tag1, size_t Tag2, size_t Tag3>
    KFR_INTRIN void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in,
                            univector<u8, Tag3>& temp, bool inverse = false) const
    {
        if (inverse)
            execute_dft(ctrue, out.data(), in.data(), temp.data());
        else
            execute_dft(cfalse, out.data(), in.data(), temp.data());
    }
    template <bool inverse, size_t Tag1, size_t Tag2, size_t Tag3>
    KFR_INTRIN void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in,
                            univector<u8, Tag3>& temp, cbool_t<inverse> inv) const
    {
        execute_dft(inv, out.data(), in.data(), temp.data());
    }

protected:
    autofree<u8> data;
    size_t data_size;
    std::vector<dft_stage_ptr> stages;

    template <typename Stage, typename... Args>
    void add_stage(Args... args);

    template <bool is_final>
    void prepare_dft_stage(size_t radix, size_t iterations, size_t blocks, cbool_t<is_final>);

    template <bool is_even, bool first>
    void make_fft(size_t stage_size, cbool_t<is_even>, cbool_t<first>);

    void initialize();
    template <bool inverse>
    void execute_dft(cbool_t<inverse>, complex<T>* out, const complex<T>* in, u8* temp) const;

    const complex<T>* select_in(size_t stage, const complex<T>* out, const complex<T>* in, const complex<T>* scratch) const;
    complex<T>* select_out(size_t stage, complex<T>* out, complex<T>* scratch) const;
};

enum class dft_pack_format
{
    Perm, // {X[0].r, X[N].r}, ... {X[i].r, X[i].i}, ... {X[N-1].r, X[N-1].i}
    CCs // {X[0].r, 0}, ... {X[i].r, X[i].i}, ... {X[N-1].r, X[N-1].i},  {X[N].r, 0}
};

template <typename T>
struct dft_plan_real : dft_plan<T>
{
    size_t size;
    dft_plan_real(size_t size, dft_type = dft_type::both);

    void execute(complex<T>*, const complex<T>*, u8*, bool = false) const = delete;

    template <bool inverse>
    void execute(complex<T>*, const complex<T>*, u8*, cbool_t<inverse>) const = delete;

    template <size_t Tag1, size_t Tag2, size_t Tag3>
    void execute(univector<complex<T>, Tag1>&, const univector<complex<T>, Tag2>&, univector<u8, Tag3>&,
                 bool = false) const = delete;

    template <bool inverse, size_t Tag1, size_t Tag2, size_t Tag3>
    void execute(univector<complex<T>, Tag1>&, const univector<complex<T>, Tag2>&, univector<u8, Tag3>&,
                 cbool_t<inverse>) const = delete;

    KFR_INTRIN void execute(complex<T>* out, const T* in, u8* temp,
                            dft_pack_format fmt = dft_pack_format::CCs) const
    {
        this->execute_dft(cfalse, out, ptr_cast<complex<T>>(in), temp);
        to_fmt(out, fmt);
    }
    KFR_INTRIN void execute(T* out, const complex<T>* in, u8* temp,
                            dft_pack_format fmt = dft_pack_format::CCs) const
    {
        complex<T>* outdata = ptr_cast<complex<T>>(out);
        from_fmt(outdata, in, fmt);
        this->execute_dft(ctrue, outdata, outdata, temp);
    }

    template <size_t Tag1, size_t Tag2, size_t Tag3>
    KFR_INTRIN void execute(univector<complex<T>, Tag1>& out, const univector<T, Tag2>& in,
                            univector<u8, Tag3>& temp, dft_pack_format fmt = dft_pack_format::CCs) const
    {
        this->execute_dft(cfalse, out.data(), ptr_cast<complex<T>>(in.data()), temp.data());
        to_fmt(out.data(), fmt);
    }
    template <size_t Tag1, size_t Tag2, size_t Tag3>
    KFR_INTRIN void execute(univector<T, Tag1>& out, const univector<complex<T>, Tag2>& in,
                            univector<u8, Tag3>& temp, dft_pack_format fmt = dft_pack_format::CCs) const
    {
        complex<T>* outdata = ptr_cast<complex<T>>(out.data());
        from_fmt(outdata, in.data(), fmt);
        this->execute_dft(ctrue, outdata, outdata, temp.data());
    }

private:
    univector<complex<T>> rtwiddle;

    void to_fmt(complex<T>* out, dft_pack_format fmt) const;
    void from_fmt(complex<T>* out, const complex<T>* in, dft_pack_format fmt) const;
};

template <typename T, size_t Tag1, size_t Tag2, size_t Tag3>
void fft_multiply(univector<complex<T>, Tag1>& dest, const univector<complex<T>, Tag2>& src1,
                  const univector<complex<T>, Tag3>& src2, dft_pack_format fmt = dft_pack_format::CCs)
{
    const complex<T> f0(src1[0].real() * src2[0].real(), src1[0].imag() * src2[0].imag());

    dest = src1 * src2;

    if (fmt == dft_pack_format::Perm)
        dest[0] = f0;
}

template <typename T, size_t Tag1, size_t Tag2, size_t Tag3>
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
template <typename T, size_t Tag1, size_t Tag2, size_t Tag3, size_t Tag4>
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
} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)

CMT_PRAGMA_MSVC(warning(pop))
