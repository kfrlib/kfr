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
