#include <kfr/cident.h>
#if !defined KFR_SKIP_IF_NON_X86 || defined(KFR_ARCH_X86)

#include <kfr/kfr.h>

namespace kfr
{
const char* library_version_dsp() { return KFR_VERSION_FULL; }
} // namespace kfr

#endif
