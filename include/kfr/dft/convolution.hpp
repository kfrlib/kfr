/** @addtogroup convolution
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

namespace internal_generic
{
template <typename T>
univector<T> convolve(const univector_ref<const T>& src1, const univector_ref<const T>& src2,
                      bool correlate = false);
}

/// @brief Convolution
template <typename T1, typename T2, univector_tag Tag1, univector_tag Tag2,
          CMT_ENABLE_IF(std::is_same_v<std::remove_const_t<T1>, std::remove_const_t<T2>>)>
univector<std::remove_const_t<T1>> convolve(const univector<T1, Tag1>& src1, const univector<T2, Tag2>& src2)
{
    return internal_generic::convolve(src1.slice(), src2.slice());
}

/// @brief Correlation
template <typename T1, typename T2, univector_tag Tag1, univector_tag Tag2,
          CMT_ENABLE_IF(std::is_same_v<std::remove_const_t<T1>, std::remove_const_t<T2>>)>
univector<std::remove_const_t<T1>> correlate(const univector<T1, Tag1>& src1, const univector<T2, Tag2>& src2)
{
    return internal_generic::convolve(src1.slice(), src2.slice(), true);
}

/// @brief Auto-correlation
template <typename T, univector_tag Tag1>
univector<std::remove_const_t<T>> autocorrelate(const univector<T, Tag1>& src)
{
    univector<std::remove_const_t<T>> result = internal_generic::convolve(src.slice(), src.slice(), true);
    result                                   = result.slice(result.size() / 2);
    return result;
}

namespace internal_generic
{
/// @brief Utility class to abstract real/complex differences
template <typename T>
struct dft_conv_plan : public dft_plan_real<T>
{
    dft_conv_plan(size_t size) : dft_plan_real<T>(size, dft_pack_format::Perm) {}

    size_t csize() const { return this->size / 2; }
};

template <typename T>
struct dft_conv_plan<complex<T>> : public dft_plan<T>
{
    dft_conv_plan(size_t size) : dft_plan<T>(size) {}

    size_t csize() const { return this->size; }
};
} // namespace internal_generic

/// @brief Convolution using Filter API
template <typename T>
class convolve_filter : public filter<T>
{
public:
    explicit convolve_filter(size_t size, size_t block_size = 1024);
    explicit convolve_filter(const univector_ref<const T>& data, size_t block_size = 1024);
    void set_data(const univector_ref<const T>& data);
    void reset() final;
    /// Apply filter to multiples of returned block size for optimal processing efficiency.
    size_t input_block_size() const { return block_size; }

protected:
    void process_expression(T* dest, const expression_handle<T>& src, size_t size) final
    {
        univector<T> input = truncate(src, size);
        process_buffer(dest, input.data(), input.size());
    }
    void process_buffer(T* output, const T* input, size_t size) final;

    using ST                       = subtype<T>;
    constexpr static bool real_fft = !std::is_same_v<T, complex<ST>>;
    using plan_t                   = internal_generic::dft_conv_plan<T>;

    // Length of filter data.
    size_t data_size;
    // Size of block to process.
    const size_t block_size;
    // FFT plan for circular convolution.
    const plan_t fft;
    // Temp storage for FFT.
    univector<u8> temp;
    // History of input segments after fwd DFT.  History is circular relative to position below.
    std::vector<univector<complex<ST>>> segments;
    // Index into segments of current block.
    size_t position;
    // Blocks of filter/data after fwd DFT.
    std::vector<univector<complex<ST>>> ir_segments;
    // Saved input for current block.
    univector<T> saved_input;
    // Index into saved_input for next input to begin.
    size_t input_position;
    // Pre-multiplied products of input history and delayed filter blocks.
    univector<complex<ST>> premul;
    // Scratch buffer for product of filter and input for processing by reverse DFT.
    univector<complex<ST>> cscratch;
    // Scratch buffers for input and output of fwd and rev DFTs.
    univector<T> scratch1, scratch2;
    // Overlap saved from previous block to add into current block.
    univector<T> overlap;
};

} // namespace kfr
CMT_PRAGMA_GNU(GCC diagnostic pop)
