/** @addtogroup dsp
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

#include "../base/basic_expressions.hpp"
#include "../base/operators.hpp"
#include "../base/vec.hpp"

namespace kfr
{
/**
 * @brief Returns expression template that generates a unit impulse
 */
template <typename T = int>
static auto unitimpulse()
{
    return lambda<T>([](cinput_t, size_t index, auto x) {
        if (index == 0)
            return onoff(x);
        else
            return zerovector(x);
    });
}

template <typename T = fbase>
static auto jaehne_arg(size_t size)
{
    return truncate(constants<T>::pi_s(1, 2) * sqr(linspace(T(0), T(size), size, false)) / size, size);
}

/**
 * @brief Returns expression template that generates a jaehne vector
 * Generates the sine with linearly increasing frequency from 0hz to nyquist frequency.
 */
template <typename T = fbase>
static auto jaehne(identity<T> magn, size_t size)
{
    return magn * sin(jaehne_arg<T>(size));
}

template <typename T = fbase>
static auto swept_arg(size_t size)
{
    return truncate(constants<T>::pi_s(1, 4) * sqr(sqr(linspace(T(0), T(size), size, false)) / sqr(T(size))) *
                        T(size),
                    size);
}

/**
 * @brief Returns expression template that generates a jaehne vector
 * Generates the sine with logarithmically increasing frequency from 0hz to nyquist frequency.
 */
template <typename T = fbase>
static auto swept(identity<T> magn, size_t size)
{
    return magn * sin(swept_arg<T>(size));
}
}
