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
    if (get_cpu() < cpu_t::native)
    {
        println("CPU is not supported");
        return -1;
    }
#ifdef HAVE_MPFR
    mpfr::scoped_precision p(128);
#endif
    return testo::run_all("");
}
