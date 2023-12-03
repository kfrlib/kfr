/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/complex.hpp>
#include <kfr/simd/read_write.hpp>

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

TEST(complex_static_tests)
{
    static_assert(is_numeric<vec<complex<float>, 4>>, "");
    static_assert(is_numeric_args<vec<complex<float>, 4>>, "");

    static_assert(sizeof(vec<c32, 4>) == sizeof(vec<f32, 8>), "");
    static_assert(vec<f32, 4>::size() == 4, "");
    static_assert(vec<c32, 4>::size() == 4, "");
    static_assert(vec<f32, 4>::scalar_size() == 4, "");
    static_assert(vec<c32, 4>::scalar_size() == 8, "");
    testo::assert_is_same<subtype<complex<i32>>, i32>();
    testo::assert_is_same<vec<c32, 4>::value_type, c32>();
    testo::assert_is_same<vec<c32, 4>::scalar_type, f32>();
    testo::assert_is_same<vec<f32, 4>::value_type, f32>();
    testo::assert_is_same<vec<f32, 4>::scalar_type, f32>();
    testo::assert_is_same<vec<c32, 1>, decltype(make_vector(c32{ 0, 0 }))>();
    testo::assert_is_same<vec<c32, 2>, decltype(make_vector(c32{ 0, 0 }, 4))>();
    testo::assert_is_same<ftype<complex<i32>>, complex<f32>>();
    testo::assert_is_same<ftype<complex<i64>>, complex<f64>>();
    testo::assert_is_same<ftype<vec<complex<i32>, 4>>, vec<complex<f32>, 4>>();
    testo::assert_is_same<ftype<vec<complex<i64>, 8>>, vec<complex<f64>, 8>>();

    testo::assert_is_same<std::common_type_t<complex<int>, double>, complex<double>>();
}

TEST(complex_shuffle)
{
    const vec<c32, 2> a{ c32{ 0, 1 }, c32{ 2, 3 } };
    CHECK(reverse(a) == make_vector(c32{ 2, 3 }, c32{ 0, 1 }));
}

TEST(complex_read_write)
{
    c32 buffer[8] = { c32{ 1, 2 },  c32{ 3, 4 },   c32{ 5, 6 },   c32{ 7, 8 },
                      c32{ 9, 10 }, c32{ 11, 12 }, c32{ 13, 14 }, c32{ 15, 16 } };

    CHECK(read<4>(buffer) == make_vector(c32{ 1, 2 }, c32{ 3, 4 }, c32{ 5, 6 }, c32{ 7, 8 }));
    CHECK(read<3>(buffer + 1) == make_vector(c32{ 3, 4 }, c32{ 5, 6 }, c32{ 7, 8 }));
    write(buffer + 2, make_vector(c32{ 10, 11 }, c32{ 12, 13 }));
    CHECK(read<4>(buffer) == make_vector(c32{ 1, 2 }, c32{ 3, 4 }, c32{ 10, 11 }, c32{ 12, 13 }));
}

TEST(complex_cast)
{
    const vec<f32, 4> v1 = bitcast<f32>(make_vector(c32{ 0, 1 }, c32{ 2, 3 }));
    CHECK(v1.flatten()[0] == 0.f);
    CHECK(v1.flatten()[1] == 1.f);
    CHECK(v1.flatten()[2] == 2.f);
    CHECK(v1.flatten()[3] == 3.f);

    const vec<c32, 1> v2 = bitcast<c32>(make_vector(1.f, 2.f));
    CHECK(v2.flatten()[0] == 1.f);
    CHECK(v2.flatten()[1] == 2.f);

    const vec<c32, 2> v3 = make_vector(1.f, 2.f);
    CHECK(v3.flatten()[0] == 1.f);
    CHECK(v3.flatten()[1] == 0.f);
    CHECK(v3.flatten()[2] == 2.f);
    CHECK(v3.flatten()[3] == 0.f);

    const vec<c32, 2> v4 = make_vector(1, 2);
    CHECK(v4.flatten()[0] == 1.f);
    CHECK(v4.flatten()[1] == 0.f);
    CHECK(v4.flatten()[2] == 2.f);
    CHECK(v4.flatten()[3] == 0.f);

    CHECK(zerovector<c32, 4>() == make_vector(c32{ 0, 0 }, c32{ 0, 0 }, c32{ 0, 0 }, c32{ 0, 0 }));
    CHECK(enumerate<c32, 4>() == make_vector(c32{ 0, 0 }, c32{ 1, 0 }, c32{ 2, 0 }, c32{ 3, 0 }));
}

TEST(complex_vector)
{
    const vec<c32, 1> c32x1{ c32{ 0, 1 } };
    CHECK(c32x1.flatten()[0] == 0.0f);
    CHECK(c32x1.flatten()[1] == 1.0f);

    const vec<c32, 2> c32x2{ c32{ 0, 1 }, c32{ 2, 3 } };
    CHECK(c32x2.flatten()[0] == 0.0f);
    CHECK(c32x2.flatten()[1] == 1.0f);
    CHECK(c32x2.flatten()[2] == 2.0f);
    CHECK(c32x2.flatten()[3] == 3.0f);

    const vec<c32, 3> c32x3{ c32{ 0, 1 }, c32{ 2, 3 }, c32{ 4, 5 } };
    CHECK(c32x3.flatten()[0] == 0.0f);
    CHECK(c32x3.flatten()[1] == 1.0f);
    CHECK(c32x3.flatten()[2] == 2.0f);
    CHECK(c32x3.flatten()[3] == 3.0f);
    CHECK(c32x3.flatten()[4] == 4.0f);
    CHECK(c32x3.flatten()[5] == 5.0f);

    const vec<c32, 1> c32s = 2;
    CHECK(c32s.flatten()[0] == 2.f);
    CHECK(c32s.flatten()[1] == 0.f);
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
