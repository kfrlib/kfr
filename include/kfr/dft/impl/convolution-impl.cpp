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
#include "../../base/simd_expressions.hpp"
#include "../../simd/complex.hpp"
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
    using ST                          = subtype<T>;
    const size_t size                 = next_poweroftwo(src1.size() + src2.size() - 1);
    univector<complex<ST>> src1padded = src1;
    univector<complex<ST>> src2padded = src2;
    src1padded.resize(size);
    src2padded.resize(size);

    dft_plan_ptr<ST> dft = dft_cache::instance().get(ctype_t<ST>(), size);
    univector<u8> temp(dft->temp_size);
    dft->execute(src1padded, src1padded, temp);
    dft->execute(src2padded, src2padded, temp);
    src1padded = src1padded * src2padded;
    dft->execute(src1padded, src1padded, temp, true);
    const ST invsize = reciprocal<ST>(static_cast<ST>(size));
    return truncate(real(src1padded), src1.size() + src2.size() - 1) * invsize;
}

template <typename T>
univector<T> correlate(const univector_ref<const T>& src1, const univector_ref<const T>& src2)
{
    using ST                          = subtype<T>;
    const size_t size                 = next_poweroftwo(src1.size() + src2.size() - 1);
    univector<complex<ST>> src1padded = src1;
    univector<complex<ST>> src2padded = reverse(src2);
    src1padded.resize(size);
    src2padded.resize(size);
    dft_plan_ptr<ST> dft = dft_cache::instance().get(ctype_t<ST>(), size);
    univector<u8> temp(dft->temp_size);
    dft->execute(src1padded, src1padded, temp);
    dft->execute(src2padded, src2padded, temp);
    src1padded = src1padded * src2padded;
    dft->execute(src1padded, src1padded, temp, true);
    const ST invsize = reciprocal<ST>(static_cast<ST>(size));
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
convolve_filter<T>::convolve_filter(size_t size_, size_t block_size_)
    : data_size(size_), block_size(next_poweroftwo(block_size_)), fft(2 * block_size), temp(fft.temp_size),
      segments((data_size + block_size - 1) / block_size), position(0), ir_segments(segments.size()),
      saved_input(block_size), input_position(0), premul(fft.csize()), cscratch(fft.csize()),
      scratch1(fft.size), scratch2(fft.size), overlap(block_size)
{
}

template <typename T>
convolve_filter<T>::convolve_filter(const univector_ref<const T>& data, size_t block_size_)
    : convolve_filter(data.size(), block_size_)
{
    set_data(data);
}

template <typename T>
void convolve_filter<T>::set_data(const univector_ref<const T>& data)
{
    data_size = data.size();
    segments.resize((data_size + block_size - 1) / block_size);
    ir_segments.resize(segments.size());
    univector<T> input(fft.size);
    const ST ifftsize = reciprocal(static_cast<ST>(fft.size));
    for (size_t i = 0; i < ir_segments.size(); i++)
    {
        segments[i].resize(fft.csize());
        ir_segments[i].resize(fft.csize());
        input = padded(data.slice(i * block_size, block_size));

        fft.execute(ir_segments[i], input, temp);
        process(ir_segments[i], ir_segments[i] * ifftsize);
    }
    reset();
}

template <typename T>
void convolve_filter<T>::process_buffer(T* output, const T* input, size_t size)
{
    // Note that the conditionals in the following algorithm are meant to
    // reduce complexity in the common cases of either processing complete
    // blocks (processing == block_size) or only one segment.

    // For complex filtering, use CCs pack format to omit special processing in fft_multiply[_accumulate].
    static constexpr auto fft_multiply_pack = real_fft ? dft_pack_format::Perm : dft_pack_format::CCs;

    size_t processed = 0;
    while (processed < size)
    {
        // Calculate how many samples to process this iteration.
        auto const processing = std::min(size - processed, block_size - input_position);

        // Prepare input to forward FFT:
        if (processing == block_size)
        {
            // No need to work with saved_input.
            builtin_memcpy(scratch1.data(), input + processed, processing * sizeof(T));
        }
        else
        {
            // Append this iteration's input to the saved_input current block.
            builtin_memcpy(saved_input.data() + input_position, input + processed, processing * sizeof(T));
            builtin_memcpy(scratch1.data(), saved_input.data(), block_size * sizeof(T));
        }

        // Forward FFT saved_input block.
        fft.execute(segments[position], scratch1, temp);

        if (segments.size() == 1)
        {
            // Just one segment/block of history.
            // Y_k = H * X_k
            fft_multiply(cscratch, ir_segments[0], segments[0], fft_multiply_pack);
        }
        else
        {
            // More than one segment/block of history so this is more involved.
            if (input_position == 0)
            {
                // At the start of an input block, we premultiply the history from
                // previous input blocks with the extended filter blocks.

                // Y_(k-i,i) = H_i * X_(k-i)
                // premul += Y_(k-i,i) for i=1,...,N

                fft_multiply(premul, ir_segments[1], segments[(position + 1) % segments.size()],
                             fft_multiply_pack);
                for (size_t i = 2; i < segments.size(); i++)
                {
                    const size_t n = (position + i) % segments.size();
                    fft_multiply_accumulate(premul, ir_segments[i], segments[n], fft_multiply_pack);
                }
            }
            // Y_(k,0) = H_0 * X_k
            // Y_k = premul + Y_(k,0)
            fft_multiply_accumulate(cscratch, premul, ir_segments[0], segments[position], fft_multiply_pack);
        }
        // y_k = IFFT( Y_k )
        fft.execute(scratch2, cscratch, temp, cinvert_t{});

        // z_k = y_k + overlap
        process(make_univector(output + processed, processing),
                scratch2.slice(input_position, processing) + overlap.slice(input_position, processing));

        input_position += processing;
        processed += processing;

        // If a whole block was processed, prepare for next block.
        if (input_position == block_size)
        {
            // Input block k is complete. Move to (k+1)-th input block.
            input_position = 0;

            // Zero out the saved_input if it will be used in the next iteration.
            auto const remaining = size - processed;
            if (remaining < block_size && remaining > 0)
            {
                process(saved_input, zeros());
            }

            builtin_memcpy(overlap.data(), scratch2.data() + block_size, block_size * sizeof(T));

            position = position > 0 ? position - 1 : segments.size() - 1;
        }
    }
}

template <typename T>
void convolve_filter<T>::reset()
{
    for (auto& segment : segments)
    {
        process(segment, zeros());
    }
    position = 0;
    process(saved_input, zeros());
    input_position = 0;
    process(overlap, zeros());
}

namespace intrinsics
{

template univector<float> convolve<float>(const univector_ref<const float>&,
                                          const univector_ref<const float>&);
template univector<complex<float>> convolve<complex<float>>(const univector_ref<const complex<float>>&,
                                                            const univector_ref<const complex<float>>&);
template univector<float> correlate<float>(const univector_ref<const float>&,
                                           const univector_ref<const float>&);
template univector<complex<float>> correlate<complex<float>>(const univector_ref<const complex<float>>&,
                                                             const univector_ref<const complex<float>>&);

template univector<float> autocorrelate<float>(const univector_ref<const float>&);
template univector<complex<float>> autocorrelate<complex<float>>(const univector_ref<const complex<float>>&);

} // namespace intrinsics

template convolve_filter<float>::convolve_filter(size_t, size_t);
template convolve_filter<complex<float>>::convolve_filter(size_t, size_t);

template convolve_filter<float>::convolve_filter(const univector_ref<const float>&, size_t);
template convolve_filter<complex<float>>::convolve_filter(const univector_ref<const complex<float>>&, size_t);

template void convolve_filter<float>::set_data(const univector_ref<const float>&);
template void convolve_filter<complex<float>>::set_data(const univector_ref<const complex<float>>&);

template void convolve_filter<float>::process_buffer(float* output, const float* input, size_t size);
template void convolve_filter<complex<float>>::process_buffer(complex<float>* output,
                                                              const complex<float>* input, size_t size);

template void convolve_filter<float>::reset();
template void convolve_filter<complex<float>>::reset();

namespace intrinsics
{

template univector<double> convolve<double>(const univector_ref<const double>&,
                                            const univector_ref<const double>&);
template univector<complex<double>> convolve<complex<double>>(const univector_ref<const complex<double>>&,
                                                              const univector_ref<const complex<double>>&);
template univector<double> correlate<double>(const univector_ref<const double>&,
                                             const univector_ref<const double>&);
template univector<complex<double>> correlate<complex<double>>(const univector_ref<const complex<double>>&,
                                                               const univector_ref<const complex<double>>&);

template univector<double> autocorrelate<double>(const univector_ref<const double>&);
template univector<complex<double>> autocorrelate<complex<double>>(
    const univector_ref<const complex<double>>&);

} // namespace intrinsics

template convolve_filter<double>::convolve_filter(size_t, size_t);
template convolve_filter<complex<double>>::convolve_filter(size_t, size_t);

template convolve_filter<double>::convolve_filter(const univector_ref<const double>&, size_t);
template convolve_filter<complex<double>>::convolve_filter(const univector_ref<const complex<double>>&,
                                                           size_t);

template void convolve_filter<double>::set_data(const univector_ref<const double>&);
template void convolve_filter<complex<double>>::set_data(const univector_ref<const complex<double>>&);

template void convolve_filter<double>::process_buffer(double* output, const double* input, size_t size);
template void convolve_filter<complex<double>>::process_buffer(complex<double>* output,
                                                               const complex<double>* input, size_t size);

template void convolve_filter<double>::reset();
template void convolve_filter<complex<double>>::reset();

template <typename T>
filter<T>* make_convolve_filter(const univector_ref<const T>& taps, size_t block_size)
{
    return new convolve_filter<T>(taps, block_size);
}

template filter<float>* make_convolve_filter(const univector_ref<const float>&, size_t);
template filter<complex<float>>* make_convolve_filter(const univector_ref<const complex<float>>&, size_t);
template filter<double>* make_convolve_filter(const univector_ref<const double>&, size_t);
template filter<complex<double>>* make_convolve_filter(const univector_ref<const complex<double>>&, size_t);

} // namespace CMT_ARCH_NAME
} // namespace kfr
