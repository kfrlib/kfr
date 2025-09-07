/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include "kfr/base/tensor.hpp"
#include <kfr/base/reduce.hpp>
#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/univector.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

TEST_CASE("reduce")
{
    epsilon_scope<void> e(100);
    {
        univector<float, 6> a({ 1, 2, 3, 4, 5, -9 });
        CHECK_THAT((sum(a)), DeepMatcher(6));
        CHECK_THAT((mean(a)), DeepMatcher(1));
        CHECK_THAT((minof(a)), DeepMatcher(-9));
        CHECK_THAT((maxof(a)), DeepMatcher(5));
        CHECK_THAT((absminof(a)), DeepMatcher(1));
        CHECK_THAT((absmaxof(a)), DeepMatcher(9));
        CHECK_THAT((sumsqr(a)), DeepMatcher(136));
        CHECK_THAT((rms(a)), DeepMatcher(4.760952285695233f));
        CHECK_THAT((product(a)), DeepMatcher(-1080));
    }
    {
        univector<double, 6> a({ 1, 2, 3, 4, 5, -9 });
        CHECK_THAT((sum(a)), DeepMatcher(6));
        CHECK_THAT((mean(a)), DeepMatcher(1));
        CHECK_THAT((minof(a)), DeepMatcher(-9));
        CHECK_THAT((maxof(a)), DeepMatcher(5));
        CHECK_THAT((absminof(a)), DeepMatcher(1));
        CHECK_THAT((absmaxof(a)), DeepMatcher(9));
        CHECK_THAT((sumsqr(a)), DeepMatcher(136));
        CHECK_THAT((rms(a)), DeepMatcher(4.760952285695233));
        CHECK_THAT((product(a)), DeepMatcher(-1080));
    }
    {
        univector<int, 6> a({ 1, 2, 3, 4, 5, -9 });
        CHECK_THAT((sum(a)), DeepMatcher(6));
        CHECK_THAT((mean(a)), DeepMatcher(1));
        CHECK_THAT((minof(a)), DeepMatcher(-9));
        CHECK_THAT((maxof(a)), DeepMatcher(5));
        CHECK_THAT((absminof(a)), DeepMatcher(1));
        CHECK_THAT((absmaxof(a)), DeepMatcher(9));
        CHECK_THAT((sumsqr(a)), DeepMatcher(136));
        CHECK_THAT((product(a)), DeepMatcher(-1080));
    }
}

TEST_CASE("dotproduct")
{
    univector<float, 177> v1 = counter();
    univector<float, 177> v2 = counter() * 2 + 10;
    CHECK_THAT((dotproduct(v1, v2)), DeepMatcher(3821312));
}

TEST_CASE("histogram")
{
    univector<int, 16> v{ 1, 9, 5, 2, 1, -3, 100, 19, -4, -3, 1, 5, 9, 8, 0, 1 };
    auto h = histogram<10>(v);
    CHECK(h.total() == 16);
    CHECK(h.below() == 3);
    CHECK(h.above() == 2);
    CHECK(h[0] == 1);
    CHECK(h[1] == 4);
    CHECK(h[2] == 1);
    CHECK(h[3] == 0);
    CHECK(h[4] == 0);
    CHECK(h[5] == 2);
    CHECK(h[6] == 0);
    CHECK(h[7] == 0);
    CHECK(h[8] == 1);
    CHECK(h[9] == 2);

    univector<double, 16> v2{ 0.1,  0.9,  0.5, 0.2, 0.1, -0.3, 10.0, 1.9,
                              -0.4, -0.3, 0.1, 0.5, 0.9, 0.8,  0.0,  0.1 };
    auto h2 = histogram<10>(v2);
    CHECK(h2.total() == 16);
    CHECK(h2.below() == 3);
    CHECK(h2.above() == 2);
    CHECK(h2[0] == 1);
    CHECK(h2[1] == 4);
    CHECK(h2[2] == 1);
    CHECK(h2[3] == 0);
    CHECK(h2[4] == 0);
    CHECK(h2[5] == 2);
    CHECK(h2[6] == 0);
    CHECK(h2[7] == 0);
    CHECK(h2[8] == 1);
    CHECK(h2[9] == 2);
}

TEST_CASE("reduce_multidim")
{
    CHECK(sum(tensor<int, 2>(shape{ 3, 3 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9 })) == 45); //
    CHECK(sum(tensor<int, 3>(shape{ 2, 2, 2 }, { 1, 2, 3, 4, 5, 6, 7, 8 })) == 36); //

    CHECK(maxof(tensor<int, 3>(shape{ 2, 2, 2 }, { 1, 2, 3, 4, 5, 6, 7, 8 })) == 8); //
    CHECK(minof(tensor<int, 3>(shape{ 2, 2, 2 }, { 1, 2, 3, 4, 5, 6, 7, 8 })) == 1); //
    CHECK(product(tensor<int, 3>(shape{ 2, 2, 2 }, { 1, 2, 3, 4, 5, 6, 7, 8 })) == 40320); //
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
