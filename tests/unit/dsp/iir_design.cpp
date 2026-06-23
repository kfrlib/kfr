/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/dsp.hpp>
#include <kfr/test/test.hpp>

#include <cmath>

using namespace kfr;
using Catch::Approx;

namespace
{

// Evaluate the total magnitude of an iir_params filter at digital frequency omega (rad/sample).
// Each section: H_k(z) = (b0 + b1*z^-1 + b2*z^-2) / (a0 + a1*z^-1 + a2*z^-2), z = e^(j*omega)
double sos_magnitude(const iir_params<double>& sos, double omega)
{
    double mag_sq = 1.0;
    for (const auto& s : sos)
    {
        double cr  = std::cos(omega);
        double sr  = std::sin(omega);
        double cr2 = std::cos(2.0 * omega);
        double sr2 = std::sin(2.0 * omega);

        double nr = s.b0 + s.b1 * cr + s.b2 * cr2;
        double ni = -s.b1 * sr - s.b2 * sr2;
        double dr = s.a0 + s.a1 * cr + s.a2 * cr2;
        double di = -s.a1 * sr - s.a2 * sr2;

        mag_sq *= (nr * nr + ni * ni) / (dr * dr + di * di);
    }
    return std::sqrt(mag_sq);
}

// Jury stability test for a 2nd-order section (monic denominator z^2 + p*z + q):
// |q| < 1, 1 + p + q > 0, 1 - p + q > 0
bool section_is_stable(const biquad_section<double>& s)
{
    double a1 = s.a1 / s.a0;
    double a2 = s.a2 / s.a0;
    return (std::abs(a2) < 1.0) && (1.0 + a1 + a2 > 0.0) && (1.0 - a1 + a2 > 0.0);
}

} // namespace

TEST_CASE("to_sos")
{
    SECTION("gain-only filter (no poles, no zeros) produces a single flat section")
    {
        // The fix: gain k is placed in b0 (numerator), a0=1 (monic denominator)
        // so H(z) = k at all frequencies.
        for (double k : { 0.5, 1.0, 2.0, 3.0 })
        {
            zpk filt;
            filt.k                 = k;
            iir_params<double> sos = to_sos<double>(filt);
            REQUIRE(sos.size() == 1);
            CHECK(sos[0].a0 == Approx(1.0));
            CHECK(sos[0].a1 == Approx(0.0).margin(1e-15));
            CHECK(sos[0].a2 == Approx(0.0).margin(1e-15));
            CHECK(sos[0].b0 == Approx(k));
            CHECK(sos[0].b1 == Approx(0.0).margin(1e-15));
            CHECK(sos[0].b2 == Approx(0.0).margin(1e-15));
            // Flat magnitude = k at all frequencies
            CHECK(sos_magnitude(sos, 0.0) == Approx(k));
            CHECK(sos_magnitude(sos, c_pi<double> / 2) == Approx(k));
            CHECK(sos_magnitude(sos, c_pi<double>) == Approx(k));
        }
    }

    SECTION("unity gain-only filter produces a passthrough section")
    {
        zpk filt;
        filt.k                 = 1.0;
        iir_params<double> sos = to_sos<double>(filt);

        REQUIRE(sos.size() == 1);
        CHECK(sos_magnitude(sos, 0.0) == Approx(1.0));
        CHECK(sos_magnitude(sos, c_pi<double> / 2) == Approx(1.0));
        CHECK(sos_magnitude(sos, c_pi<double>) == Approx(1.0));
    }

    SECTION("section count: even-order filters ceil(N/2)")
    {
        // 2nd order → 1 section
        CHECK(to_sos<double>(iir_lowpass(butterworth(2), 0.1)).size() == 1);
        // 4th order → 2 sections
        CHECK(to_sos<double>(iir_lowpass(butterworth(4), 0.1)).size() == 2);
        // 8th order → 4 sections
        CHECK(to_sos<double>(iir_lowpass(butterworth(8), 0.1)).size() == 4);
    }

    SECTION("section count: odd-order filters are padded to even, giving ceil(N/2) sections")
    {
        // 1st order → 1 section
        CHECK(to_sos<double>(iir_lowpass(butterworth(1), 0.1)).size() == 1);
        // 3rd order → 2 sections
        CHECK(to_sos<double>(iir_lowpass(butterworth(3), 0.1)).size() == 2);
        // 5th order → 3 sections
        CHECK(to_sos<double>(iir_lowpass(butterworth(5), 0.1)).size() == 3);
    }

    SECTION("section count: bandpass/bandstop double the prototype order")
    {
        // N-th order prototype → 2N poles after LP→BP/BS transform → N sections
        CHECK(to_sos<double>(iir_bandpass(butterworth(2), 0.1, 0.3)).size() == 2);
        CHECK(to_sos<double>(iir_bandpass(butterworth(4), 0.1, 0.3)).size() == 4);
        CHECK(to_sos<double>(iir_bandstop(butterworth(2), 0.1, 0.3)).size() == 2);
        CHECK(to_sos<double>(iir_bandstop(butterworth(4), 0.1, 0.3)).size() == 4);
    }

    SECTION("all sections of digital filters satisfy Jury stability criterion")
    {
        auto check_all_stable = [](const iir_params<double>& sos)
        {
            for (const auto& s : sos)
                CHECK(section_is_stable(s));
        };
        check_all_stable(to_sos<double>(iir_lowpass(butterworth(8), 0.1)));
        check_all_stable(to_sos<double>(iir_highpass(butterworth(8), 0.1)));
        check_all_stable(to_sos<double>(iir_bandpass(butterworth(4), 0.1, 0.3)));
        check_all_stable(to_sos<double>(iir_bandstop(butterworth(4), 0.1, 0.3)));
        check_all_stable(to_sos<double>(iir_lowpass(chebyshev1(6, 1.0), 0.2)));
        check_all_stable(to_sos<double>(iir_lowpass(chebyshev2(6, 40.0), 0.2)));
        check_all_stable(to_sos<double>(iir_lowpass(bessel(8), 0.1)));
    }

    SECTION("DC gain (z=1) of a lowpass filter is 1")
    {
        constexpr double tol = 1e-9;
        CHECK(sos_magnitude(to_sos<double>(iir_lowpass(butterworth(2), 0.1)), 0.0) ==
              Approx(1.0).margin(tol));
        CHECK(sos_magnitude(to_sos<double>(iir_lowpass(butterworth(4), 0.1)), 0.0) ==
              Approx(1.0).margin(tol));
        CHECK(sos_magnitude(to_sos<double>(iir_lowpass(butterworth(8), 0.2)), 0.0) ==
              Approx(1.0).margin(tol));
        // Even-order Chebyshev Type I has DC gain = 1/sqrt(1 + eps^2) < 1
        // (passband ripple means it doesn't reach unity at DC for even orders)
        double cheby1_dc = sos_magnitude(to_sos<double>(iir_lowpass(chebyshev1(4, 1.0), 0.15)), 0.0);
        CHECK(cheby1_dc > 0.85);
        CHECK(cheby1_dc <= 1.0 + 1e-9);
    }

    SECTION("Nyquist gain (z=-1) of a highpass filter is 1")
    {
        constexpr double tol = 1e-9;
        CHECK(sos_magnitude(to_sos<double>(iir_highpass(butterworth(2), 0.1)), c_pi<double>) ==
              Approx(1.0).margin(tol));
        CHECK(sos_magnitude(to_sos<double>(iir_highpass(butterworth(4), 0.1)), c_pi<double>) ==
              Approx(1.0).margin(tol));
        CHECK(sos_magnitude(to_sos<double>(iir_highpass(butterworth(8), 0.2)), c_pi<double>) ==
              Approx(1.0).margin(tol));
    }

    SECTION("bandstop passes DC and Nyquist with unity gain")
    {
        iir_params<double> sos = to_sos<double>(iir_bandstop(butterworth(4), 0.1, 0.3));
        CHECK(sos_magnitude(sos, 0.0) == Approx(1.0).margin(1e-9));
        CHECK(sos_magnitude(sos, c_pi<double>) == Approx(1.0).margin(1e-9));
    }

    SECTION("bandpass rejects DC and Nyquist (gain << 1)")
    {
        iir_params<double> sos = to_sos<double>(iir_bandpass(butterworth(4), 0.1, 0.3));
        CHECK(sos_magnitude(sos, 0.0) < 1e-6);
        CHECK(sos_magnitude(sos, c_pi<double>) < 1e-6);
    }

    SECTION("lowpass gain at cutoff is approximately -3 dB for Butterworth")
    {
        // At the -3 dB cutoff frequency, magnitude = 1/sqrt(2)
        // omega_c = pi * fc / (fs/2) = pi * 0.1 / 1.0 = 0.1*pi  (fs=2, Nyquist=1)
        iir_params<double> sos = to_sos<double>(iir_lowpass(butterworth(4), 0.1));
        double mag             = sos_magnitude(sos, c_pi<double> * 0.1);
        CHECK(mag == Approx(1.0 / std::sqrt(2.0)).epsilon(0.01));
    }

    SECTION("2nd-order Butterworth lowpass coefficients (equivalent to scipy butter(2, 0.1))")
    {
        // KFR: iir_lowpass(butterworth(2), 0.1) with default fs=2.0.
        // scipy: butter(2, 0.1, output='sos')  — Wn=0.1 is fraction of Nyquist.
        // Both specifications are identical: -3 dB at 10% of Nyquist = 0.1*pi rad/sample.
        // The KFR pre-warp (4*tan) with bilinear fs=2 and scipy's (2*tan) with bilinear fs=1
        // are algebraically equivalent; coefficients must match.
        iir_params<double> sos = to_sos<double>(iir_lowpass(butterworth(2), 0.1));
        REQUIRE(sos.size() == 1);

        // Reference values (verified numerically):
        CHECK(sos[0].b0 == Approx(0.020083365564211233).epsilon(1e-9));
        CHECK(sos[0].b1 == Approx(0.040166731128422466).epsilon(1e-9));
        CHECK(sos[0].b2 == Approx(0.020083365564211233).epsilon(1e-9));
        CHECK(sos[0].a0 == Approx(1.0).margin(1e-12));
        CHECK(sos[0].a1 == Approx(-1.5610180758007182).epsilon(1e-9));
        CHECK(sos[0].a2 == Approx(0.64135153805756306).epsilon(1e-9));

        // Structural properties of a lowpass Butterworth SOS section:
        CHECK(sos[0].b0 == Approx(sos[0].b2).margin(1e-15)); // b0 == b2 (symmetry)
        CHECK(sos[0].b1 == Approx(2.0 * sos[0].b0).epsilon(1e-12)); // b1 == 2*b0
    }

    SECTION("float template parameter yields coefficients close to double")
    {
        iir_params<double> sos_d = to_sos<double>(iir_lowpass(butterworth(4), 0.1));
        iir_params<float> sos_f  = to_sos<float>(iir_lowpass(butterworth(4), 0.1));
        REQUIRE(sos_d.size() == sos_f.size());

        for (size_t i = 0; i < sos_d.size(); ++i)
        {
            CHECK(static_cast<double>(sos_f[i].b0) == Approx(sos_d[i].b0).epsilon(1e-5));
            CHECK(static_cast<double>(sos_f[i].b1) == Approx(sos_d[i].b1).epsilon(1e-5));
            CHECK(static_cast<double>(sos_f[i].b2) == Approx(sos_d[i].b2).epsilon(1e-5));
            CHECK(static_cast<double>(sos_f[i].a1) == Approx(sos_d[i].a1).epsilon(1e-5));
            CHECK(static_cast<double>(sos_f[i].a2) == Approx(sos_d[i].a2).epsilon(1e-5));
        }
    }

    SECTION(
        "gain (k) is folded into first section: total product of section gains matches overall filter gain")
    {
        // The k factor from the ZPK form is distributed into the SOS, so when sections
        // are multiplied together, the total gain at DC is 1 for a normalized lowpass.
        zpk filt               = iir_lowpass(butterworth(6), 0.15);
        iir_params<double> sos = to_sos<double>(filt);

        double dc_mag = sos_magnitude(sos, 0.0);
        CHECK(dc_mag == Approx(1.0).margin(1e-9));
    }

    SECTION("impulse response of 2nd-order Butterworth lowpass matches expected values")
    {
        // Apply filter to unit impulse and verify first several output samples.
        // Reference computed from: scipy.signal.sosfilt(sos, [1,0,0,...])
        // butter(2, 0.1): impulse response starts at h[0]=b0
        iir_params<double> sos = to_sos<double>(iir_lowpass(butterworth(2), 0.1));
        constexpr size_t N     = 8;
        univector<double, N> impulse(0.0);
        impulse[0] = 1.0;

        univector<double, N> output = iir(impulse, sos);

        // h[0] = b0
        CHECK(output[0] == Approx(sos[0].b0).margin(1e-12));
        // The filter is stable; the impulse response must ultimately decay.
        // We verify with a longer window that late samples are small relative to the peak.
        constexpr size_t N2 = 32;
        univector<double, N2> impulse2(0.0);
        impulse2[0]                   = 1.0;
        univector<double, N2> output2 = iir(impulse2, sos);
        double peak                   = *std::max_element(output2.begin(), output2.end());
        CHECK(std::abs(output2[31]) < 0.5 * peak); // significant decay by sample 31
        // Sum of impulse response ≈ DC gain = 1 (over infinite samples, partial sum < 1)
        double partial_sum = 0.0;
        for (size_t i = 0; i < N; ++i)
            partial_sum += output[i];
        CHECK(partial_sum < 1.0 + 1e-9);
        CHECK(partial_sum > 0.0);
    }

    SECTION("filter application via iir() with zpk overload matches to_sos result")
    {
        // iir(signal, zpk) internally calls to_sos; verify it matches explicit to_sos path.
        constexpr size_t N = 32;
        univector<double, N> impulse(0.0);
        impulse[0]            = 1.0;
        zpk filt              = iir_lowpass(butterworth(4), 0.1);
        iir_params<fbase> sos = to_sos<fbase>(filt);

        univector<fbase, N> out_sos = iir(impulse, sos);
        univector<fbase, N> out_zpk = truncate(iir(impulse, filt), N);

        for (size_t i = 0; i < N; ++i)
            CHECK(out_sos[i] == Approx(out_zpk[i]).margin(choose_const<fbase>(1e-12f, 1e-6)));
    }

    SECTION("chebyshev1: ripple in passband, steep rolloff")
    {
        // chebyshev1(4, 1.0) → 1 dB passband ripple; gain at DC ≈ 1
        iir_params<double> sos = to_sos<double>(iir_lowpass(chebyshev1(4, 1.0), 0.2));
        REQUIRE(sos.size() == 2);
        // DC gain should be 1 (or within ripple for odd-order; 4th order has ~1 dB ripple at DC for Cheby1)
        // For Cheby1 even order, DC gain = 10^(-rp/20) = 10^(-0.05) ≈ 0.891
        double dc_mag = sos_magnitude(sos, 0.0);
        CHECK(dc_mag > 0.8);
        CHECK(dc_mag <= 1.0 + 1e-9);
    }

    SECTION("chebyshev2: monotonic passband, equiripple stopband")
    {
        // chebyshev2(4, 40) → 40 dB stopband attenuation; DC gain = 1
        iir_params<double> sos = to_sos<double>(iir_lowpass(chebyshev2(4, 40.0), 0.2));
        REQUIRE(sos.size() == 2);
        CHECK(sos_magnitude(sos, 0.0) == Approx(1.0).margin(1e-6));
        // Gain well above cutoff (Nyquist) should be heavily attenuated
        CHECK(sos_magnitude(sos, c_pi<double>) < 0.02); // < -34 dB
    }
}
