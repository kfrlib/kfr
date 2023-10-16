/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/graphics/color.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(color)
{
    testo::epsilon_scope<void> e(100);

    f32color c1 = f32color::from_argb(0xFF'FF00FF);
    CHECK(c1.r == 1);
    CHECK(c1.g == 0);
    CHECK(c1.b == 1);
    CHECK(c1.a == 1);

    u8color c2 = f32color::from_argb(0xFF'FF00FF);
    CHECK(c2.r == 0xFF);
    CHECK(c2.g == 0x00);
    CHECK(c2.b == 0xFF);
    CHECK(c2.a == 0xFF);

    u8color c3 = f32color(c1.v, 0.5f);
    CHECK(c3.r == 255);
    CHECK(c3.g == 0);
    CHECK(c3.b == 255);
    CHECK(c3.a == 127);

    f32color c4 = from_srgb(f32color(0.75f, 0.25f, 0.5f, 1.f));
#ifndef KFR_SRGB_APPROX
    CHECK(c4.r == 0.522521f);
    CHECK(c4.g == 0.0508761f);
    CHECK(c4.b == 0.214041f);
#else
    CHECK(c4.r == 0.521914f);
    CHECK(c4.g == 0.0505368f);
    CHECK(c4.b == 0.214967f);
#endif
    CHECK(c4.a == 1.f);

    f32color c5 = to_srgb(f32color(0.75f, 0.25f, 0.5f, 1.f));
#ifndef KFR_SRGB_APPROX
    CHECK(c5.r == 0.880825f);
    CHECK(c5.g == 0.537099f);
    CHECK(c5.b == 0.735357f);
#else
    CHECK(c5.r == 0.88027f);
    CHECK(c5.g == 0.536654f);
    CHECK(c5.b == 0.734586f);
#endif
    CHECK(c5.a == 1.f);

    f32color c6 = u8color(0, 127, 255);
    CHECK(c6.r == 0.f);
#ifndef KFR_SRGB_APPROX
    CHECK(c6.g == 0.212231f);
#else
    CHECK(c6.g == 0.213161f);
#endif
    CHECK(c6.b == 1.f);
    
    f32color c7 = u16color(0, 32767, 65535);
    CHECK(c7.r == 0.f);
    CHECK(c7.g == 0.499992370489f);
    CHECK(c7.b == 1.f);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
