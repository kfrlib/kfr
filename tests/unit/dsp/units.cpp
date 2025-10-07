/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2025 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/dsp/units.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

TEST_CASE("note_to_hertz")
{
    epsilon_scope<void> eps(2000);
    CHECK_THAT((kfr::note_to_hertz(60)), DeepMatcher(fbase(261.6255653005986346778499935233)));
    CHECK_THAT((kfr::note_to_hertz(pack(60))), DeepMatcher(pack(fbase(261.6255653005986346778499935233))));

    CHECK_THAT((kfr::note_to_hertz(69)), DeepMatcher(fbase(440.0)));
    CHECK_THAT((kfr::note_to_hertz(pack(69))), DeepMatcher(pack(fbase(440))));
}

TEST_CASE("hertz_to_note")
{
    epsilon_scope<void> eps(1000);
    CHECK_THAT((kfr::hertz_to_note(261.6255653005986346778499935233)), DeepMatcher(fbase(60)));
    CHECK_THAT((kfr::hertz_to_note(pack(261.6255653005986346778499935233))), DeepMatcher(pack(fbase(60))));

    CHECK_THAT((kfr::hertz_to_note(440)), DeepMatcher(fbase(69)));
    CHECK_THAT((kfr::hertz_to_note(pack(440))), DeepMatcher(pack(fbase(69))));
}

TEST_CASE("amp_to_dB")
{
    epsilon_scope<void> eps(1000);

    CHECK_THAT((kfr::amp_to_dB(fbase(2.0))), DeepMatcher(fbase(6.0205999132796239042747778944899)));
    CHECK_THAT((kfr::amp_to_dB(fbase(-2.0))), DeepMatcher(fbase(6.0205999132796239042747778944899)));
    CHECK_THAT((kfr::amp_to_dB(fbase(1.0))), DeepMatcher(fbase(0)));
    CHECK_THAT((kfr::amp_to_dB(fbase(-1.0))), DeepMatcher(fbase(0)));
    CHECK_THAT((kfr::amp_to_dB(fbase(0.5))), DeepMatcher(fbase(-6.0205999132796239042747778944899)));
    CHECK_THAT((kfr::amp_to_dB(fbase(-0.5))), DeepMatcher(fbase(-6.0205999132796239042747778944899)));
    CHECK_THAT((kfr::amp_to_dB(fbase(0.0))), DeepMatcher(fbase(-HUGE_VAL)));
}

TEST_CASE("dB_to_amp")
{
#if defined __clang__ && defined(KFR_ARCH_ARM) && __clang_major__ >= 13
    // Clang 13+ compiler bug on ARM
#else
    epsilon_scope<void> eps(1000);

    CHECK_THAT((kfr::exp(fbase(-HUGE_VAL))), DeepMatcher(fbase(0.0)));
    CHECK_THAT((kfr::exp2(fbase(-HUGE_VAL))), DeepMatcher(fbase(0.0)));
    CHECK_THAT((kfr::exp10(fbase(-HUGE_VAL))), DeepMatcher(fbase(0.0)));

    CHECK_THAT((kfr::dB_to_amp(fbase(-HUGE_VAL))), DeepMatcher(fbase(0.0)));
    CHECK_THAT((kfr::dB_to_amp(fbase(0.0))), DeepMatcher(fbase(1.0)));
    CHECK_THAT((kfr::dB_to_amp(fbase(6.0205999132796239042747778944899))), DeepMatcher(fbase(2.0)));
    CHECK_THAT((kfr::dB_to_amp(fbase(-6.0205999132796239042747778944899))), DeepMatcher(fbase(0.5)));
#endif
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
