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

#include "fft.hpp"
#include <memory>
#include <mutex>
#include <vector>

namespace kfr
{

template <typename T>
using dft_plan_ptr = std::shared_ptr<const dft_plan<T>>;

struct dft_cache
{
    static dft_cache& instance()
    {
        static dft_cache cache;
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
    void clear()
    {
#ifndef KFR_SINGLE_THREAD
        std::lock_guard<std::mutex> guard(mutex);
#endif
        cache_f32.clear();
        cache_f64.clear();
    }

private:
    template <typename T>
    std::shared_ptr<const dft_plan<T>> get_or_create(std::vector<dft_plan_ptr<T>>& cache, size_t size)
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

    std::vector<dft_plan_ptr<f32>> cache_f32;
    std::vector<dft_plan_ptr<f64>> cache_f64;
#ifndef KFR_SINGLE_THREAD
    std::mutex mutex;
#endif
};

template <typename T, size_t Tag>
univector<complex<T>> dft(const univector<complex<T>, Tag>& input)
{
    dft_plan_ptr<T> dft = dft_cache::instance().get(ctype<T>, input.size());
    univector<T> output(input.size());
    univector<u8> temp(dft->temp_size);
    dft->execute(output, input, temp);
    return output;
}

template <typename T, size_t Tag>
univector<complex<T>> idft(const univector<complex<T>, Tag>& input)
{
    dft_plan_ptr<T> dft = dft_cache::instance().get(ctype<T>, input.size());
    univector<T> output(input.size());
    univector<u8> temp(dft->temp_size);
    dft->execute(output, input, temp, ctrue);
    return output;
}
}
