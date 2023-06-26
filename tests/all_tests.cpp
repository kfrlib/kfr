/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/io/tostring.hpp>
#include <kfr/testo/testo.hpp>
#include <kfr/version.hpp>
#ifdef HAVE_MPFR
#include "mpfr/mpfrplus.hpp"
#endif

using namespace kfr;

#ifdef KFR_MULTI_ARCH

#define FORCE_LINK(arch)                                                                                     \
    namespace arch                                                                                           \
    {                                                                                                        \
    extern void force_link();                                                                                \
    void (*p)() = &force_link;                                                                               \
    }

FORCE_LINK(sse2)
FORCE_LINK(sse3)
FORCE_LINK(ssse3)
FORCE_LINK(sse41)
FORCE_LINK(avx)
FORCE_LINK(avx2)
// FORCE_LINK(avx512)
#endif

int main()
{
    println(library_version(), " running on ", cpu_runtime());
    try
    {
#ifdef HAVE_MPFR
        mpfr::scoped_precision p(64);
#endif
        return testo::run_all("");
    }
    catch (const std::exception& e)
    {
        errorln("****************************************");
        errorln("***** Exception: ", typeid(e).name(), ": ", e.what());
        return -1;
    }
    catch (...)
    {
        errorln("****************************************");
        errorln("***** Exception: unknown");
        return -1;
    }
}
