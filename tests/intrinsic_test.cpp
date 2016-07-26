/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/io/tostring.hpp>

#include "testo/testo.hpp"
#include <kfr/math.hpp>
#include <kfr/vec.hpp>

using namespace kfr;

template <typename Fn>
void test_intrinsic(Fn&& fn)
{
}

constexpr ctypes_t<i8x1, i16x1, i32x1, i64x1, //
                   i8x2, i16x2, i32x2, i64x2, //
                   i8x4, i16x4, i32x4, i64x4, //
                   i8x8, i16x8, i32x8, i64x8, //
                   i8x16, i16x16, i32x16, i64x16, //
                   i8x3, i16x3, i32x3, i64x3 //
                   >
    signed_types{};

constexpr ctypes_t<u8x1, u16x1, u32x1, u64x1, //
                   u8x2, u16x2, u32x2, u64x2, //
                   u8x4, u16x4, u32x4, u64x4, //
                   u8x8, u16x8, u32x8, u64x8, //
                   u8x16, u16x16, u32x16, u64x16, //
                   u8x3, u16x3, u32x3, u64x3 //
                   >
    unsigned_types{};

constexpr ctypes_t<f32x1, f64x1, //
                   f32x2, f64x2, //
                   f32x4, f64x4, //
                   f32x8, f64x8, //
                   f32x16, f64x16, //
                   f32x3, f64x3 //
                   >
    float_types{};

constexpr ctypes_t<u8x1, i8x1, u16x1, i16x1, u32x1, i32x1, u64x1, i64x1, f32x1, f64x1, //
                   u8x2, i8x2, u16x2, i16x2, u32x2, i32x2, u64x2, i64x2, f32x2, f64x2, //
                   u8x4, i8x4, u16x4, i16x4, u32x4, i32x4, u64x4, i64x4, f32x4, f64x4, //
                   u8x8, i8x8, u16x8, i16x8, u32x8, i32x8, u64x8, i64x8, f32x8, f64x8, //
                   u8x16, i8x16, u16x16, i16x16, u32x16, i32x16, u64x16, i64x16, f32x16, f64x16, //
                   u8x3, i8x3, u16x3, i16x3, u32x3, i32x3, u64x3, i64x3, f32x3, f64x3 //
                   >
    all_types{};

template <typename T>
inline T ref_abs(T x)
{
    return x >= T(0) ? x : -x;
}

template <typename T>
inline T ref_satadd(T x, T y)
{
    T result;
    if (__builtin_add_overflow(x, y, &result))
        return x < 0 ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
    else
        return result;
}

template <typename T>
inline T ref_satsub(T x, T y)
{
    T result;
    if (__builtin_sub_overflow(x, y, &result))
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
    testo::assert_is_same<decltype(kfr::sqrt(9)), double>();
    testo::assert_is_same<decltype(kfr::sqrt(make_vector(9))), f64x1>();
    testo::assert_is_same<decltype(kfr::sqrt(make_vector(9, 25))), f64x2>();
    CHECK(kfr::sqrt(9) == 3.0);
    CHECK(kfr::sqrt(-9) == qnan);
    CHECK(kfr::sqrt(make_vector(9)) == make_vector(3.0));
    CHECK(kfr::sqrt(make_vector(-9)) == make_vector(qnan));
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
    CHECK(kfr::floor(100) == 100);
    CHECK(kfr::ceil(100) == 100);
    CHECK(kfr::round(100) == 100);
    CHECK(kfr::trunc(100) == 100);
    CHECK(kfr::fract(100) == 0);

    testo::matrix(named("type")  = float_types,
                  named("value") = std::vector<double>{ -1.51, -1.49, 0.0, +1.49, +1.51 },
                  [](auto type, double value) {
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
    testo::matrix(named("type") = float_types,
                  named("value") =
                      std::vector<std::pair<double, double>>{ { -100, +100 }, { infinity, 0.0 } },
                  [](auto type, std::pair<double, double> value) {
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

int main(int argc, char** argv) { return testo::run_all("", false); }
