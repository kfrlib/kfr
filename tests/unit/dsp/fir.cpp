/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <complex>
#include <kfr/dsp/fir.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

TEST(fir_state)
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

TEST(fir)
{
#ifdef KFR_COMPILER_IS_MSVC
    // testo::matrix causes error in MSVC
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
    testo::matrix(named("type") = ctypes_t<float
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
TEST(fir_different)
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

TEST(fir_complex)
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
