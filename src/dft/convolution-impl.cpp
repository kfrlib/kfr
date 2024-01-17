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
#include <kfr/base/simd_expressions.hpp>
#include <kfr/dft/convolution.hpp>
#include <kfr/simd/complex.hpp>
#include <kfr/multiarch.h>

namespace kfr
{

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

//-------------------------------------------------------------------------------------

CMT_MULTI_PROTO(namespace impl {
    template <typename T>
    univector<T> convolve(const univector_ref<const T>&, const univector_ref<const T>&, bool);

    template <typename T>
    class convolve_filter : public kfr::convolve_filter<T>
    {
    public:
        void process_buffer_impl(T* output, const T* input, size_t size);
    };
})

inline namespace CMT_ARCH_NAME
{

namespace impl
{

template <typename T>
univector<T> convolve(const univector_ref<const T>& src1, const univector_ref<const T>& src2, bool correlate)
{
    using ST                          = subtype<T>;
    const size_t size                 = next_poweroftwo(src1.size() + src2.size() - 1);
    univector<complex<ST>> src1padded = src1;
    univector<complex<ST>> src2padded;
    if (correlate)
        src2padded = reverse(src2);
    else
        src2padded = src2;
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
template univector<f32> convolve<f32>(const univector_ref<const f32>&, const univector_ref<const f32>&, bool);
template univector<f64> convolve<f64>(const univector_ref<const f64>&, const univector_ref<const f64>&, bool);
template univector<c32> convolve<c32>(const univector_ref<const c32>&, const univector_ref<const c32>&, bool);
template univector<c64> convolve<c64>(const univector_ref<const c64>&, const univector_ref<const c64>&, bool);

template <typename T>
void convolve_filter<T>::process_buffer_impl(T* output, const T* input, size_t size)
{
    // Note that the conditionals in the following algorithm are meant to
    // reduce complexity in the common cases of either processing complete
    // blocks (processing == block_size) or only one segment.

    // For complex filtering, use CCs pack format to omit special processing in fft_multiply[_accumulate].
    const dft_pack_format fft_multiply_pack = this->real_fft ? dft_pack_format::Perm : dft_pack_format::CCs;

    size_t processed = 0;
    while (processed < size)
    {
        // Calculate how many samples to process this iteration.
        auto const processing = std::min(size - processed, this->block_size - this->input_position);

        // Prepare input to forward FFT:
        if (processing == this->block_size)
        {
            // No need to work with saved_input.
            builtin_memcpy(this->scratch1.data(), input + processed, processing * sizeof(T));
        }
        else
        {
            // Append this iteration's input to the saved_input current block.
            builtin_memcpy(this->saved_input.data() + this->input_position, input + processed,
                           processing * sizeof(T));
            builtin_memcpy(this->scratch1.data(), this->saved_input.data(), this->block_size * sizeof(T));
        }

        // Forward FFT saved_input block.
        this->fft.execute(this->segments[this->position], this->scratch1, this->temp);

        if (this->segments.size() == 1)
        {
            // Just one segment/block of history.
            // Y_k = H * X_k
            fft_multiply(this->cscratch, this->ir_segments[0], this->segments[0], fft_multiply_pack);
        }
        else
        {
            // More than one segment/block of history so this is more involved.
            if (this->input_position == 0)
            {
                // At the start of an input block, we premultiply the history from
                // previous input blocks with the extended filter blocks.

                // Y_(k-i,i) = H_i * X_(k-i)
                // premul += Y_(k-i,i) for i=1,...,N

                fft_multiply(this->premul, this->ir_segments[1],
                             this->segments[(this->position + 1) % this->segments.size()], fft_multiply_pack);
                for (size_t i = 2; i < this->segments.size(); i++)
                {
                    const size_t n = (this->position + i) % this->segments.size();
                    fft_multiply_accumulate(this->premul, this->ir_segments[i], this->segments[n],
                                            fft_multiply_pack);
                }
            }
            // Y_(k,0) = H_0 * X_k
            // Y_k = premul + Y_(k,0)
            fft_multiply_accumulate(this->cscratch, this->premul, this->ir_segments[0],
                                    this->segments[this->position], fft_multiply_pack);
        }
        // y_k = IFFT( Y_k )
        this->fft.execute(this->scratch2, this->cscratch, this->temp, cinvert_t{});

        // z_k = y_k + overlap
        process(make_univector(output + processed, processing),
                this->scratch2.slice(this->input_position, processing) +
                    this->overlap.slice(this->input_position, processing));

        this->input_position += processing;
        processed += processing;

        // If a whole block was processed, prepare for next block.
        if (this->input_position == this->block_size)
        {
            // Input block k is complete. Move to (k+1)-th input block.
            this->input_position = 0;

            // Zero out the saved_input if it will be used in the next iteration.
            auto const remaining = size - processed;
            if (remaining < this->block_size && remaining > 0)
            {
                process(this->saved_input, zeros());
            }

            builtin_memcpy(this->overlap.data(), this->scratch2.data() + this->block_size,
                           this->block_size * sizeof(T));

            this->position = this->position > 0 ? this->position - 1 : this->segments.size() - 1;
        }
    }
}

template class convolve_filter<float>;
template class convolve_filter<double>;
template class convolve_filter<complex<float>>;
template class convolve_filter<complex<double>>;

} // namespace impl

} // namespace CMT_ARCH_NAME

#ifdef CMT_MULTI_NEEDS_GATE
namespace internal_generic
{
template <typename T>
univector<T> convolve(const univector_ref<const T>& src1, const univector_ref<const T>& src2, bool correlate)
{
    CMT_MULTI_GATE(return ns::impl::convolve(src1, src2, correlate));
}

template univector<f32> convolve<f32>(const univector_ref<const f32>&, const univector_ref<const f32>&, bool);
template univector<f64> convolve<f64>(const univector_ref<const f64>&, const univector_ref<const f64>&, bool);
template univector<c32> convolve<c32>(const univector_ref<const c32>&, const univector_ref<const c32>&, bool);
template univector<c64> convolve<c64>(const univector_ref<const c64>&, const univector_ref<const c64>&, bool);

} // namespace internal_generic

template <typename T>
void convolve_filter<T>::process_buffer(T* output, const T* input, size_t size)
{
    CMT_MULTI_GATE(
        reinterpret_cast<ns::impl::convolve_filter<T>*>(this)->process_buffer_impl(output, input, size));
}

template class convolve_filter<float>;
template class convolve_filter<double>;
template class convolve_filter<complex<float>>;
template class convolve_filter<complex<double>>;
#endif

} // namespace kfr
