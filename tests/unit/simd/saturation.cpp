/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/saturation.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T>
bool builtin_add_overflow(T x, T y, T* r)
{
#if CMT_HAS_BUILTIN(__builtin_add_overflow) || defined CMT_COMPILER_GCC
    return __builtin_add_overflow(x, y, r);
#else
    *r = x + y;
    return static_cast<long long>(x) + static_cast<long long>(y) != static_cast<long long>(*r);
#endif
}
template <>
bool builtin_add_overflow<u64>(u64 x, u64 y, u64* r)
{
#if CMT_HAS_BUILTIN(__builtin_uaddll_overflow) || defined CMT_COMPILER_GCC
    return __builtin_uaddll_overflow(x, y, reinterpret_cast<unsigned long long*>(r));
#else
    *r = x + y;
    return x > 0xFFFFFFFFFFFFFFFFull - y;
#endif
}
template <>
bool builtin_add_overflow<i64>(i64 x, i64 y, i64* r)
{
#if CMT_HAS_BUILTIN(__builtin_saddll_overflow) || defined CMT_COMPILER_GCC
    return __builtin_saddll_overflow(x, y, reinterpret_cast<long long*>(r));
#else
    *r = x + y;
    return !((x ^ y) & 0x8000000000000000ull) && ((*r ^ x) & 0x8000000000000000ull);
#endif
}
template <typename T>
bool builtin_sub_overflow(T x, T y, T* r)
{
#if CMT_HAS_BUILTIN(__builtin_sub_overflow) || defined CMT_COMPILER_GCC
    return __builtin_sub_overflow(x, y, r);
#else
    *r = x - y;
    return static_cast<long long>(x) - static_cast<long long>(y) != static_cast<long long>(*r);
#endif
}
template <>
bool builtin_sub_overflow<u64>(u64 x, u64 y, u64* r)
{
#if CMT_HAS_BUILTIN(__builtin_usubll_overflow) || defined CMT_COMPILER_GCC
    return __builtin_usubll_overflow(x, y, reinterpret_cast<unsigned long long*>(r));
#else
    *r = x - y;
    return x < y;
#endif
}
template <>
bool builtin_sub_overflow<i64>(i64 x, i64 y, i64* r)
{
#if CMT_HAS_BUILTIN(__builtin_ssubll_overflow) || defined CMT_COMPILER_GCC
    return __builtin_ssubll_overflow(x, y, reinterpret_cast<long long*>(r));
#else
    *r = x - y;
    return ((x ^ y) & 0x8000000000000000ull) && ((*r ^ x) & 0x8000000000000000ull);
#endif
}
template <typename T>
inline T ref_satadd(T x, T y)
{
    T result;
    if (builtin_add_overflow(x, y, &result))
        return x < 0 ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
    else
        return result;
}

template <typename T>
inline T ref_satsub(T x, T y)
{
    T result;
    if (builtin_sub_overflow(x, y, &result))
        return x < y ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
    else
        return result;
}

TEST(intrin_satadd_satsub)
{
    testo::matrix(named("type") = cconcat(signed_vector_types<vec>, unsigned_vector_types<vec>),
                  [](auto type)
                  {
                      using T     = typename decltype(type)::type;
                      using Tsub  = subtype<T>;
                      const T min = std::numeric_limits<Tsub>::min();
                      const T max = std::numeric_limits<Tsub>::max();
                      CHECK(kfr::satadd(min, min) ==
                            apply([](auto x, auto y) -> decltype(x) { return ref_satadd(x, y); }, min, min));
                      CHECK(kfr::satadd(max, max) ==
                            apply([](auto x, auto y) -> decltype(x) { return ref_satadd(x, y); }, max, max));
                      CHECK(kfr::satadd(min, max) ==
                            apply([](auto x, auto y) -> decltype(x) { return ref_satadd(x, y); }, min, max));
                      CHECK(kfr::satadd(max, min) ==
                            apply([](auto x, auto y) -> decltype(x) { return ref_satadd(x, y); }, max, min));

                      CHECK(kfr::satsub(min, min) ==
                            apply([](auto x, auto y) -> decltype(x) { return ref_satsub(x, y); }, min, min));
                      CHECK(kfr::satsub(max, max) ==
                            apply([](auto x, auto y) -> decltype(x) { return ref_satsub(x, y); }, max, max));
                      CHECK(kfr::satsub(min, max) ==
                            apply([](auto x, auto y) -> decltype(x) { return ref_satsub(x, y); }, min, max));
                      CHECK(kfr::satsub(max, min) ==
                            apply([](auto x, auto y) -> decltype(x) { return ref_satsub(x, y); }, max, min));
                  });
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
