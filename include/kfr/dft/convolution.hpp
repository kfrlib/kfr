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

namespace internal
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
} // namespace internal

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
    static constexpr auto real_fft = !std::is_same<T, complex<ST>>::value;
    using plan_t                   = internal::dft_conv_plan<T>;

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
} // namespace CMT_ARCH_NAME

CMT_MULTI_PROTO(template <typename T>
                filter<T>* make_convolve_filter(const univector_ref<const T>& taps, size_t block_size);)

#ifdef CMT_MULTI
template <typename T>
KFR_FUNCTION filter<T>* make_convolve_filter(cpu_t cpu, const univector_ref<const T>& taps, size_t block_size)
{
    CMT_MULTI_PROTO_GATE(make_convolve_filter<T>(taps, block_size))
}
#endif
} // namespace kfr
CMT_PRAGMA_GNU(GCC diagnostic pop)
