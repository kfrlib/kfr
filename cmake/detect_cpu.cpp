#include <kfr/runtime/cpuid.hpp>

using namespace kfr;

int main()
{
#ifdef CPU_target
    cpu_t cpu = cpu_t::native;
#else
    cpu_t cpu = kfr::internal_generic::detect_cpu();
#endif
    printf("%s", cpu_name(cpu));
}
