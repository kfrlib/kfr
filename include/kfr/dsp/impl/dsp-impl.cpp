#include "../biquad.hpp"
#include "../fir.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
template <typename U, typename T>
filter<U>* make_fir_filter(const univector_ref<const T>& taps)
{
    return new fir_filter<T, U>(taps);
}

template filter<float>* make_fir_filter<float, float>(const univector_ref<const float>&);
template filter<double>* make_fir_filter<double, double>(const univector_ref<const double>&);
template filter<float>* make_fir_filter<float, double>(const univector_ref<const double>&);

template <typename T, size_t maxfiltercount>
KFR_FUNCTION filter<T>* make_biquad_filter(const biquad_params<T>* bq, size_t count)
{
    return new biquad_filter<T, maxfiltercount>(bq, count);
}

template filter<float>* make_biquad_filter<float, 64>(const biquad_params<float>* bq, size_t count);
template filter<double>* make_biquad_filter<double, 64>(const biquad_params<double>* bq, size_t count);

} // namespace CMT_ARCH_NAME
} // namespace kfr