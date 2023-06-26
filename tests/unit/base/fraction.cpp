/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/fraction.hpp>
#include <kfr/io.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(fraction)
{
    fraction f = 1;
    f          = f / 3;
    CHECK(f == fraction{ 1, 3 });
    CHECK(+f == fraction{ 1, 3 });
    CHECK(-f == fraction{ -1, 3 });
    CHECK(static_cast<double>(f) == 1.0 / 3.0);
    CHECK(static_cast<float>(f) == 1.0f / 3.0f);
    CHECK(static_cast<i64>(f) == 0);

    fraction x{ 1, 5 };
    CHECK(f + x == fraction{ 8, 15 });
    CHECK(f - x == fraction{ 2, 15 });
    CHECK(f * x == fraction{ 1, 15 });
    CHECK(f / x == fraction{ 5, 3 });

    CHECK(f > x);
    CHECK(f >= x);
    CHECK(!(f < x));
    CHECK(!(f <= x));

    f += fraction(1, 3);
    CHECK(f == fraction{ 2, 3 });
    f -= fraction(1, 6);
    CHECK(f == fraction{ 1, 2 });
    f *= fraction(1, 7);
    CHECK(f == fraction{ 1, 14 });
    f /= fraction(1, 2);
    CHECK(f == fraction{ 1, 7 });

    CHECK(fraction{ 100, 200 } == fraction{ 1, 2 });
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
