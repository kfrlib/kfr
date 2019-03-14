/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/base/reduce.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(reduce)
{
    {
        univector<float, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == 15);
        CHECK(mean(a) == 3);
        CHECK(minof(a) == 1);
        CHECK(maxof(a) == 5);
        CHECK(sumsqr(a) == 55);
        CHECK(rms(a) == 3.316624790355399849115f);
        CHECK(product(a) == 120);
    }
    {
        univector<double, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == 15);
        CHECK(mean(a) == 3);
        CHECK(minof(a) == 1);
        CHECK(maxof(a) == 5);
        CHECK(sumsqr(a) == 55);
        CHECK(rms(a) == 3.316624790355399849115);
        CHECK(product(a) == 120);
    }
    {
        univector<int, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == 15);
        CHECK(mean(a) == 3);
        CHECK(minof(a) == 1);
        CHECK(maxof(a) == 5);
        CHECK(sumsqr(a) == 55);
        CHECK(product(a) == 120);
    }
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
