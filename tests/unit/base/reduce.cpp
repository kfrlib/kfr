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
    testo::eplison_scope<void> e(100);
    {
        univector<float, 6> a({ 1, 2, 3, 4, 5, -9 });
        CHECK(sum(a) == 6);
        CHECK(mean(a) == 1);
        CHECK(minof(a) == -9);
        CHECK(maxof(a) == 5);
        CHECK(absminof(a) == 1);
        CHECK(absmaxof(a) == 9);
        CHECK(sumsqr(a) == 136);
        CHECK(rms(a) == 4.760952285695233f);
        CHECK(product(a) == -1080);
    }
    {
        univector<double, 6> a({ 1, 2, 3, 4, 5, -9 });
        CHECK(sum(a) == 6);
        CHECK(mean(a) == 1);
        CHECK(minof(a) == -9);
        CHECK(maxof(a) == 5);
        CHECK(absminof(a) == 1);
        CHECK(absmaxof(a) == 9);
        CHECK(sumsqr(a) == 136);
        CHECK(rms(a) == 4.760952285695233);
        CHECK(product(a) == -1080);
    }
    {
        univector<int, 6> a({ 1, 2, 3, 4, 5, -9 });
        CHECK(sum(a) == 6);
        CHECK(mean(a) == 1);
        CHECK(minof(a) == -9);
        CHECK(maxof(a) == 5);
        CHECK(absminof(a) == 1);
        CHECK(absmaxof(a) == 9);
        CHECK(sumsqr(a) == 136);
        CHECK(product(a) == -1080);
    }
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
