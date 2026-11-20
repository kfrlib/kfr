/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/base/basic_expressions.hpp>
#include <kfr/base/generators.hpp>
#include <kfr/base/math_expressions.hpp>
#include <kfr/base/reduce.hpp>
#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/univector.hpp>
#include <kfr/dsp/dcremove.hpp>
#include <kfr/dsp/oscillators.hpp>
#include <kfr/math/sin_cos.hpp>

using namespace kfr;

namespace KFR_ARCH_NAME
{

TEST_CASE("dcremove")
{
    univector<fbase> orig   = truncate(sin(linspace(0, 4800, 48000)), 48000) * 0.5f;
    univector<fbase> withdc = orig + 0.5f;
    univector<fbase> v2     = dcremove(withdc, 10, 48000);
    CHECK(rms(slice(v2, 24000) - slice(orig, 24000)) < 0.01f);

    iir_state<fbase, 1> state{ dcremove_params<fbase>(10, 48000) };
    univector<fbase> streamed(withdc.size());
    auto first = streamed.slice(0, 137);
    first = iir(withdc.slice(0, 137), std::ref(state));
    auto rest = streamed.slice(137);
    rest = iir(withdc.slice(137), std::ref(state));
    CHECK(absmaxof(streamed - v2) < 1e-6f);
}

} // namespace KFR_ARCH_NAME
