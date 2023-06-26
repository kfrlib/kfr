/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/dsp/units.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(note_to_hertz)
{
    testo::eplison_scope<void> eps(2000);
    CHECK(kfr::note_to_hertz(60) == fbase(261.6255653005986346778499935233));
    CHECK(kfr::note_to_hertz(pack(60)) == pack(fbase(261.6255653005986346778499935233)));

    CHECK(kfr::note_to_hertz(69) == fbase(440.0));
    CHECK(kfr::note_to_hertz(pack(69)) == pack(fbase(440)));
}

TEST(hertz_to_note)
{
    testo::eplison_scope<void> eps(1000);
    CHECK(kfr::hertz_to_note(261.6255653005986346778499935233) == fbase(60));
    CHECK(kfr::hertz_to_note(pack(261.6255653005986346778499935233)) == pack(fbase(60)));

    CHECK(kfr::hertz_to_note(440) == fbase(69));
    CHECK(kfr::hertz_to_note(pack(440)) == pack(fbase(69)));
}

TEST(amp_to_dB)
{
    testo::eplison_scope<void> eps(1000);

    CHECK(kfr::amp_to_dB(fbase(2.0)) == fbase(6.0205999132796239042747778944899));
    CHECK(kfr::amp_to_dB(fbase(-2.0)) == fbase(6.0205999132796239042747778944899));
    CHECK(kfr::amp_to_dB(fbase(1.0)) == fbase(0));
    CHECK(kfr::amp_to_dB(fbase(-1.0)) == fbase(0));
    CHECK(kfr::amp_to_dB(fbase(0.5)) == fbase(-6.0205999132796239042747778944899));
    CHECK(kfr::amp_to_dB(fbase(-0.5)) == fbase(-6.0205999132796239042747778944899));
    CHECK(kfr::amp_to_dB(fbase(0.0)) == fbase(-HUGE_VAL));
}

TEST(dB_to_amp)
{
    testo::eplison_scope<void> eps(1000);

    CHECK(kfr::dB_to_amp(fbase(-HUGE_VAL)) == fbase(0.0));
    CHECK(kfr::dB_to_amp(fbase(0.0)) == fbase(1.0));
    CHECK(kfr::dB_to_amp(fbase(6.0205999132796239042747778944899)) == fbase(2.0));
    CHECK(kfr::dB_to_amp(fbase(-6.0205999132796239042747778944899)) == fbase(0.5));
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
