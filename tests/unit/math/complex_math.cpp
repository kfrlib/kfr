/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/math/complex_math.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

TEST_CASE("complex_math")
{
    const vec<c32, 1> a{ c32{ 1, 2 } };
    const vec<c32, 1> b{ c32{ 3, 4 } };
    CHECK(c32(vec<c32, 1>(2)[0]) == c32{ 2, 0 });
    CHECK_THAT(a + b, DeepMatcher(make_vector(c32{ 4, 6 })));
    CHECK_THAT(a - b, DeepMatcher(make_vector(c32{ -2, -2 })));
    CHECK_THAT(a * b, DeepMatcher(make_vector(c32{ -5, 10 })));
    CHECK_THAT((a * vec<c32, 1>(2)), DeepMatcher(make_vector(c32{ 2, 4 })));
    CHECK_THAT(a * 2, DeepMatcher(make_vector(c32{ 2, 4 })));
    CHECK_THAT(a / b, DeepMatcher(make_vector(c32{ 0.44f, 0.08f })));
    CHECK_THAT(-a, DeepMatcher(make_vector(c32{ -1, -2 })));

    CHECK_THAT(real(a), DeepMatcher(make_vector(1.f)));
    CHECK_THAT(imag(a), DeepMatcher(make_vector(2.f)));

    CHECK_THAT(make_complex(5.f, 7), DeepMatcher(c32{ 5.f, 7.f }));
    CHECK_THAT(make_complex(make_vector(5.f, 8.f), make_vector(7.f, 9.f)),
               DeepMatcher(make_vector(c32{ 5.f, 7.f }, c32{ 8.f, 9.f })));

    CHECK_THAT(cabs(c32{ 3.f, 4.f }), DeepMatcher(5.f));
    CHECK_THAT(cabs(make_vector(c32{ 3.f, 4.f })), DeepMatcher(make_vector(5.f)));

    CHECK_THAT(cabs(-3.f), DeepMatcher(3.f));
    CHECK_THAT(cabs(make_vector(-3.f)), DeepMatcher(make_vector(3.f)));

    CHECK_THAT((vec<c32, 4>{ 100, 100, 100, 100 } + vec<c32, 4>{ 1, 2, 3, 4 }),
               DeepMatcher(vec<c32, 4>{ 101, 102, 103, 104 }));
    CHECK_THAT((c32{ 100 } + vec<c32, 4>{ 1, 2, 3, 4 }), DeepMatcher(vec<c32, 4>{ 101, 102, 103, 104 }));

    CHECK_THAT((carg(c32{ +1.f, 0.f })), DeepMatcher(0.f));
    CHECK_THAT((carg(c32{ 0.f, +1.f })), DeepMatcher(c_pi<float> / 2));
    CHECK_THAT((carg(c32{ 0.f, -1.f })), DeepMatcher(-c_pi<float> / 2));
    CHECK_THAT((carg(c32{ -1.f, 0.f })), DeepMatcher(c_pi<float>));

    epsilon_scope<void> eps(5);

    CHECK_THAT((csin(c32{ 1.f, 1.f })), DeepMatcher(c32{ 1.2984575814159773f, 0.634963914784736f }));
    CHECK_THAT((ccos(c32{ 1.f, 1.f })), DeepMatcher(c32{ 0.8337300251311489f, -0.9888977057628651f }));
    CHECK_THAT((csinh(c32{ 1.f, 1.f })), DeepMatcher(c32{ 0.634963914784736f, 1.2984575814159773f }));
    CHECK_THAT((ccosh(c32{ 1.f, 1.f })), DeepMatcher(c32{ 0.8337300251311489f, 0.9888977057628651f }));

    CHECK_THAT((clog(c32{ 1.f, 1.f })), DeepMatcher(c32{ 0.34657359027997264f, 0.7853981633974483f }));
    CHECK_THAT((clog2(c32{ 1.f, 1.f })), DeepMatcher(c32{ 0.5f, 1.1330900354567983f }));
    CHECK_THAT((clog10(c32{ 1.f, 1.f })), DeepMatcher(c32{ 0.15051499783199057f, 0.3410940884604603f }));

    CHECK_THAT((cexp(c32{ 1.f, 1.f })), DeepMatcher(c32{ 1.4686939399158849f, 2.2873552871788423f }));
    CHECK_THAT((cexp2(c32{ 1.f, 1.f })), DeepMatcher(c32{ 1.5384778027279442f, 1.2779225526272695f }));
    CHECK_THAT((cexp10(c32{ 1.f, 1.f })), DeepMatcher(c32{ -6.682015101903131f, 7.439803369574931f }));

#ifdef KFR_NATIVE_F64
    CHECK_THAT((csin(c64{ 1.0, 1.0 })), DeepMatcher(c64{ 1.2984575814159773, 0.634963914784736 }));
    CHECK_THAT((ccos(c64{ 1.0, 1.0 })), DeepMatcher(c64{ 0.8337300251311489, -0.9888977057628651 }));
    CHECK_THAT((csinh(c64{ 1.0, 1.0 })), DeepMatcher(c64{ 0.634963914784736, 1.2984575814159773 }));
    CHECK_THAT((ccosh(c64{ 1.0, 1.0 })), DeepMatcher(c64{ 0.8337300251311489, 0.9888977057628651 }));
    CHECK_THAT((clog(c64{ 1.0, 1.0 })), DeepMatcher(c64{ 0.34657359027997264, 0.7853981633974483 }));
    CHECK_THAT((cexp(c64{ 1.0, 1.0 })), DeepMatcher(c64{ 1.4686939399158849, 2.2873552871788423 }));
#endif
}

TEST_CASE("complex_functions")
{
    CHECK_THAT((csqr(complex<f32>(4.f, 0.f))), DeepMatcher(c32{ 16.f, 0.f }));
    CHECK_THAT((csqrt(complex<f32>(16.f, 0.f))), DeepMatcher(c32{ 4.f, 0.f }));

    CHECK_THAT((csqr(complex<f32>(1.f, 4.f))), DeepMatcher(c32{ -15.f, 8.f }));

    CHECK_THAT((csqrt(complex<f32>(15.f, 8.f))), DeepMatcher(c32{ 4.f, 1.f }));
    CHECK_THAT((csqrt(complex<f32>(-15.f, 8.f))), DeepMatcher(c32{ 1.f, 4.f }));
    CHECK_THAT((csqrt(complex<f32>(15.f, -8.f))), DeepMatcher(c32{ 4.f, -1.f }));
    CHECK_THAT((csqrt(complex<f32>(-15.f, -8.f))), DeepMatcher(c32{ 1.f, -4.f }));
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
