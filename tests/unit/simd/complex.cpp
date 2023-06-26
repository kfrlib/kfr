/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/complex.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(complex_convertible)
{
    static_assert(std::is_convertible<float, complex<float>>::value, "");
    static_assert(std::is_convertible<float, complex<double>>::value, "");
    static_assert(std::is_convertible<short, complex<double>>::value, "");

    static_assert(std::is_convertible<complex<float>, vec<complex<float>, 4>>::value, "");
    static_assert(!std::is_convertible<vec<complex<float>, 1>, vec<complex<float>, 4>>::value, "");

    static_assert(std::is_convertible<vec<complex<float>, 2>, vec<complex<double>, 2>>::value, "");
    static_assert(std::is_convertible<vec<vec<float, 4>, 2>, vec<vec<double, 4>, 2>>::value, "");

    CHECK(static_cast<complex<float>>(10.f) == complex<float>{ 10.f, 0.f });
    CHECK(static_cast<complex<double>>(10.f) == complex<double>{ 10., 0. });
    CHECK(static_cast<complex<double>>(static_cast<short>(10)) == complex<double>{ 10., 0. });

    CHECK(static_cast<vec<complex<float>, 2>>(complex<float>{ 1.f, 2.f }) ==
          vec<complex<float>, 2>{ c32{ 1.f, 2.f }, c32{ 1.f, 2.f } });

    CHECK(static_cast<vec<complex<float>, 4>>(complex<float>{ 1.f, 2.f }) ==
          vec<complex<float>, 4>{ c32{ 1.f, 2.f }, c32{ 1.f, 2.f }, c32{ 1.f, 2.f }, c32{ 1.f, 2.f } });

    CHECK(static_cast<vec<complex<double>, 2>>(vec<complex<float>, 2>{ c32{ 1.f, 2.f }, c32{ 1.f, 2.f } }) ==
          vec<complex<double>, 2>{ c64{ 1., 2. }, c64{ 1., 2. } });
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
