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
#include "../convolution.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

template <typename T>
univector<T> convolve(const univector_ref<const T>& src1, const univector_ref<const T>& src2)
{
    const size_t size                = next_poweroftwo(src1.size() + src2.size() - 1);
    univector<complex<T>> src1padded = src1;
    univector<complex<T>> src2padded = src2;
    src1padded.resize(size, 0);
    src2padded.resize(size, 0);

    dft_plan_ptr<T> dft = dft_cache::instance().get(ctype_t<T>(), size);
    univector<u8> temp(dft->temp_size);
    dft->execute(src1padded, src1padded, temp);
    dft->execute(src2padded, src2padded, temp);
    src1padded = src1padded * src2padded;
    dft->execute(src1padded, src1padded, temp, true);
    const T invsize = reciprocal<T>(size);
    return truncate(real(src1padded), src1.size() + src2.size() - 1) * invsize;
}

template <typename T>
univector<T> correlate(const univector_ref<const T>& src1, const univector_ref<const T>& src2)
{
    const size_t size                = next_poweroftwo(src1.size() + src2.size() - 1);
    univector<complex<T>> src1padded = src1;
    univector<complex<T>> src2padded = reverse(src2);
    src1padded.resize(size, 0);
    src2padded.resize(size, 0);
    dft_plan_ptr<T> dft = dft_cache::instance().get(ctype_t<T>(), size);
    univector<u8> temp(dft->temp_size);
    dft->execute(src1padded, src1padded, temp);
    dft->execute(src2padded, src2padded, temp);
    src1padded = src1padded * src2padded;
    dft->execute(src1padded, src1padded, temp, true);
    const T invsize = reciprocal<T>(size);
    return truncate(real(src1padded), src1.size() + src2.size() - 1) * invsize;
}

template <typename T>
univector<T> autocorrelate(const univector_ref<const T>& src1)
{
    univector<T> result = correlate(src1, src1);
    result              = result.slice(result.size() / 2);
    return result;
}

} // namespace intrinsics

template <typename T>
convolve_filter<T>::convolve_filter(size_t size, size_t block_size)
    : size(size), block_size(block_size), fft(2 * next_poweroftwo(block_size)), temp(fft.temp_size),
      segments((size + block_size - 1) / block_size)

{
}

template <typename T>
convolve_filter<T>::convolve_filter(const univector<T>& data, size_t block_size)
    : size(data.size()), block_size(next_poweroftwo(block_size)), fft(2 * next_poweroftwo(block_size)),
      temp(fft.temp_size),
      segments((data.size() + next_poweroftwo(block_size) - 1) / next_poweroftwo(block_size)),
      ir_segments((data.size() + next_poweroftwo(block_size) - 1) / next_poweroftwo(block_size)),
      input_position(0), position(0)
{
    set_data(data);
}

template <typename T>
void convolve_filter<T>::set_data(const univector<T>& data)
{
    univector<T> input(fft.size);
    const T ifftsize = reciprocal(T(fft.size));
    for (size_t i = 0; i < ir_segments.size(); i++)
    {
        segments[i].resize(block_size);
        ir_segments[i].resize(block_size, 0);
        input = padded(data.slice(i * block_size, block_size));

        fft.execute(ir_segments[i], input, temp, dft_pack_format::Perm);
        process(ir_segments[i], ir_segments[i] * ifftsize);
    }
    saved_input.resize(block_size, 0);
    scratch.resize(block_size * 2);
    premul.resize(block_size, 0);
    cscratch.resize(block_size);
    overlap.resize(block_size, 0);
}

template <typename T>
void convolve_filter<T>::process_buffer(T* output, const T* input, size_t size)
{
    size_t processed = 0;
    while (processed < size)
    {
        const size_t processing = std::min(size - processed, block_size - input_position);
        builtin_memcpy(saved_input.data() + input_position, input + processed, processing * sizeof(T));

        process(scratch, padded(saved_input));
        fft.execute(segments[position], scratch, temp, dft_pack_format::Perm);

        if (input_position == 0)
        {
            process(premul, zeros());
            for (size_t i = 1; i < segments.size(); i++)
            {
                const size_t n = (position + i) % segments.size();
                fft_multiply_accumulate(premul, ir_segments[i], segments[n], dft_pack_format::Perm);
            }
        }
        fft_multiply_accumulate(cscratch, premul, ir_segments[0], segments[position], dft_pack_format::Perm);

        fft.execute(scratch, cscratch, temp, dft_pack_format::Perm);

        process(make_univector(output + processed, processing),
                scratch.slice(input_position) + overlap.slice(input_position));

        input_position += processing;
        if (input_position == block_size)
        {
            input_position = 0;
            process(saved_input, zeros());

            builtin_memcpy(overlap.data(), scratch.data() + block_size, block_size * sizeof(T));

            position = position > 0 ? position - 1 : segments.size() - 1;
        }

        processed += processing;
    }
}

namespace intrinsics
{

template univector<float> convolve<float>(const univector_ref<const float>&,
                                          const univector_ref<const float>&);
template univector<float> correlate<float>(const univector_ref<const float>&,
                                           const univector_ref<const float>&);

template univector<float> autocorrelate<float>(const univector_ref<const float>&);

} // namespace intrinsics

template convolve_filter<float>::convolve_filter(size_t, size_t);

template convolve_filter<float>::convolve_filter(const univector<float>&, size_t);

template void convolve_filter<float>::set_data(const univector<float>&);

template void convolve_filter<float>::process_buffer(float* output, const float* input, size_t size);

namespace intrinsics
{

template univector<double> convolve<double>(const univector_ref<const double>&,
                                            const univector_ref<const double>&);
template univector<double> correlate<double>(const univector_ref<const double>&,
                                             const univector_ref<const double>&);

template univector<double> autocorrelate<double>(const univector_ref<const double>&);

} // namespace intrinsics

template convolve_filter<double>::convolve_filter(size_t, size_t);

template convolve_filter<double>::convolve_filter(const univector<double>&, size_t);

template void convolve_filter<double>::set_data(const univector<double>&);

template void convolve_filter<double>::process_buffer(double* output, const double* input, size_t size);
} // namespace CMT_ARCH_NAME
} // namespace kfr
