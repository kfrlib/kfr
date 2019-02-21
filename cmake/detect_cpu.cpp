#include <kfr/runtime/cpuid.hpp>

using namespace kfr;

int main()
{
    cpu_t cpu = kfr::internal_generic::detect_cpu();
    printf("%s", cpu_name(cpu));
}