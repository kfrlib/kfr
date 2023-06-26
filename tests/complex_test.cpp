/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <complex>
#include <kfr/base.hpp>
#include <kfr/io.hpp>

using namespace kfr;

namespace CMT_ARCH_NAME
{

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

TEST(complex_math)
{
    const vec<c32, 1> a{ c32{ 1, 2 } };
    const vec<c32, 1> b{ c32{ 3, 4 } };
    CHECK(c32(vec<c32, 1>(2)[0]) == c32{ 2, 0 });
    CHECK(a + b == make_vector(c32{ 4, 6 }));
    CHECK(a - b == make_vector(c32{ -2, -2 }));
    CHECK(a * b == make_vector(c32{ -5, 10 }));
    CHECK(a * vec<c32, 1>(2) == make_vector(c32{ 2, 4 }));
    CHECK(a * 2 == make_vector(c32{ 2, 4 }));
    CHECK(a / b == make_vector(c32{ 0.44f, 0.08f }));
    CHECK(-a == make_vector(c32{ -1, -2 }));

    CHECK(real(a) == make_vector(1.f));
    CHECK(imag(a) == make_vector(2.f));

    CHECK(make_complex(5.f, 7) == c32{ 5.f, 7.f });
    CHECK(make_complex(make_vector(5.f, 8.f), make_vector(7.f, 9.f)) ==
          make_vector(c32{ 5.f, 7.f }, c32{ 8.f, 9.f }));

    CHECK(cabs(c32{ 3.f, 4.f }) == 5.f);
    CHECK(cabs(make_vector(c32{ 3.f, 4.f })) == make_vector(5.f));

    CHECK(cabs(-3.f) == 3.f);
    CHECK(cabs(make_vector(-3.f)) == make_vector(3.f));

    CHECK(carg(c32{ +1.f, 0.f }) == 0.f);
    CHECK(carg(c32{ 0.f, +1.f }) == c_pi<float> / 2);
    CHECK(carg(c32{ 0.f, -1.f }) == -c_pi<float> / 2);
    CHECK(carg(c32{ -1.f, 0.f }) == c_pi<float>);

    testo::eplison_scope<void> eps(5);

    CHECK(csin(c32{ 1.f, 1.f }) == c32{ 1.2984575814159773f, 0.634963914784736f });
    CHECK(ccos(c32{ 1.f, 1.f }) == c32{ 0.8337300251311489f, -0.9888977057628651f });
    CHECK(csinh(c32{ 1.f, 1.f }) == c32{ 0.634963914784736f, 1.2984575814159773f });
    CHECK(ccosh(c32{ 1.f, 1.f }) == c32{ 0.8337300251311489f, 0.9888977057628651f });

    CHECK(clog(c32{ 1.f, 1.f }) == c32{ 0.34657359027997264f, 0.7853981633974483f });
    CHECK(clog2(c32{ 1.f, 1.f }) == c32{ 0.5f, 1.1330900354567983f });
    CHECK(clog10(c32{ 1.f, 1.f }) == c32{ 0.15051499783199057f, 0.3410940884604603f });

    CHECK(cexp(c32{ 1.f, 1.f }) == c32{ 1.4686939399158849f, 2.2873552871788423f });
    CHECK(cexp2(c32{ 1.f, 1.f }) == c32{ 1.5384778027279442f, 1.2779225526272695f });
    CHECK(cexp10(c32{ 1.f, 1.f }) == c32{ -6.682015101903131f, 7.439803369574931f });

#ifdef CMT_NATIVE_F64
    CHECK(csin(c64{ 1.0, 1.0 }) == c64{ 1.2984575814159773, 0.634963914784736 });
    CHECK(ccos(c64{ 1.0, 1.0 }) == c64{ 0.8337300251311489, -0.9888977057628651 });
    CHECK(csinh(c64{ 1.0, 1.0 }) == c64{ 0.634963914784736, 1.2984575814159773 });
    CHECK(ccosh(c64{ 1.0, 1.0 }) == c64{ 0.8337300251311489, 0.9888977057628651 });
    CHECK(clog(c64{ 1.0, 1.0 }) == c64{ 0.34657359027997264, 0.7853981633974483 });
    CHECK(cexp(c64{ 1.0, 1.0 }) == c64{ 1.4686939399158849, 2.2873552871788423 });
#endif
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

TEST(complex_shuffle)
{
    const vec<c32, 2> a{ c32{ 0, 1 }, c32{ 2, 3 } };
    CHECK(reverse(a) == make_vector(c32{ 2, 3 }, c32{ 0, 1 }));
}

TEST(complex_basic_expressions)
{
    const univector<c32, 15> uv1 = zeros();
    CHECK(uv1[0] == c32{ 0, 0 });
    CHECK(uv1[1] == c32{ 0, 0 });
    CHECK(uv1[2] == c32{ 0, 0 });
    CHECK(uv1[14] == c32{ 0, 0 });
    const univector<c32, 15> uv2 = ones();
    CHECK(uv2[0] == c32{ 1, 0 });
    CHECK(uv2[1] == c32{ 1, 0 });
    CHECK(uv2[2] == c32{ 1, 0 });
    CHECK(uv2[14] == c32{ 1, 0 });
    const univector<c32, 15> uv3 = counter();
    CHECK(uv3[0] == c32{ 0, 0 });
    CHECK(uv3[1] == c32{ 1, 0 });
    CHECK(uv3[2] == c32{ 2, 0 });
    CHECK(uv3[14] == c32{ 14, 0 });
}

TEST(complex_functions)
{
    CHECK(csqr(complex<f32>(4.f, 0.f)) == c32{ 16.f, 0.f });
    CHECK(csqrt(complex<f32>(16.f, 0.f)) == c32{ 4.f, 0.f });

    CHECK(csqr(complex<f32>(1.f, 4.f)) == c32{ -15.f, 8.f });

    CHECK(csqrt(complex<f32>(15.f, 8.f)) == c32{ 4.f, 1.f });
    CHECK(csqrt(complex<f32>(-15.f, 8.f)) == c32{ 1.f, 4.f });
    CHECK(csqrt(complex<f32>(15.f, -8.f)) == c32{ 4.f, -1.f });
    CHECK(csqrt(complex<f32>(-15.f, -8.f)) == c32{ 1.f, -4.f });
}

TEST(complex_function_expressions)
{
    const univector<c32, 4> uv1 = sqr(counter());
    CHECK(uv1[0] == c32{ 0, 0 });
    CHECK(uv1[1] == c32{ 1, 0 });
    CHECK(uv1[2] == c32{ 4, 0 });
    CHECK(uv1[3] == c32{ 9, 0 });

    const univector<c32, 4> uv2 = uv1 * 2.f;
    CHECK(uv2[0] == c32{ 0, 0 });
    CHECK(uv2[1] == c32{ 2, 0 });
    CHECK(uv2[2] == c32{ 8, 0 });
    CHECK(uv2[3] == c32{ 18, 0 });

    const univector<f32, 4> uv3 = real(uv2);
    CHECK(uv3[0] == 0.f);
    CHECK(uv3[1] == 2.f);
    CHECK(uv3[2] == 8.f);
    CHECK(uv3[3] == 18.f);
    testo::assert_is_same<c32, expression_value_type<decltype(uv2)>>();
    testo::assert_is_same<f32, expression_value_type<decltype(uv3)>>();
    testo::assert_is_same<f32, expression_value_type<decltype(real(uv2))>>();
}

TEST(static_tests)
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
} // namespace CMT_ARCH_NAME

#ifndef KFR_NO_MAIN
int main()
{
    println(library_version());

    return testo::run_all("", true);
}
#endif
