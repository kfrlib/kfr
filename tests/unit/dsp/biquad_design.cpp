/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/dsp/biquad_design.hpp>

#include <complex>

using namespace kfr;
using Catch::Approx;

TEST_CASE("biquad_allpass")
{
    test_matrix(named("type") = ctypes_t<float, double>{},
                [](auto type)
                {
                    using T = typename decltype(type)::type;

                    const auto bq          = biquad_allpass<T>(1000.0 / 48000.0, 1.0 / std::sqrt(2.0));
                    const double tolerance = std::is_same_v<T, float> ? 1e-6 : 1e-12;

                    // W3C Audio EQ Cookbook APF coefficients at 1 kHz / 48 kHz, Q = 1 / sqrt(2).
                    CHECK(bq.a0 == T(1));
                    CHECK(bq.a1 == Approx(-1.815341082704568).margin(tolerance));
                    CHECK(bq.a2 == Approx(0.8310055893467575).margin(tolerance));
                    CHECK(bq.b0 == bq.a2);
                    CHECK(bq.b1 == bq.a1);
                    CHECK(bq.b2 == bq.a0);

                    for (double frequency : { 0.0, 1000.0 / 48000.0, 0.1, 0.25, 0.49 })
                    {
                        INFO("frequency = " << frequency);
                        const auto z           = std::polar(1.0, -2 * c_pi<double> * frequency);
                        const auto numerator   = double(bq.b0) + double(bq.b1) * z + double(bq.b2) * z * z;
                        const auto denominator = double(bq.a0) + double(bq.a1) * z + double(bq.a2) * z * z;
                        CHECK(std::abs(numerator / denominator) == Approx(1.0).margin(1e-12));
                    }
                });
}
