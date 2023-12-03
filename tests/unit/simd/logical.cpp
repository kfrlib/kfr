/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/logical.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(logical_all)
{
    CHECK(all(mask<f32, 4>{ true, true, true, true }) == true);
    CHECK(all(mask<f32, 4>{ true, false, true, false }) == false);
    CHECK(all(mask<f32, 4>{ false, true, false, true }) == false);
    CHECK(all(mask<f32, 4>{ false, false, false, false }) == false);
}
TEST(logical_any)
{
    CHECK(any(mask<f32, 4>{ true, true, true, true }) == true);
    CHECK(any(mask<f32, 4>{ true, false, true, false }) == true);
    CHECK(any(mask<f32, 4>{ false, true, false, true }) == true);
    CHECK(any(mask<f32, 4>{ false, false, false, false }) == false);
}

TEST(intrin_any_all)
{
    testo::matrix(named("type") = unsigned_vector_types<vec>,
                  [](auto type)
                  {
                      using T                = typename decltype(type)::type;
                      constexpr size_t width = widthof<T>();
                      using Tsub             = subtype<T>;
                      const auto x           = enumerate<Tsub, width>() == Tsub(0);
                      CHECK(any(x) == true);
                      if (width == 1)
                          CHECK(all(x) == true);
                      else
                          CHECK(all(x) == false);
                      const auto y = zerovector<Tsub, width>() == Tsub(127);
                      CHECK(all(y) == false);
                      CHECK(any(y) == false);
                      const auto z = zerovector<Tsub, width>() == Tsub(0);
                      CHECK(all(z) == true);
                      CHECK(any(z) == true);
                  });
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
