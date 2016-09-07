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
#include "../base/vec.hpp"
#include <cmath>

namespace kfr
{

template <typename Tnumber = long double>
void reference_fft_pass(Tnumber pi2, size_t N, size_t offset, size_t delta, int flag, Tnumber (*x)[2],
                        Tnumber (*X)[2], Tnumber (*XX)[2])
{
    const size_t N2 = N / 2;
    using std::sin;
    using std::cos;

    if (N != 2)
    {
        reference_fft_pass(pi2, N2, offset, 2 * delta, flag, x, XX, X);
        reference_fft_pass(pi2, N2, offset + delta, 2 * delta, flag, x, XX, X);

        for (size_t k = 0; k < N2; k++)
        {
            const size_t k00   = offset + k * delta;
            const size_t k01   = k00 + N2 * delta;
            const size_t k10   = offset + 2 * k * delta;
            const size_t k11   = k10 + delta;
            const Tnumber m    = static_cast<Tnumber>(k) / N;
            const Tnumber cs   = cos(pi2 * m);
            const Tnumber sn   = flag * sin(pi2 * m);
            const Tnumber tmp0 = cs * XX[k11][0] + sn * XX[k11][1];
            const Tnumber tmp1 = cs * XX[k11][1] - sn * XX[k11][0];
            X[k01][0]          = XX[k10][0] - tmp0;
            X[k01][1]          = XX[k10][1] - tmp1;
            X[k00][0]          = XX[k10][0] + tmp0;
            X[k00][1]          = XX[k10][1] + tmp1;
        }
    }
    else
    {
        const size_t k00 = offset;
        const size_t k01 = k00 + delta;
        X[k01][0]        = x[k00][0] - x[k01][0];
        X[k01][1]        = x[k00][1] - x[k01][1];
        X[k00][0]        = x[k00][0] + x[k01][0];
        X[k00][1]        = x[k00][1] + x[k01][1];
    }
}

template <typename Tnumber = long double, typename T>
void reference_fft(complex<T>* out, const complex<T>* in, size_t size, bool inversion = false)
{
    using Tcmplx = Tnumber(*)[2];
    if (size < 2)
        return;
    std::vector<complex<Tnumber>> datain(size);
    std::vector<complex<Tnumber>> dataout(size);
    std::vector<complex<Tnumber>> temp(size);
    std::copy(in, in + size, datain.begin());
    const Tnumber pi2 = c_pi<Tnumber, 2, 1>;
    reference_fft_pass<Tnumber>(pi2, size, 0, 1, inversion ? -1 : +1, Tcmplx(datain.data()),
                                Tcmplx(dataout.data()), Tcmplx(temp.data()));
    std::copy(dataout.begin(), dataout.end(), out);
}

template <typename Tnumber = long double, typename T>
void reference_fft(complex<T>* out, const T* in, size_t size)
{
    constexpr bool inversion = false;
    using Tcmplx             = Tnumber(*)[2];
    if (size < 2)
        return;
    std::vector<complex<Tnumber>> datain(size);
    std::vector<complex<Tnumber>> dataout(size);
    std::vector<complex<Tnumber>> temp(size);
    std::copy(in, in + size, datain.begin());
    const Tnumber pi2 = c_pi<Tnumber, 2, 1>;
    reference_fft_pass<Tnumber>(pi2, size, 0, 1, inversion ? -1 : +1, Tcmplx(datain.data()),
                                Tcmplx(dataout.data()), Tcmplx(temp.data()));
    std::copy(dataout.begin(), dataout.end(), out);
}

template <typename Tnumber = long double, typename T>
void reference_fft(T* out, const complex<T>* in, size_t size)
{
    constexpr bool inversion = true;
    using Tcmplx             = Tnumber(*)[2];
    if (size < 2)
        return;
    std::vector<complex<Tnumber>> datain(size);
    std::vector<complex<Tnumber>> dataout(size);
    std::vector<complex<Tnumber>> temp(size);
    std::copy(in, in + size, datain.begin());
    const Tnumber pi2 = c_pi<Tnumber, 2, 1>;
    reference_fft_pass<Tnumber>(pi2, size, 0, 1, inversion ? -1 : +1, Tcmplx(datain.data()),
                                Tcmplx(dataout.data()), Tcmplx(temp.data()));
    for (size_t i = 0; i < size; i++)
        out[i]    = dataout[i].real();
}

template <typename Tnumber = long double, typename T>
void reference_dft(complex<T>* out, const complex<T>* in, size_t size, bool inversion = false)
{
    using std::sin;
    using std::cos;
    if (is_poweroftwo(size))
    {
        return reference_fft<Tnumber>(out, in, size, inversion);
    }
    constexpr Tnumber pi2 = c_pi<Tnumber, 2>;
    if (size < 2)
        return;
    std::vector<complex<T>> datain;
    if (out == in)
    {
        datain.resize(size);
        std::copy_n(in, size, datain.begin());
        in = datain.data();
    }
    {
        Tnumber sumr = 0;
        Tnumber sumi = 0;
        for (size_t j = 0; j < size; j++)
        {
            sumr += static_cast<Tnumber>(in[j].real());
            sumi += static_cast<Tnumber>(in[j].imag());
        }
        out[0] = { static_cast<T>(sumr), static_cast<T>(sumi) };
    }
    for (size_t i = 1; i < size; i++)
    {
        Tnumber sumr = static_cast<Tnumber>(in[0].real());
        Tnumber sumi = static_cast<Tnumber>(in[0].imag());

        for (size_t j = 1; j < size; j++)
        {
            const Tnumber x = pi2 * ((i * j) % size) / size;
            Tnumber twr     = cos(x);
            Tnumber twi     = sin(x);
            if (inversion)
                twi = -twi;

            sumr += twr * static_cast<Tnumber>(in[j].real()) + twi * static_cast<Tnumber>(in[j].imag());
            sumi += twr * static_cast<Tnumber>(in[j].imag()) - twi * static_cast<Tnumber>(in[j].real());
            out[i] = { static_cast<T>(sumr), static_cast<T>(sumi) };
        }
    }
}
}
