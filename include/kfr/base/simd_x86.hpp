#pragma once

#include "constants.hpp"
#include "platform.hpp"
#include "simd_intrin.hpp"
namespace kfr
{
#ifdef CMT_ARCH_SSE2

template <>
KFR_I_CE CMT_INLINE vec<f32, 4> vec<f32, 4>::operator+(const vec<f32, 4>& y) const noexcept
{
    return _mm_add_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 4> vec<f32, 4>::operator-(const vec<f32, 4>& y) const noexcept
{
    return _mm_sub_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 4> vec<f32, 4>::operator*(const vec<f32, 4>& y) const noexcept
{
    return _mm_mul_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 4> vec<f32, 4>::operator/(const vec<f32, 4>& y) const noexcept
{
    return _mm_div_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 4> vec<f32, 4>::operator&(const vec<f32, 4>& y) const noexcept
{
    return _mm_and_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 4> vec<f32, 4>::operator|(const vec<f32, 4>& y) const noexcept
{
    return _mm_or_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 4> vec<f32, 4>::operator^(const vec<f32, 4>& y) const noexcept
{
    return _mm_xor_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 2> vec<f64, 2>::operator+(const vec<f64, 2>& y) const noexcept
{
    return _mm_add_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 2> vec<f64, 2>::operator-(const vec<f64, 2>& y) const noexcept
{
    return _mm_sub_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 2> vec<f64, 2>::operator*(const vec<f64, 2>& y) const noexcept
{
    return _mm_mul_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 2> vec<f64, 2>::operator/(const vec<f64, 2>& y) const noexcept
{
    return _mm_div_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 2> vec<f64, 2>::operator&(const vec<f64, 2>& y) const noexcept
{
    return _mm_and_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 2> vec<f64, 2>::operator|(const vec<f64, 2>& y) const noexcept
{
    return _mm_or_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 2> vec<f64, 2>::operator^(const vec<f64, 2>& y) const noexcept
{
    return _mm_xor_pd(simd, y.simd);
}

#endif // CMT_ARCH_SSE2

#ifdef CMT_ARCH_AVX

template <>
KFR_I_CE CMT_INLINE vec<f32, 8> vec<f32, 8>::operator+(const vec<f32, 8>& y) const noexcept
{
    return _mm256_add_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 8> vec<f32, 8>::operator-(const vec<f32, 8>& y) const noexcept
{
    return _mm256_sub_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 8> vec<f32, 8>::operator*(const vec<f32, 8>& y) const noexcept
{
    return _mm256_mul_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 8> vec<f32, 8>::operator/(const vec<f32, 8>& y) const noexcept
{
    return _mm256_div_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 8> vec<f32, 8>::operator&(const vec<f32, 8>& y) const noexcept
{
    return _mm256_and_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 8> vec<f32, 8>::operator|(const vec<f32, 8>& y) const noexcept
{
    return _mm256_or_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f32, 8> vec<f32, 8>::operator^(const vec<f32, 8>& y) const noexcept
{
    return _mm256_xor_ps(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 4> vec<f64, 4>::operator+(const vec<f64, 4>& y) const noexcept
{
    return _mm256_add_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 4> vec<f64, 4>::operator-(const vec<f64, 4>& y) const noexcept
{
    return _mm256_sub_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 4> vec<f64, 4>::operator*(const vec<f64, 4>& y) const noexcept
{
    return _mm256_mul_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 4> vec<f64, 4>::operator/(const vec<f64, 4>& y) const noexcept
{
    return _mm256_div_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 4> vec<f64, 4>::operator&(const vec<f64, 4>& y) const noexcept
{
    return _mm256_and_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 4> vec<f64, 4>::operator|(const vec<f64, 4>& y) const noexcept
{
    return _mm256_or_pd(simd, y.simd);
}

template <>
KFR_I_CE CMT_INLINE vec<f64, 4> vec<f64, 4>::operator^(const vec<f64, 4>& y) const noexcept
{
    return _mm256_xor_pd(simd, y.simd);
}

#endif // CMT_ARCH_AVX

} // namespace kf
