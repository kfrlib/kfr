/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/abs.hpp>

#include <kfr/io.hpp>

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4146))

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
TEST(abs)
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::abs(x); },
        [](auto x) -> decltype(x) { return x >= 0 ? x : -x; });
}
} // namespace KFR_ARCH_NAME
} // namespace kfr

KFR_PRAGMA_MSVC(warning(pop))
