/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/simd.hpp>

namespace kfr
{

inline namespace KFR_ARCH_NAME
{

TEST_CASE("f16 conversion")
{
    SECTION("Zeros & Signed Zeros")
    {
        CHECK(f16(0.0f).raw == 0x0000);
        CHECK(f16(-0.0f).raw == 0x8000);

        f16 pos_zero = f16::from_raw(0x0000), neg_zero = f16::from_raw(0x8000);
        CHECK(bitcast_anything<u32>(static_cast<f32>(pos_zero)) == 0x00000000);
        CHECK(bitcast_anything<u32>(static_cast<f32>(neg_zero)) == 0x80000000);
    }

    SECTION("Infinities")
    {
        // f32 -> f16 Infinities
        CHECK(f16(bitcast_anything<f32>(u32(0x7F800000))).raw == 0x7C00); // +Inf
        CHECK(f16(bitcast_anything<f32>(u32(0xFF800000))).raw == 0xFC00); // -Inf

        // f16 -> f32 Infinities
        f16 pos_inf = f16::from_raw(0x7C00), neg_inf = f16::from_raw(0xFC00);
        CHECK(bitcast_anything<u32>(static_cast<f32>(pos_inf)) == 0x7F800000);
        CHECK(bitcast_anything<u32>(static_cast<f32>(neg_inf)) == 0xFF800000);
    }

    SECTION("Subnormals & Boundary Transitions")
    {
        // Min subnormal (2^-24)
        f16 min_sub = f16::from_raw(0x0001);
        CHECK(bitcast_anything<u32>(static_cast<f32>(min_sub)) == 0x33800000);

        // Max subnormal (2^-14 - 2^-24)
        f16 max_sub = f16::from_raw(0x03FF);
        CHECK(bitcast_anything<u32>(static_cast<f32>(max_sub)) == 0x387FC000);

        // Min normal (2^-14)
        f16 min_norm = f16::from_raw(0x0400);
        CHECK(bitcast_anything<u32>(static_cast<f32>(min_norm)) == 0x38800000);

        // Negative subnormal
        f16 neg_sub = f16::from_raw(0x8001);
        CHECK(bitcast_anything<u32>(static_cast<f32>(neg_sub)) == 0xB3800000);
    }

    SECTION("Underflow to Zero")
    {
        // Value strictly less than 2^-25 rounds to zero
        CHECK(f16(bitcast_anything<f32>(u32(0x32FFFFFF))).raw == 0x0000); // positive zero
        CHECK(f16(bitcast_anything<f32>(u32(0xB2FFFFFF))).raw == 0x8000); // negative zero
    }

    SECTION("Overflow Boundaries")
    {
        // Max finite f16 (65504.0f)
        CHECK(f16(65504.0f).raw == 0x7BFF);

        // Exact midpoint (65520.0f) rounds to +Infinity (0x7C00) due to round-to-even
        CHECK(f16(65520.0f).raw == 0x7C00);

        // Values > 65520 overflow to Infinity
        CHECK(f16(70000.0f).raw == 0x7C00);
        CHECK(f16(-70000.0f).raw == 0xFC00);
    }

    SECTION("Round-to-Nearest-Even Tie Breaking")
    {
        // Tie between 1.0 (0x3C00) and 1.0009765625 (0x3C01)
        // Midpoint in f32 is 1 + 2^-11 = 1.00048828125 (0x3F801000)

        // Exactly at midpoint -> round to EVEN mantissa (0x3C00)
        CHECK(f16(bitcast_anything<f32>(u32(0x3F801000))).raw == 0x3C00);

        // Slightly above midpoint -> round UP to (0x3C01)
        CHECK(f16(bitcast_anything<f32>(u32(0x3F801001))).raw == 0x3C01);

        // Tie between 1.0009765625 (0x3C01, odd) and 1.001953125 (0x3C02, even)
        // Midpoint is 0x3F803000 -> rounds to EVEN mantissa (0x3C02)
        CHECK(f16(bitcast_anything<f32>(u32(0x3F803000))).raw == 0x3C02);
    }
}

TEST_CASE("grouped runtime stride")
{
    const i32 values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    CHECK_THAT((gather_stride<3, 2>(values, 3)), DeepMatcher((vec<i32, 6>{ 0, 1, 6, 7, 12, 13 })));
    CHECK_THAT((gather_stride<4, 2>(values, 2)), DeepMatcher((vec<i32, 8>{ 0, 1, 4, 5, 8, 9, 12, 13 })));
    CHECK_THAT((gather_stride<3, 3>(values, 2)),
               DeepMatcher((vec<i32, 9>{ 0, 1, 2, 6, 7, 8, 12, 13, 14 })));

    i32 result[16]{};
    scatter_stride<2>(result, vec<i32, 6>{ 20, 21, 30, 31, 40, 41 }, 3);
    CHECK_THAT(read<14>(result),
               DeepMatcher(vec<i32, 14>{ 20, 21, 0, 0, 0, 0, 30, 31, 0, 0, 0, 0, 40, 41 }));

    std::fill(std::begin(result), std::end(result), 0);
    scatter_stride<3>(result, vec<i32, 9>{ 20, 21, 22, 30, 31, 32, 40, 41, 42 }, 2);
    CHECK_THAT(read<15>(result),
               DeepMatcher(vec<i32, 15>{ 20, 21, 22, 0, 0, 0, 30, 31, 32, 0, 0, 0, 40, 41, 42 }));

    std::fill(std::begin(result), std::end(result), 0);
    scatter_stride<2>(result, vec<i32, 8>{ 20, 21, 30, 31, 40, 41, 50, 51 }, 2);
    CHECK_THAT(read<14>(result),
               DeepMatcher(vec<i32, 14>{ 20, 21, 0, 0, 30, 31, 0, 0, 40, 41, 0, 0, 50, 51 }));

    stride_pointer<const i32, 2> reader{ values, 2 };
    CHECK_THAT(reader.read<4>(), DeepMatcher((vec<i32, 4>{ 0, 1, 4, 5 })));

    std::fill(std::begin(result), std::end(result), 0);
    stride_pointer<i32, 2> writer{ result, 2 };
    writer.write(vec<i32, 4>{ 20, 21, 30, 31 });
    CHECK_THAT(read<6>(result), DeepMatcher(vec<i32, 6>{ 20, 21, 0, 0, 30, 31 }));
}

TEST_CASE("partial read/write")
{
    constexpr size_t width = 7;
    const i32 source[]     = { 10, 11, 12, 13, 14, 15, 16, 17 };

    for (size_t count = 1; count <= width; ++count)
    {
        const vec<i32, width> value = partial_read<width>(source + 1, count);
        for (size_t i = 0; i < count; ++i)
            CHECK(value[i] == source[i + 1]);

        i32 destination[width + 2];
        std::fill(std::begin(destination), std::end(destination), -1);
        partial_write(destination + 1, value, count);

        CHECK(destination[0] == -1);
        for (size_t i = 0; i < width; ++i)
            CHECK(destination[i + 1] == (i < count ? source[i + 1] : -1));
        CHECK(destination[width + 1] == -1);
    }

    CHECK_THAT(partial_read<width>(source + 1, width + 1),
               DeepMatcher((vec<i32, width>{ 11, 12, 13, 14, 15, 16, 17 })));

    i32 destination[width];
    std::fill(std::begin(destination), std::end(destination), -1);
    partial_write(destination, vec<i32, width>{ 11, 12, 13, 14, 15, 16, 17 }, width + 1);
    CHECK_THAT(read<width>(destination),
               DeepMatcher((vec<i32, width>{ 11, 12, 13, 14, 15, 16, 17 })));

    alignas(8 * sizeof(i32)) const i32 aligned_source[] = { 20, 21, 22, 23, 24, 25, 26, 27 };
    const vec<i32, 8> aligned_value                    = partial_read<8, true>(aligned_source, 3);
    CHECK(aligned_value[0] == 20);
    CHECK(aligned_value[1] == 21);
    CHECK(aligned_value[2] == 22);

    alignas(8 * sizeof(i32)) i32 aligned_destination[] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    partial_write<true>(aligned_destination, vec<i32, 8>{ 30, 31, 32, 33, 34, 35, 36, 37 }, 3);
    CHECK_THAT((read<8, true>(aligned_destination)),
               DeepMatcher((vec<i32, 8>{ 30, 31, 32, -1, -1, -1, -1, -1 })));
}

TEST_CASE("f16 round-trip test")
{
    // Every single valid non-NaN f16 must round-trip through f32 without losing precision
    for (u32 i = 0; i < 65536; ++i)
    {
        u16 raw = static_cast<u16>(i);

        // Skip NaNs (exponent bits all 1s and non-zero mantissa)
        bool is_nan = ((raw & 0x7C00) == 0x7C00) && ((raw & 0x03FF) != 0);
        if (is_nan)
            continue;

        f16 original;
        original.raw = raw;

        f32 converted = static_cast<f32>(original);
        f16 round_tripped(converted);

        CHECK(round_tripped.raw == original.raw);
    }
}

} // namespace KFR_ARCH_NAME

} // namespace kfr
