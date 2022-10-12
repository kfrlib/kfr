/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/math/abs.hpp>

#include <kfr/io.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(abs)
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::abs(x); },
        [](auto x) -> decltype(x) { return x >= 0 ? x : -x; });
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
