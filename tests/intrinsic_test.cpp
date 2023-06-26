/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/io.hpp>

using namespace kfr;

namespace CMT_ARCH_NAME
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
//#endif
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

TEST(intrin_sqrt)
{
    testo::assert_is_same<decltype(kfr::sqrt(9)), fbase>();
    testo::assert_is_same<decltype(kfr::intrinsics::sqrt(9)), fbase>();
    testo::assert_is_same<decltype(kfr::sqrt(make_vector(9))), vec<fbase, 1>>();
    testo::assert_is_same<decltype(kfr::sqrt(make_vector(9, 25))), vec<fbase, 2>>();
    CHECK(kfr::sqrt(9) == fbase(3.0));
    CHECK(kfr::sqrt(2) == fbase(1.4142135623730950488));
    CHECK(kfr::sqrt(-9) == fbase(qnan));
    CHECK(kfr::sqrt(make_vector(9)) == make_vector<fbase>(3.0));
    CHECK(kfr::sqrt(make_vector(-9)) == make_vector<fbase>(qnan));
    testo::matrix(named("type") = float_vector_types<vec>, named("value") = std::vector<int>{ 0, 2, 65536 },
                  [](auto type, int value)
                  {
                      using T = typename decltype(type)::type;
                      const T x(value);
                      CHECK(kfr::sqrt(x) == apply([](auto x) -> decltype(x) { return std::sqrt(x); }, x));
                  });
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

TEST(intrin_any_all)
{
    testo::matrix(named("type") = unsigned_vector_types<vec>,
                  [](auto type)
                  {
                      using T                = typename decltype(type)::type;
                      constexpr size_t width = widthof<T>();
                      using Tsub             = subtype<T>;
                      const auto x           = enumerate<Tsub, width>() == Tsub(0);
                      CHECK(any(x) == true);
                      if (width == 1)
                          CHECK(all(x) == true);
                      else
                          CHECK(all(x) == false);
                      const auto y = zerovector<Tsub, width>() == Tsub(127);
                      CHECK(all(y) == false);
                      CHECK(any(y) == false);
                      const auto z = zerovector<Tsub, width>() == Tsub(0);
                      CHECK(all(z) == true);
                      CHECK(any(z) == true);
                  });
}

} // namespace CMT_ARCH_NAME

#ifndef KFR_NO_MAIN
int main()
{
    println(library_version(), " running on ", cpu_runtime());
    return testo::run_all("", false);
}
#endif
