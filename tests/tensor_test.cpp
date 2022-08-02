#include "kfr/base.hpp"
#include "kfr/testo/testo.hpp"

using namespace kfr;

#ifndef KFR_NO_MAIN
int main()
{
    println(library_version(), " running on ", cpu_runtime());

    return testo::run_all("", false);
}
#endif
