/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/dsp/delay.hpp>

using namespace kfr;

namespace CMT_ARCH_NAME
{

TEST(delay)
{
    const univector<float, 33> v1 = counter() + 100;
    CHECK_EXPRESSION(delay(v1), 33, [](size_t i) { return i < 1 ? 0.f : (i - 1) + 100.f; });

    CHECK_EXPRESSION(delay<3>(v1), 33, [](size_t i) { return i < 3 ? 0.f : (i - 3) + 100.f; });

    delay_state<float, 3> state1;
    CHECK_EXPRESSION(delay(state1, v1), 33, [](size_t i) { return i < 3 ? 0.f : (i - 3) + 100.f; });

    delay_state<float, 3, tag_dynamic_vector> state2;
    CHECK_EXPRESSION(delay(state2, v1), 33, [](size_t i) { return i < 3 ? 0.f : (i - 3) + 100.f; });
}

TEST(fracdelay)
{
    univector<double, 5> a({ 1, 2, 3, 4, 5 });
    univector<double, 5> b = fracdelay(a, 0.5);
    CHECK(rms(b - univector<double>({ 0.5, 1.5, 2.5, 3.5, 4.5 })) < constants<double>::epsilon * 5);

    b = fracdelay(a, 0.1);
    CHECK(rms(b - univector<double>({ 0.9, 1.9, 2.9, 3.9, 4.9 })) < constants<double>::epsilon * 5);

    b = fracdelay(a, 0.0);
    CHECK(rms(b - univector<double>({ 1, 2, 3, 4, 5 })) < constants<double>::epsilon * 5);

    b = fracdelay(a, 1.0);
    CHECK(rms(b - univector<double>({ 0, 1, 2, 3, 4 })) < constants<double>::epsilon * 5);
}

} // namespace CMT_ARCH_NAME
