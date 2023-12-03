/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/basic_expressions.hpp>
#include <kfr/base/univector.hpp>
#include <kfr/io/tostring.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(univector_assignment)
{
    univector<int> x = truncate(counter(), 10);
    CHECK(x.size() == 10u);

    univector<int> y;
    y = truncate(counter(), 10);
    CHECK(y.size() == 10u);
}

#ifdef KFR_USE_STD_ALLOCATION
TEST(std_allocation)
{
    univector<float> u;
    std::vector<float>& v = u;

    std::vector<float> v2{ 1, 2, 3, 4 };

    // Technically an UB but ok with all sane compilers
    reinterpret_cast<univector<float>&>(v2) += 100.f;
    CHECK(v2[0] == 101);
    CHECK(v2[1] == 102);
    CHECK(v2[2] == 103);
    CHECK(v2[3] == 104);
}
#endif

} // namespace CMT_ARCH_NAME
} // namespace kfr
