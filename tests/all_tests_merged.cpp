#include <kfr/cident.h>

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wparentheses")

#include "auto_test.cpp"

#include "base_test.cpp"
#include "complex_test.cpp"
#include "dsp_test.cpp"
#include "expression_test.cpp"
#include "intrinsic_test.cpp"
#include "io_test.cpp"
#include "resampler_test.cpp"

#ifndef KFR_NO_DFT
#include "dft_test.cpp"
#endif

namespace CMT_ARCH_NAME
{
void force_link() {}
} // namespace CMT_ARCH_NAME

CMT_PRAGMA_GNU(GCC diagnostic pop)
