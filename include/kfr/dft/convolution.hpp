/** @addtogroup convolution
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

#include "../base/filter.hpp"
#include "../base/memory.hpp"
#include "../simd/complex.hpp"
#include "../simd/constants.hpp"
#include "../simd/read_write.hpp"
#include "../simd/vec.hpp"

#include "cache.hpp"
#include "fft.hpp"

CMT_PRAGMA_GNU(GCC diagnostic push)
#if CMT_HAS_WARNING("-Wshadow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
#endif

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{
template <typename T>
univector<T> convolve(const univector_ref<const T>& src1, const univector_ref<const T>& src2);
template <typename T>
univector<T> correlate(const univector_ref<const T>& src1, const univector_ref<const T>& src2);
template <typename T>
univector<T> autocorrelate(const univector_ref<const T>& src1);
} // namespace intrinsics

/// @brief Convolution
template <typename T, univector_tag Tag1, univector_tag Tag2>
univector<T> convolve(const univector<T, Tag1>& src1, const univector<T, Tag2>& src2)
{
    return intrinsics::convolve(src1.slice(), src2.slice());
}

/// @brief Correlation
template <typename T, univector_tag Tag1, univector_tag Tag2>
univector<T> correlate(const univector<T, Tag1>& src1, const univector<T, Tag2>& src2)
{
    return intrinsics::correlate(src1.slice(), src2.slice());
}

/// @brief Auto-correlation
template <typename T, univector_tag Tag1>
univector<T> autocorrelate(const univector<T, Tag1>& src)
{
    return intrinsics::autocorrelate(src.slice());
}

/// @brief Convolution using Filter API
template <typename T>
class convolve_filter : public filter<T>
{
public:
    explicit convolve_filter(size_t size, size_t block_size = 1024);
    explicit convolve_filter(const univector<T>& data, size_t block_size = 1024);
    void set_data(const univector<T>& data);

protected:
    void process_expression(T* dest, const expression_pointer<T>& src, size_t size) final
    {
        univector<T> input = truncate(src, size);
        process_buffer(dest, input.data(), input.size());
    }
    void process_buffer(T* output, const T* input, size_t size) final;

    const size_t size;
    const size_t block_size;
    const dft_plan_real<T> fft;
    univector<u8> temp;
    std::vector<univector<complex<T>>> segments;
    std::vector<univector<complex<T>>> ir_segments;
    size_t input_position;
    univector<T> saved_input;
    univector<complex<T>> premul;
    univector<complex<T>> cscratch;
    univector<T> scratch;
    univector<T> overlap;
    size_t position;
};
} // namespace CMT_ARCH_NAME
} // namespace kfr
CMT_PRAGMA_GNU(GCC diagnostic pop)
