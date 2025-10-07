/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2025 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/test/test.hpp>

#include <kfr/base.hpp>
#include <kfr/dsp/dcremove.hpp>
#include <kfr/audio.hpp>

using namespace kfr;

namespace KFR_ARCH_NAME
{

TEST_CASE("dcremove")
{
    univector<fbase> orig   = truncate(sin(linspace(0, 4800, 48000)), 48000) * 0.5f;
    univector<fbase> withdc = orig + 0.5f;
    univector<fbase> v2     = dcremove(withdc, 10, 48000);
    CHECK(rms(slice(v2, 24000) - slice(orig, 24000)) < 0.01f);
}

} // namespace KFR_ARCH_NAME
