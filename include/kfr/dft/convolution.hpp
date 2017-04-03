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
#include "../base/vec.hpp"

#include "cache.hpp"
#include "fft.hpp"

CMT_PRAGMA_GNU(GCC diagnostic push)
#if CMT_HAS_WARNING("-Wshadow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
#endif

namespace kfr
{

template <typename T, size_t Tag1, size_t Tag2>
CMT_FUNC univector<T> convolve(const univector<T, Tag1>& src1, const univector<T, Tag2>& src2)
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

template <typename T, size_t Tag1, size_t Tag2>
CMT_FUNC univector<T> correlate(const univector<T, Tag1>& src1, const univector<T, Tag2>& src2)
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

template <typename T, size_t Tag1>
CMT_FUNC univector<T> autocorrelate(const univector<T, Tag1>& src)
{
    univector<T> result = correlate(src, src);
    result              = result.slice(result.size() / 2);
    return result;
}

template <typename T>
class convolve_filter : public filter<T>
{
public:
    explicit convolve_filter(size_t size, size_t block_size = 1024)
        : fft(2 * next_poweroftwo(block_size)), size(size), block_size(block_size), temp(fft.temp_size),
          segments((size + block_size - 1) / block_size)
    {
    }
    explicit convolve_filter(const univector<T>& data, size_t block_size = 1024)
        : fft(2 * next_poweroftwo(block_size)), size(data.size()), block_size(next_poweroftwo(block_size)),
          temp(fft.temp_size),
          segments((data.size() + next_poweroftwo(block_size) - 1) / next_poweroftwo(block_size)),
          ir_segments((data.size() + next_poweroftwo(block_size) - 1) / next_poweroftwo(block_size)),
          input_position(0), position(0)
    {
        set_data(data);
    }
    void set_data(const univector<T>& data)
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

protected:
    void process_expression(T* dest, const expression_pointer<T>& src, size_t size) final
    {
        univector<T> input = truncate(src, size);
        process_buffer(dest, input.data(), input.size());
    }
    void process_buffer(T* output, const T* input, size_t size) final
    {
        size_t processed = 0;
        while (processed < size)
        {
            const size_t processing = std::min(size - processed, block_size - input_position);
            internal::builtin_memcpy(saved_input.data() + input_position, input + processed,
                                     processing * sizeof(T));

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
            fft_multiply_accumulate(cscratch, premul, ir_segments[0], segments[position],
                                    dft_pack_format::Perm);

            fft.execute(scratch, cscratch, temp, dft_pack_format::Perm);

            process(make_univector(output + processed, processing),
                    scratch.slice(input_position) + overlap.slice(input_position));

            input_position += processing;
            if (input_position == block_size)
            {
                input_position = 0;
                process(saved_input, zeros());

                internal::builtin_memcpy(overlap.data(), scratch.data() + block_size, block_size * sizeof(T));

                position = position > 0 ? position - 1 : segments.size() - 1;
            }

            processed += processing;
        }
    }

    const dft_plan_real<T> fft;
    univector<u8> temp;
    std::vector<univector<complex<T>>> segments;
    std::vector<univector<complex<T>>> ir_segments;
    const size_t size;
    const size_t block_size;
    size_t input_position;
    univector<T> saved_input;
    univector<complex<T>> premul;
    univector<complex<T>> cscratch;
    univector<T> scratch;
    univector<T> overlap;
    size_t position;
};
}
CMT_PRAGMA_GNU(GCC diagnostic pop)
