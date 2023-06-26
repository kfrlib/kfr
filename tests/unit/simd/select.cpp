/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/select.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(select_true)
{
    test_function2(
        test_catogories::vectors,
        [](auto x, auto y)
        {
            mask<subtype<decltype(x)>, decltype(x)::scalar_size()> m(true);
            return kfr::select(m, x, y);
        },
        [](auto x, auto) { return x; });
}

TEST(select_false)
{
    test_function2(
        test_catogories::vectors,
        [](auto x, auto y)
        {
            mask<subtype<decltype(x)>, decltype(x)::scalar_size()> m(false);
            return kfr::select(m, x, y);
        },
        [](auto, auto y) { return y; });
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
