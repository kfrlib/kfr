/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <complex>
#include <kfr/dsp/fir.hpp>
#include <kfr/dsp/fir_design.hpp>
#include <kfr/dsp/waveshaper.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

TEST_CASE("waveshaper_zero_drive_is_identity")
{
    const univector<float, 4> input{ -1.f, -0.25f, 0.f, 0.5f };
    const univector<float, 4> result_i  = waveshaper_saturate_I(input, 0.0);
    const univector<float, 4> result_ii = waveshaper_saturate_II(input, -0.0);
    for (size_t i = 0; i < input.size(); ++i)
    {
        CHECK(result_i[i] == input[i]);
        CHECK(result_ii[i] == input[i]);
    }
}

TEST_CASE("saturate_I_extreme_inputs")
{
    constexpr float tiny     = 0x1p-30f;
    constexpr float infinity = std::numeric_limits<float>::infinity();
    CHECK(saturate_I(tiny) == tiny);
    const auto values = saturate_I(vec<float, 4>{ -tiny, 1.f, -infinity, infinity });
    CHECK(values[0] == -tiny);
    CHECK(values[1] == 0.5f);
    CHECK(values[2] == -1.f);
    CHECK(values[3] == 1.f);
}

TEST_CASE("saturate_II_extreme_inputs")
{
    constexpr float tiny = 0x1p-30f;
    CHECK(saturate_II(tiny) == tiny);
    const auto values = saturate_II(vec<float, 4>{ -tiny, 1.f, -1e30f, 1e30f });
    CHECK(values[0] == -tiny);
    CHECK(values[1] == 0.6f);
    CHECK(values[2] == -1.f);
    CHECK(values[3] == 1.f);
}

TEST_CASE("fir_design_center_tap_uses_window")
{
    univector<float, 3> taps;
    const auto window = to_handle(dimensions<1>(0.5f));
    fir_lowpass(taps, 0.125f, window, false);
    CHECK(taps[1] == 0.125f);
    fir_highpass(taps, 0.125f, window, false);
    CHECK(taps[1] == 0.375f);
    fir_bandpass(taps, 0.125f, 0.25f, window, false);
    CHECK(taps[1] == 0.125f);
    fir_bandstop(taps, 0.125f, 0.25f, window, false);
    CHECK(taps[1] == 0.375f);
}

TEST_CASE("fir_state")
{
    fir_state<float, float> state(univector<float>{ 1, 2, 3, 4, 5, 6 });
    {
        const expression_fir<float, float, expression_dimensions<1, float>, false> stateful(
            dimensions<1>(0.f), state);
        CHECK(&stateful.state->delayline_cursor != &state.delayline_cursor);

        const expression_fir<float, float, expression_dimensions<1, float>, true> stateless(
            dimensions<1>(0.f), std::ref(state));
        CHECK(&stateless.state->delayline_cursor == &state.delayline_cursor);
    }
    {
        auto stateful = fir(dimensions<1>(0.f), state.params);
        CHECK(&stateful.state->delayline_cursor != &state.delayline_cursor);

        auto stateless = fir(dimensions<1>(0.f), std::ref(state));
        CHECK(&stateless.state->delayline_cursor == &state.delayline_cursor);
    }
}

TEST_CASE("short_fir_state_reset")
{
    const univector<float, 3> taps{ 1.f, 2.f, 3.f };
    short_fir_state<3, float, complex<float>> state(taps);
    univector<complex<float>> input{ { 1.f, 2.f }, { 3.f, 4.f } };
    univector<complex<float>> before = short_fir(input, std::ref(state));
    state.reset();
    univector<complex<float>> after = short_fir(input, std::ref(state));
    for (size_t i = 0; i < input.size(); ++i)
        CHECK(after[i] == before[i]);
}

TEST_CASE("fir_state_reset")
{
    const univector<float, 3> taps{ 1.f, 2.f, 3.f };
    fir_state<float, complex<float>> state(taps);
    univector<complex<float>> input{ { 1.f, 2.f }, { 3.f, 4.f } };
    univector<complex<float>> before = fir(input, std::ref(state));
    state.reset();
    CHECK(state.delayline_cursor == 0);
    univector<complex<float>> after = fir(input, std::ref(state));
    for (size_t i = 0; i < input.size(); ++i)
        CHECK(after[i] == before[i]);
}

TEST_CASE("fir_expression_reset")
{
    const univector<float, 3> taps{ 1.f, 2.f, 3.f };
    univector<float, 1> input{ 0.f };

    SECTION("short FIR")
    {
        short_fir_state<3, float> state(taps);
        state.delayline          = vec<float, 2>{ 4.f, 5.f };
        const auto expected_taps = state.taps;

        reset(short_fir(input, std::ref(state)));

        CHECK(state.delayline[0] == 0.f);
        CHECK(state.delayline[1] == 0.f);
        CHECK(state.taps[0] == expected_taps[0]);
        CHECK(state.taps[1] == expected_taps[1]);
        CHECK(state.taps[2] == expected_taps[2]);
    }

    SECTION("generic FIR")
    {
        fir_state<float> state(taps);
        state.delayline          = univector<float>{ 4.f, 5.f, 6.f };
        state.delayline_cursor   = 2;
        const auto expected_taps = state.params.taps;

        reset(fir(input, std::ref(state)));

        CHECK(state.delayline_cursor == 0);
        CHECK(state.delayline[0] == 0.f);
        CHECK(state.delayline[1] == 0.f);
        CHECK(state.delayline[2] == 0.f);
        CHECK(state.params.taps[0] == expected_taps[0]);
        CHECK(state.params.taps[1] == expected_taps[1]);
        CHECK(state.params.taps[2] == expected_taps[2]);
    }

    SECTION("moving sum")
    {
        moving_sum_state<float> state(3);
        state.delayline   = univector<float>{ 4.f, 5.f, 6.f };
        state.head_cursor = 2;
        state.tail_cursor = 0;

        reset(moving_sum(input, std::ref(state)));

        CHECK(state.delayline[0] == 0.f);
        CHECK(state.delayline[1] == 0.f);
        CHECK(state.delayline[2] == 0.f);
        CHECK(state.head_cursor == 0);
        CHECK(state.tail_cursor == 1);
    }
}

TEST_CASE("fir")
{
#ifdef KFR_COMPILER_IS_MSVC
    // test_matrix causes error in MSVC
    {
        using T = float;

        const univector<T, 100> data = counter() + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5);
        const univector<T, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };

        CHECK_EXPRESSION(fir(data, taps), 100,
                         [&](size_t index) -> T
                         {
                             T result = 0;
                             for (size_t i = 0; i < taps.size(); i++)
                                 result += data.get(index - i, 0) * taps[i];
                             return result;
                         });

        CHECK_EXPRESSION(short_fir(data, taps), 100,
                         [&](size_t index) -> T
                         {
                             T result = 0;
                             for (size_t i = 0; i < taps.size(); i++)
                                 result += data.get(index - i, 0) * taps[i];
                             return result;
                         });
    }
    {
        using T = double;

        const univector<T, 100> data = counter() + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5);
        const univector<T, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };

        CHECK_EXPRESSION(fir(data, taps), 100,
                         [&](size_t index) -> T
                         {
                             T result = 0;
                             for (size_t i = 0; i < taps.size(); i++)
                                 result += data.get(index - i, 0) * taps[i];
                             return result;
                         });

        CHECK_EXPRESSION(short_fir(data, taps), 100,
                         [&](size_t index) -> T
                         {
                             T result = 0;
                             for (size_t i = 0; i < taps.size(); i++)
                                 result += data.get(index - i, 0) * taps[i];
                             return result;
                         });
    }
#else
    test_matrix(named("type") = ctypes_t<float
#ifdef KFR_NATIVE_F64
                                         ,
                                         double
#endif
                                         >{},
                [](auto type)
                {
                    using T = typename decltype(type)::type;

                    const univector<T, 100> data =
                        counter() + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5);
                    const univector<T, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };

                    CHECK_EXPRESSION(fir(data, fir_params{ taps }), 100,
                                     [&](size_t index) -> T
                                     {
                                         T result = 0;
                                         for (size_t i = 0; i < taps.size(); i++)
                                             result += data.get(index - i, 0) * taps[i];
                                         return result;
                                     });

                    fir_state<T> state(taps.ref());

                    CHECK_EXPRESSION(fir(data, std::ref(state)), 100,
                                     [&](size_t index) -> T
                                     {
                                         T result = 0;
                                         for (size_t i = 0; i < taps.size(); i++)
                                             result += data.get(index - i, 0) * taps[i];
                                         return result;
                                     });

                    CHECK_EXPRESSION(short_fir(data, taps), 100,
                                     [&](size_t index) -> T
                                     {
                                         T result = 0;
                                         for (size_t i = 0; i < taps.size(); i++)
                                             result += data.get(index - i, 0) * taps[i];
                                         return result;
                                     });

                    short_fir_state<9, T> state2(taps);

                    CHECK_EXPRESSION(short_fir(data, std::ref(state2)), 100,
                                     [&](size_t index) -> T
                                     {
                                         T result = 0;
                                         for (size_t i = 0; i < taps.size(); i++)
                                             result += data.get(index - i, 0) * taps[i];
                                         return result;
                                     });

                    CHECK_EXPRESSION(moving_sum(data, taps.size()), 100,
                                     [&](size_t index) -> T
                                     {
                                         T result = 0;
                                         for (size_t i = 0; i < taps.size(); i++)
                                             result += data.get(index - i, 0);
                                         return result;
                                     });

                    moving_sum_state<T, 131> msstate1;

                    CHECK_EXPRESSION(moving_sum(data, std::ref(msstate1)), 100,
                                     [&](size_t index) -> T
                                     {
                                         T result = 0;
                                         for (size_t i = 0; i < msstate1.delayline.size(); i++)
                                             result += data.get(index - i, 0);
                                         return result;
                                     });

                    moving_sum_state<T> msstate2(133);

                    CHECK_EXPRESSION(moving_sum(data, std::ref(msstate2)), 100,
                                     [&](size_t index) -> T
                                     {
                                         T result = 0;
                                         for (size_t i = 0; i < msstate2.delayline.size(); i++)
                                             result += data.get(index - i, 0);
                                         return result;
                                     });
                });
#endif
}

#ifdef KFR_NATIVE_F64
TEST_CASE("fir_different")
{
    const univector<float, 100> data = counter() + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5f);
    //    const univector<double, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };
    const univector<double, 4> taps{ 1, 2, 3, 4 };

    CHECK_EXPRESSION(fir(data, fir_params{ taps }), 100,
                     [&](size_t index) -> float
                     {
                         double result = 0.0;
                         for (size_t i = 0; i < taps.size(); i++)
                             result += data.get(index - i, 0.0) * taps[i];
                         return float(result);
                     });

    CHECK_EXPRESSION(short_fir(data, taps), 100,
                     [&](size_t index) -> float
                     {
                         double result = 0.0;
                         for (size_t i = 0; i < taps.size(); i++)
                             result += data.get(index - i, 0.0) * taps[i];
                         return float(result);
                     });
}
#endif

TEST_CASE("fir_complex")
{
    const univector<complex<float>, 100> data =
        counter() * complex<float>{ 0.f, 1.f } + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5f);
    const univector<float, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };

    CHECK_EXPRESSION(fir(data, fir_params{ taps }), 100,
                     [&](size_t index) -> complex<float>
                     {
                         std::complex<float> result = 0.0;
                         for (size_t i = 0; i < taps.size(); i++)
                             result = result + data.get(index - i, 0.0) * taps[i];
                         return result;
                     });

    CHECK_EXPRESSION(short_fir(data, taps), 100,
                     [&](size_t index) -> complex<float>
                     {
                         std::complex<float> result = 0.0;
                         for (size_t i = 0; i < taps.size(); i++)
                             result = result + data.get(index - i, 0.0) * taps[i];
                         return result;
                     });
}
} // namespace KFR_ARCH_NAME

} // namespace kfr
