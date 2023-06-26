/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/conversion.hpp>

#include <kfr/base/basic_expressions.hpp>
#include <kfr/base/simd_expressions.hpp>

#include <kfr/base/reduce.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(sample_conversion)
{
    CHECK(convert_sample<float>(static_cast<i8>(-127)) == -1.f);
    CHECK(convert_sample<float>(static_cast<i8>(0)) == 0.f);
    CHECK(convert_sample<float>(static_cast<i8>(127)) == 1.f);

    CHECK(convert_sample<float>(static_cast<i16>(-32767)) == -1.f);
    CHECK(convert_sample<float>(static_cast<i16>(0)) == 0.f);
    CHECK(convert_sample<float>(static_cast<i16>(32767)) == 1.f);

    CHECK(convert_sample<float>(static_cast<i24>(-8388607)) == -1.f);
    CHECK(convert_sample<float>(static_cast<i24>(0)) == 0.f);
    CHECK(convert_sample<float>(static_cast<i24>(8388607)) == 1.f);

    CHECK(convert_sample<float>(static_cast<i32>(-2147483647)) == -1.f);
    CHECK(convert_sample<float>(static_cast<i32>(0)) == 0.f);
    CHECK(convert_sample<float>(static_cast<i32>(2147483647)) == 1.f);

    CHECK(convert_sample<i8>(-1.f) == -127);
    CHECK(convert_sample<i8>(0.f) == 0);
    CHECK(convert_sample<i8>(1.f) == 127);

    CHECK(convert_sample<i16>(-1.f) == -32767);
    CHECK(convert_sample<i16>(0.f) == 0);
    CHECK(convert_sample<i16>(1.f) == 32767);

    CHECK(convert_sample<i24>(-1.f) == -8388607);
    CHECK(convert_sample<i24>(0.f) == 0);
    CHECK(convert_sample<i24>(1.f) == 8388607);

    CHECK(convert_sample<i32>(-1.f) == -2147483647);
    CHECK(convert_sample<i32>(0.f) == 0);
    CHECK(convert_sample<i32>(1.f) == 2147483647);
}

TEST(sample_interleave_deinterleave)
{
    const size_t size = 50;
    univector2d<float> in;
    in.push_back(truncate(counter() * 3.f + 0.f, size));
    in.push_back(truncate(counter() * 3.f + 1.f, size));
    in.push_back(truncate(counter() * 3.f + 2.f, size));
    univector<float> out(size * 3);
    interleave(out.data(), std::array<const float*, 3>{ in[0].data(), in[1].data(), in[2].data() }.data(), 3,
               size);
    CHECK(maxof(out - render(counter() * 1.f, out.size())) == 0);

    deinterleave(std::array<float*, 3>{ in[0].data(), in[1].data(), in[2].data() }.data(), out.data(), 3,
                 size);

    CHECK(absmaxof(in[0] - render(counter() * 3.f + 0.f, size)) == 0);
    CHECK(absmaxof(in[1] - render(counter() * 3.f + 1.f, size)) == 0);
    CHECK(absmaxof(in[2] - render(counter() * 3.f + 2.f, size)) == 0);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
