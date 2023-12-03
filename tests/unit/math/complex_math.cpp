/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/math/complex_math.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

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

    testo::epsilon_scope<void> eps(5);

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

} // namespace CMT_ARCH_NAME
} // namespace kfr
