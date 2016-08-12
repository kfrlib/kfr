/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/io/tostring.hpp>

#include "testo/testo.hpp"
#include <kfr/math.hpp>
#include <kfr/dsp.hpp>

using namespace kfr;

constexpr ctypes_t<i8x1, i8x2, i8x4, i8x8, i8x16, i8x3, //
                   i16x1, i16x2, i16x4, i16x8, i16x16, i16x3, //
                   i32x1, i32x2, i32x4, i32x8, i32x16, i32x3 //
#ifdef KFR_NATIVE_I64
                   ,
                   i64x1, i64x2, i64x4, i64x8, i64x16, i64x3 //
#endif
                   >
    signed_types{};

constexpr ctypes_t<u8x1, u8x2, u8x4, u8x8, u8x16, u8x3, //
                   u16x1, u16x2, u16x4, u16x8, u16x16, u16x3, //
                   u32x1, u32x2, u32x4, u32x8, u32x16, u32x3 //
#ifdef KFR_NATIVE_I64
                   ,
                   u64x1, u64x2, u64x4, u64x8, u64x16, u64x3 //
#endif
                   >
    unsigned_types{};

constexpr ctypes_t<f32x1, f32x2, f32x4, f32x8, f32x16, f32x3 //
#ifdef KFR_NATIVE_F64
                   ,
                   f64x1, f64x2, f64x4, f64x8, f64x16, f64x3 //
#endif
                   >
    float_types{};

constexpr ctypes_t<i8x1, i8x2, i8x4, i8x8, i8x16, i8x3, //
                   i16x1, i16x2, i16x4, i16x8, i16x16, i16x3, //
                   i32x1, i32x2, i32x4, i32x8, i32x16, i32x3, //
#ifdef KFR_NATIVE_I64

                   i64x1, i64x2, i64x4, i64x8, i64x16, i64x3, //
#endif
                   u8x1, u8x2, u8x4, u8x8, u8x16, u8x3, //
                   u16x1, u16x2, u16x4, u16x8, u16x16, u16x3, //
                   u32x1, u32x2, u32x4, u32x8, u32x16, u32x3, //
#ifdef KFR_NATIVE_I64
                   u64x1, u64x2, u64x4, u64x8, u64x16, u64x3, //
#endif
                   f32x1, f32x2, f32x4, f32x8, f32x16, f32x3 //
#ifdef KFR_NATIVE_F64
                   ,
                   f64x1, f64x2, f64x4, f64x8, f64x16, f64x3 //
#endif
                   >
    all_types{};

template <typename T>
inline T ref_abs(T x)
{
    return x >= T(0) ? x : -x;
}

template <typename T>
bool builtin_add_overflow(T x, T y, T* r)
{
#if __has_builtin(__builtin_add_overflow)
    return __builtin_add_overflow(x, y, r);
#else
    *r = x + y;
    return static_cast<long long>(x) + static_cast<long long>(y) != static_cast<long long>(*r);
#endif
}
template <>
bool builtin_add_overflow<u64>(u64 x, u64 y, u64* r)
{
    return __builtin_uaddll_overflow(x, y, reinterpret_cast<unsigned long long*>(r));
}
template <>
bool builtin_add_overflow<i64>(i64 x, i64 y, i64* r)
{
    return __builtin_saddll_overflow(x, y, reinterpret_cast<long long*>(r));
}
template <typename T>
bool builtin_sub_overflow(T x, T y, T* r)
{
#if __has_builtin(__builtin_sub_overflow)
    return __builtin_sub_overflow(x, y, r);
#else
    *r = x - y;
    return static_cast<long long>(x) - static_cast<long long>(y) != static_cast<long long>(*r);
#endif
}
template <>
bool builtin_sub_overflow<u64>(u64 x, u64 y, u64* r)
{
    return __builtin_usubll_overflow(x, y, reinterpret_cast<unsigned long long*>(r));
}
template <>
bool builtin_sub_overflow<i64>(i64 x, i64 y, i64* r)
{
    return __builtin_ssubll_overflow(x, y, reinterpret_cast<long long*>(r));
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

TEST(intrin_abs)
{
    testo::assert_is_same<decltype(kfr::abs(1)), int>();
    testo::assert_is_same<decltype(kfr::abs(1u)), unsigned int>();
    testo::assert_is_same<decltype(kfr::abs(make_vector(1))), i32x1>();
    testo::assert_is_same<decltype(kfr::abs(make_vector(1, 2))), i32x2>();
    CHECK(kfr::abs(9u) == 9u);
    CHECK(kfr::abs(9) == 9);
    CHECK(kfr::abs(-9) == 9);
    CHECK(kfr::abs(-infinity) == infinity);
    CHECK(kfr::abs(make_vector(9)) == make_vector(9));
    CHECK(kfr::abs(make_vector(-9)) == make_vector(9));

    testo::matrix(named("type") = signed_types, named("value") = std::vector<int>{ -1, 0, +1 },
                  [](auto type, int value) {
                      using T    = type_of<decltype(type)>;
                      using Tsub = subtype<T>;
                      const T x(value);
                      CHECK(kfr::abs(x) == apply([](auto x) { return ref_abs(x); }, x));
                  });
    testo::matrix(named("type") = float_types, named("value") = std::vector<int>{ -1, 0, +1 },
                  [](auto type, int value) {
                      using T    = type_of<decltype(type)>;
                      using Tsub = subtype<T>;
                      const T x(value);
                      CHECK(kfr::abs(x) == apply([](auto x) { return Tsub(std::abs(x)); }, x));
                  });
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
    testo::matrix(named("type") = float_types, named("value") = std::vector<int>{ 0, 2, 65536 },
                  [](auto type, int value) {
                      using T    = type_of<decltype(type)>;
                      using Tsub = subtype<T>;
                      const T x(value);
                      CHECK(kfr::sqrt(x) == apply([](auto x) { return std::sqrt(x); }, x));
                  });
}

TEST(intrin_round)
{
    testo::assert_is_same<decltype(kfr::floor(100)), int>();
    testo::assert_is_same<decltype(kfr::ceil(100)), int>();
    testo::assert_is_same<decltype(kfr::round(100)), int>();
    testo::assert_is_same<decltype(kfr::trunc(100)), int>();
    testo::assert_is_same<decltype(kfr::fract(100)), int>();

    testo::assert_is_same<decltype(kfr::ifloor(100.f)), int>();
    testo::assert_is_same<decltype(kfr::iceil(100.f)), int>();
    testo::assert_is_same<decltype(kfr::iround(100.f)), int>();
    testo::assert_is_same<decltype(kfr::itrunc(100.f)), int>();
    CHECK(kfr::floor(100) == 100);
    CHECK(kfr::ceil(100) == 100);
    CHECK(kfr::round(100) == 100);
    CHECK(kfr::trunc(100) == 100);
    CHECK(kfr::fract(100) == 0);

    testo::matrix(named("type")  = float_types,
                  named("value") = std::vector<fbase>{ -1.51, -1.49, 0.0, +1.49, +1.51 },
                  [](auto type, fbase value) {
                      using T    = type_of<decltype(type)>;
                      using Tsub = subtype<T>;
                      const T x(value);
                      CHECK(kfr::floor(x) == apply([](auto x) { return std::floor(x); }, x));
                      CHECK(kfr::ceil(x) == apply([](auto x) { return std::ceil(x); }, x));
                      CHECK(kfr::round(x) == apply([](auto x) { return std::round(x); }, x));
                      CHECK(kfr::trunc(x) == apply([](auto x) { return std::trunc(x); }, x));
                      CHECK(kfr::fract(x) == apply([](auto x) { return x - std::floor(x); }, x));
                  });
}

TEST(intrin_min_max)
{
    testo::assert_is_same<decltype(min(1, 2)), int>();
    testo::assert_is_same<decltype(min(1, 2u)), unsigned int>();
    testo::assert_is_same<decltype(min(1, 2)), int>();
    testo::assert_is_same<decltype(min(pack(1), 2u)), u32x1>();
    testo::assert_is_same<decltype(min(2u, pack(1))), u32x1>();
    testo::assert_is_same<decltype(min(pack(1), pack(2u))), u32x1>();
    testo::assert_is_same<decltype(min(pack(1, 2, 3), pack(1.0, 2.0, 3.0))), f64x3>();
    testo::assert_is_same<decltype(min(pack(1.0, 2.0, 3.0), pack(1, 2, 3))), f64x3>();

    CHECK(min(1, 2) == 1);
    CHECK(min(1, 2u) == 1u);
    CHECK(min(pack(1), 2) == pack(1));
    CHECK(min(pack(1, 2, 3), 2) == pack(1, 2, 2));
    CHECK(min(pack(1., 2., 3.), 2) == pack(1., 2., 2.));

    testo::matrix(named("type")  = float_types,
                  named("value") = std::vector<std::pair<fbase, fbase>>{ { -100, +100 }, { infinity, 0.0 } },
                  [](auto type, std::pair<fbase, fbase> value) {
                      using T    = type_of<decltype(type)>;
                      using Tsub = subtype<T>;
                      const T x(value.first);
                      const T y(value.second);
                      CHECK(kfr::min(x, y) == apply([](auto x, auto y) { return std::min(x, y); }, x, y));
                      CHECK(kfr::max(x, y) == apply([](auto x, auto y) { return std::max(x, y); }, x, y));
                      CHECK(kfr::absmin(x, y) ==
                            apply([](auto x, auto y) { return std::min(ref_abs(x), ref_abs(y)); }, x, y));
                      CHECK(kfr::absmax(x, y) ==
                            apply([](auto x, auto y) { return std::max(ref_abs(x), ref_abs(y)); }, x, y));
                  });
    testo::matrix(named("type")  = signed_types,
                  named("value") = std::vector<std::pair<int, int>>{ { -100, +100 } },
                  [](auto type, std::pair<int, int> value) {
                      using T    = type_of<decltype(type)>;
                      using Tsub = subtype<T>;
                      const T x(value.first);
                      const T y(value.second);
                      CHECK(kfr::min(x, y) == apply([](auto x, auto y) { return std::min(x, y); }, x, y));
                      CHECK(kfr::max(x, y) == apply([](auto x, auto y) { return std::max(x, y); }, x, y));
                      CHECK(kfr::absmin(x, y) ==
                            apply([](auto x, auto y) { return std::min(ref_abs(x), ref_abs(y)); }, x, y));
                      CHECK(kfr::absmax(x, y) ==
                            apply([](auto x, auto y) { return std::max(ref_abs(x), ref_abs(y)); }, x, y));
                  });
    testo::matrix(named("type")  = unsigned_types,
                  named("value") = std::vector<std::pair<unsigned, unsigned>>{ { 0, +200 } },
                  [](auto type, std::pair<unsigned, unsigned> value) {
                      using T    = type_of<decltype(type)>;
                      using Tsub = subtype<T>;
                      const T x(value.first);
                      const T y(value.second);
                      CHECK(kfr::min(x, y) == apply([](auto x, auto y) { return std::min(x, y); }, x, y));
                      CHECK(kfr::max(x, y) == apply([](auto x, auto y) { return std::max(x, y); }, x, y));
                      CHECK(kfr::absmin(x, y) ==
                            apply([](auto x, auto y) { return std::min(ref_abs(x), ref_abs(y)); }, x, y));
                      CHECK(kfr::absmax(x, y) ==
                            apply([](auto x, auto y) { return std::max(ref_abs(x), ref_abs(y)); }, x, y));
                  });
}

TEST(intrin_satadd_satsub)
{
    testo::matrix(named("type") = signed_types, [](auto type) {
        using T     = type_of<decltype(type)>;
        using Tsub  = subtype<T>;
        const T min = std::numeric_limits<Tsub>::min();
        const T max = std::numeric_limits<Tsub>::max();
        CHECK(kfr::satadd(min, min) == apply([](auto x, auto y) { return ref_satadd(x, y); }, min, min));
        CHECK(kfr::satadd(max, max) == apply([](auto x, auto y) { return ref_satadd(x, y); }, max, max));
        CHECK(kfr::satadd(min, max) == apply([](auto x, auto y) { return ref_satadd(x, y); }, min, max));
        CHECK(kfr::satadd(max, min) == apply([](auto x, auto y) { return ref_satadd(x, y); }, max, min));

        CHECK(kfr::satsub(min, min) == apply([](auto x, auto y) { return ref_satsub(x, y); }, min, min));
        CHECK(kfr::satsub(max, max) == apply([](auto x, auto y) { return ref_satsub(x, y); }, max, max));
        CHECK(kfr::satsub(min, max) == apply([](auto x, auto y) { return ref_satsub(x, y); }, min, max));
        CHECK(kfr::satsub(max, min) == apply([](auto x, auto y) { return ref_satsub(x, y); }, max, min));
    });

    testo::matrix(named("type") = unsigned_types, [](auto type) {
        using T      = type_of<decltype(type)>;
        using Tsub   = subtype<T>;
        const T& min = std::numeric_limits<Tsub>::min();
        const T& max = std::numeric_limits<Tsub>::max();
        CHECK(kfr::satadd(min, min) == apply([](auto x, auto y) { return ref_satadd(x, y); }, min, min));
        CHECK(kfr::satadd(max, max) == apply([](auto x, auto y) { return ref_satadd(x, y); }, max, max));
        CHECK(kfr::satadd(min, max) == apply([](auto x, auto y) { return ref_satadd(x, y); }, min, max));
        CHECK(kfr::satadd(max, min) == apply([](auto x, auto y) { return ref_satadd(x, y); }, max, min));

        CHECK(kfr::satsub(min, min) == apply([](auto x, auto y) { return ref_satsub(x, y); }, min, min));
        CHECK(kfr::satsub(max, max) == apply([](auto x, auto y) { return ref_satsub(x, y); }, max, max));
        CHECK(kfr::satsub(min, max) == apply([](auto x, auto y) { return ref_satsub(x, y); }, min, max));
        CHECK(kfr::satsub(max, min) == apply([](auto x, auto y) { return ref_satsub(x, y); }, max, min));
    });
}

TEST(intrin_any_all)
{
    testo::matrix(named("type") = unsigned_types, [](auto type) {
        using T                = type_of<decltype(type)>;
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

TEST(intrin_math)
{
    testo::assert_is_same<decltype(pack(11) * pack(0.5)), f64x1>();
    testo::assert_is_same<decltype(pack(11) * 0.5), f64x1>();
    testo::assert_is_same<decltype(kfr::sin(2)), fbase>();
    testo::assert_is_same<decltype(kfr::sin(pack(2))), vec<fbase, 1>>();
    testo::assert_is_same<decltype(kfr::sindeg(2)), fbase>();
    testo::assert_is_same<decltype(kfr::sindeg(pack(2))), vec<fbase, 1>>();

    CHECK(pack(11) * pack(0.5) == 5.5);
    CHECK(pack(11) * 0.5 == 5.5);
    CHECK(kfr::sin(2) == fbase(0.90929742682568169539601986591174));
    CHECK(kfr::sin(pack(2)) == pack(fbase(0.90929742682568169539601986591174)));
    CHECK(kfr::sindeg(2) == fbase(0.03489949670250097164599518162533));
    CHECK(kfr::sindeg(pack(2)) == pack(fbase(0.03489949670250097164599518162533)));
    CHECK(kfr::cos(2) == fbase(-0.41614683654714238699756822950076));
    CHECK(kfr::cos(pack(2)) == pack(fbase(-0.41614683654714238699756822950076)));
    CHECK(kfr::cosdeg(2) == fbase(0.99939082701909573000624344004393));
    CHECK(kfr::cosdeg(pack(2)) == pack(fbase(0.99939082701909573000624344004393)));

    CHECK(kfr::log(2) == fbase(0.6931471805599453));
    CHECK(kfr::log(pack(2)) == pack(fbase(0.6931471805599453)));
    CHECK(kfr::log2(2) == fbase(1.0));
    CHECK(kfr::log2(pack(2)) == pack(fbase(1.0)));
    CHECK(kfr::log10(2) == fbase(0.30102999566398119521373889472449));
    CHECK(kfr::log10(pack(2)) == pack(fbase(0.30102999566398119521373889472449)));

    CHECK(kfr::exp(2) == fbase(7.3890560989306502));
    CHECK(kfr::exp(pack(2)) == pack(fbase(7.3890560989306502)));
    CHECK(kfr::exp2(2) == fbase(4.0));
    CHECK(kfr::exp2(pack(2)) == pack(fbase(4.0)));

    CHECK(kfr::logn(2, 10) == fbase(0.30102999566398119521373889472449));
    CHECK(kfr::logn(pack(2), pack(10)) == pack(fbase(0.30102999566398119521373889472449)));

    CHECK(kfr::pow(2, fbase(0.9)) == fbase(1.8660659830736148319626865322999));
    CHECK(kfr::pow(pack(2), pack(fbase(0.9))) == pack(fbase(1.8660659830736148319626865322999)));

    CHECK(kfr::root(fbase(1.5), 2) == fbase(1.2247448713915890490986420373529));
    CHECK(kfr::root(pack(fbase(1.5)), pack(2)) == pack(fbase(1.2247448713915890490986420373529)));

    testo::epsilon<float>() *= 10.0;
    testo::epsilon<double>() *= 10.0;

    CHECK(kfr::sinh(2) == fbase(3.6268604078470187676682139828013));
    CHECK(kfr::sinh(pack(2)) == pack(fbase(3.6268604078470187676682139828013)));
    CHECK(kfr::cosh(2) == fbase(3.7621956910836314595622134777737));
    CHECK(kfr::cosh(pack(2)) == pack(fbase(3.7621956910836314595622134777737)));

    CHECK(kfr::tanh(2) == fbase(0.96402758007581688394641372410092));
    CHECK(kfr::tanh(pack(2)) == pack(fbase(0.96402758007581688394641372410092)));
    CHECK(kfr::coth(2) == fbase(1.0373147207275480958778097647678));
    CHECK(kfr::coth(pack(2)) == pack(fbase(1.0373147207275480958778097647678)));

    testo::epsilon<float>() *= 10.0;
    testo::epsilon<double>() *= 10.0;

    CHECK(kfr::tan(2) == fbase(-2.1850398632615189916433061023137));
    CHECK(kfr::tan(pack(2)) == pack(fbase(-2.1850398632615189916433061023137)));
    CHECK(kfr::tandeg(2) == fbase(0.03492076949174773050040262577373));
    CHECK(kfr::tandeg(pack(2)) == pack(fbase(0.03492076949174773050040262577373)));

    testo::epsilon<float>() *= 10.0;
    testo::epsilon<double>() *= 10.0;

    CHECK(kfr::note_to_hertz(60) == fbase(261.6255653005986346778499935233));
    CHECK(kfr::note_to_hertz(pack(60)) == pack(fbase(261.6255653005986346778499935233)));
}

int main(int argc, char** argv) { return testo::run_all("", false); }
