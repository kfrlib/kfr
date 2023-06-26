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

#include "fft.hpp"
#include <memory>
#include <mutex>
#include <vector>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T>
using dft_plan_ptr = std::shared_ptr<const dft_plan<T>>;

template <typename T>
using dft_plan_real_ptr = std::shared_ptr<const dft_plan_real<T>>;

template <int = 0>
struct dft_cache_impl
{
    static dft_cache_impl& instance()
    {
        static dft_cache_impl cache;
        return cache;
    }
    dft_plan_ptr<f32> get(ctype_t<f32>, size_t size)
    {
#ifndef KFR_SINGLE_THREAD
        std::lock_guard<std::mutex> guard(mutex);
#endif
        return get_or_create(cache_f32, size);
    }
    dft_plan_ptr<f64> get(ctype_t<f64>, size_t size)
    {
#ifndef KFR_SINGLE_THREAD
        std::lock_guard<std::mutex> guard(mutex);
#endif
        return get_or_create(cache_f64, size);
    }
    dft_plan_real_ptr<f32> getreal(ctype_t<f32>, size_t size)
    {
#ifndef KFR_SINGLE_THREAD
        std::lock_guard<std::mutex> guard(mutex);
#endif
        return get_or_create(cache_real_f32, size);
    }
    dft_plan_real_ptr<f64> getreal(ctype_t<f64>, size_t size)
    {
#ifndef KFR_SINGLE_THREAD
        std::lock_guard<std::mutex> guard(mutex);
#endif
        return get_or_create(cache_real_f64, size);
    }
    void clear()
    {
#ifndef KFR_SINGLE_THREAD
        std::lock_guard<std::mutex> guard(mutex);
#endif
        cache_f32.clear();
        cache_f64.clear();
        cache_real_f32.clear();
        cache_real_f64.clear();
    }

private:
    template <typename T>
    dft_plan_ptr<T> get_or_create(std::vector<dft_plan_ptr<T>>& cache, size_t size)
    {
        for (dft_plan_ptr<T>& dft : cache)
        {
            if (dft->size == size)
                return dft;
        }
        dft_plan_ptr<T> sh = std::make_shared<dft_plan<T>>(size);
        cache.push_back(sh);
        return sh;
    }
    template <typename T>
    dft_plan_real_ptr<T> get_or_create(std::vector<dft_plan_real_ptr<T>>& cache, size_t size)
    {
        for (dft_plan_real_ptr<T>& dft : cache)
        {
            if (dft->size == size)
                return dft;
        }
        dft_plan_real_ptr<T> sh = std::make_shared<dft_plan_real<T>>(size);
        cache.push_back(sh);
        return sh;
    }

    std::vector<dft_plan_ptr<f32>> cache_f32;
    std::vector<dft_plan_ptr<f64>> cache_f64;
    std::vector<dft_plan_real_ptr<f32>> cache_real_f32;
    std::vector<dft_plan_real_ptr<f64>> cache_real_f64;
#ifndef KFR_SINGLE_THREAD
    std::mutex mutex;
#endif
};

using dft_cache = dft_cache_impl<>;

/// @brief Performs Direct DFT using cached plan
template <typename T, univector_tag Tag>
univector<complex<T>> dft(const univector<complex<T>, Tag>& input)
{
    dft_plan_ptr<T> dft = dft_cache::instance().get(ctype_t<T>(), input.size());
    univector<complex<T>> output(input.size(), std::numeric_limits<T>::quiet_NaN());
    univector<u8> temp(dft->temp_size);
    dft->execute(output, input, temp);
    return output;
}

/// @brief Performs Inverse DFT using cached plan
template <typename T, univector_tag Tag>
univector<complex<T>> idft(const univector<complex<T>, Tag>& input)
{
    dft_plan_ptr<T> dft = dft_cache::instance().get(ctype_t<T>(), input.size());
    univector<complex<T>> output(input.size(), std::numeric_limits<T>::quiet_NaN());
    univector<u8> temp(dft->temp_size);
    dft->execute(output, input, temp, ctrue);
    return output;
}

/// @brief Performs Real Direct DFT using cached plan
template <typename T, univector_tag Tag>
univector<complex<T>> realdft(const univector<T, Tag>& input)
{
    dft_plan_real_ptr<T> dft = dft_cache::instance().getreal(ctype_t<T>(), input.size());
    univector<complex<T>> output(input.size() / 2 + 1, std::numeric_limits<T>::quiet_NaN());
    univector<u8> temp(dft->temp_size);
    dft->execute(output, input, temp);
    return output;
}

/// @brief Permorms Real Inverse DFT using cached plan
template <typename T, univector_tag Tag>
univector<T> irealdft(const univector<complex<T>, Tag>& input)
{
    dft_plan_real_ptr<T> dft = dft_cache::instance().getreal(ctype_t<T>(), (input.size() - 1) * 2);
    univector<T> output((input.size() - 1) * 2, std::numeric_limits<T>::quiet_NaN());
    univector<u8> temp(dft->temp_size);
    dft->execute(output, input, temp);
    return output;
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
