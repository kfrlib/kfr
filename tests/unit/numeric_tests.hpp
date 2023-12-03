/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/io.hpp>
#include <kfr/testo/testo.hpp>

namespace kfr
{

inline bool show_measured_accuracy = false;

using testo::test_data_entry;

inline namespace CMT_ARCH_NAME
{

using vector_types =
    ctypes_t<f32, f32x1, f32x2, f32x4, f32x8, f32x16, f32x32, f64, f64x1, f64x2, f64x4, f64x8, f64x16>;

template <typename T>
uint64_t ulps(T x, T y)
{
    if (std::abs(x) < std::numeric_limits<T>::min() && std::abs(y) < std::numeric_limits<T>::min())
        return 0;
    if (std::isnan(x) && std::isnan(y))
        return 0;
    if (std::isinf(x) && std::isinf(y))
        return (x < 0) == (y < 0) ? 0 : ULLONG_MAX;
    if (x < 0 && y < 0)
        return ulps<T>(-x, -y);
    if ((x < 0) != (y < 0))
        return ulps<T>(std::abs(x), 0) + ulps<T>(std::abs(y), 0);

    utype<T> ix = cometa::bitcast_anything<utype<T>>(x);
    utype<T> iy = cometa::bitcast_anything<utype<T>>(y);
    if (std::abs(x) < std::numeric_limits<T>::min() && y > std::numeric_limits<T>::min())
        return 1 + ulps<T>(std::numeric_limits<T>::min(), y);
    if (std::abs(x) > std::numeric_limits<T>::min() && y < std::numeric_limits<T>::min())
        return 1 + ulps<T>(x, std::numeric_limits<T>::min());
    return ix > iy ? ix - iy : iy - ix;
}

template <typename T, size_t N>
uint64_t ulps(vec<T, N> x, vec<T, N> y)
{
    uint64_t u = 0;
    for (size_t i = 0; i < N; i++)
    {
        u = std::max(u, ulps(x[i], y[i]));
    }
    return u;
}

inline const char* tname(ctype_t<f32>) { return "float"; }
inline const char* tname(ctype_t<f64>) { return "double"; }

#define CHECK_DIFF(x_arg, y_arg, threshold, file, line)                                                      \
    do                                                                                                       \
    {                                                                                                        \
        ++checks_count;                                                                                      \
        const auto x_arg_value = x_arg;                                                                      \
        const auto y_arg_value = y_arg;                                                                      \
        const auto arg_diff    = ulps(x_arg_value, y_arg_value);                                             \
        error_sum += arg_diff;                                                                               \
        error_peak = std::max(error_peak, arg_diff);                                                         \
        ::testo::active_test()->check(                                                                       \
            arg_diff <= threshold,                                                                           \
            ::cometa::as_string(x_arg_value, " ~= ", y_arg_value, " (", arg_diff, " <= ", threshold, ")"),   \
            #x_arg " ~= " #y_arg, file, line);                                                               \
    } while (0)

#define KFR_AUTO_TEST_1(fn, datafile, maxulps, avgulps)                                                      \
    TEST(fn##_##datafile)                                                                                    \
    {                                                                                                        \
        testo::matrix(named("type") = vector_types(),                                                        \
                      [&](auto type)                                                                         \
                      {                                                                                      \
                          using T               = typename decltype(type)::type;                             \
                          using Tsub            = subtype<T>;                                                \
                          double error_sum      = 0.0;                                                       \
                          uint64_t error_peak   = 0;                                                         \
                          uint64_t checks_count = 0;                                                         \
                          std::shared_ptr<file_reader<test_data_entry<Tsub, 1>>> reader =                    \
                              open_file_for_reading<test_data_entry<Tsub, 1>>(                               \
                                  std::string(KFR_SRC_DIR "/tests/data/" #fn "_") +                          \
                                  tname(cometa::ctype<Tsub>) + "_" #datafile);                               \
                          test_data_entry<Tsub, 1> entry;                                                    \
                          while (reader->read(entry))                                                        \
                          {                                                                                  \
                              testo::scope s(as_string(entry.arguments[0]));                                 \
                              CHECK_DIFF(kfr::fn(entry.arguments[0]), entry.result, maxulps, __FILE__,       \
                                         __LINE__);                                                          \
                          }                                                                                  \
                          CHECK(checks_count > 0u);                                                          \
                          CHECK(error_sum / checks_count <= avgulps);                                        \
                          if (show_measured_accuracy)                                                        \
                              println("measured accuracy: ", tname(cometa::ctype<Tsub>), " ",                \
                                      error_sum / checks_count, "(peak ", error_peak, ")");                  \
                      });                                                                                    \
    }

#define KFR_AUTO_TEST_2(fn, datafile, maxulps, avgulps)                                                      \
    TEST(fn##_##datafile)                                                                                    \
    {                                                                                                        \
        testo::matrix(named("type") = vector_types(),                                                        \
                      [&](auto type)                                                                         \
                      {                                                                                      \
                          using T               = typename decltype(type)::type;                             \
                          using Tsub            = subtype<T>;                                                \
                          double error_sum      = 0.0;                                                       \
                          uint64_t error_peak   = 0;                                                         \
                          uint64_t checks_count = 0;                                                         \
                          std::shared_ptr<file_reader<test_data_entry<Tsub, 2>>> reader =                    \
                              open_file_for_reading<test_data_entry<Tsub, 2>>(                               \
                                  std::string(KFR_SRC_DIR "/tests/data/" #fn "_") +                          \
                                  tname(cometa::ctype<Tsub>) + "_" #datafile);                               \
                          test_data_entry<Tsub, 2> entry;                                                    \
                          while (reader->read(entry))                                                        \
                          {                                                                                  \
                              testo::scope s(as_string(entry.arguments[0], entry.arguments[1]));             \
                              CHECK_DIFF(kfr::fn(entry.arguments[0], entry.arguments[1]), entry.result,      \
                                         maxulps, __FILE__, __LINE__);                                       \
                          }                                                                                  \
                          CHECK(checks_count > 0u);                                                          \
                          CHECK(error_sum / checks_count <= avgulps);                                        \
                          if (show_measured_accuracy)                                                        \
                              println("measured accuracy: ", tname(cometa::ctype<Tsub>), " ",                \
                                      error_sum / checks_count, "(peak ", error_peak, ")");                  \
                      });                                                                                    \
    }
} // namespace CMT_ARCH_NAME
} // namespace kfr
