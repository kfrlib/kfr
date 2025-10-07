/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2025 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/io.hpp>
#include <kfr/test/test.hpp>

namespace kfr
{

inline bool show_measured_accuracy = false;

inline namespace KFR_ARCH_NAME
{

using vector_types =
    ctypes_t<f32, f32x1, f32x2, f32x4, f32x8, f32x16, f32x32, f64, f64x1, f64x2, f64x4, f64x8, f64x16>;

inline const char* tname(ctype_t<f32>) { return "float"; }
inline const char* tname(ctype_t<f64>) { return "double"; }

#define KFR_AUTO_TEST_1(fn, datafile, maxulps, avgulps)                                                      \
    TEST_CASE(KFR_STRINGIFY(fn##_##datafile))                                                                \
    {                                                                                                        \
        test_matrix(named("type") = vector_types(),                                                          \
                    [&](auto type)                                                                           \
                    {                                                                                        \
                        using T               = typename decltype(type)::type;                               \
                        using Tsub            = subtype<T>;                                                  \
                        double error_sum      = 0.0;                                                         \
                        uint64_t error_peak   = 0;                                                           \
                        uint64_t checks_count = 0;                                                           \
                        std::shared_ptr<file_reader<test_data_entry<Tsub, 1>>> reader =                      \
                            open_file_for_reading<test_data_entry<Tsub, 1>>(                                 \
                                std::string(KFR_SRC_DIR "/tests/data/" #fn "_") + tname(kfr::ctype<Tsub>) +  \
                                "_" #datafile);                                                              \
                        test_data_entry<Tsub, 1> entry;                                                      \
                        while (reader->read(entry))                                                          \
                        {                                                                                    \
                            INFO(as_string(entry.arguments[0]));                                             \
                            ++checks_count;                                                                  \
                            const auto x_arg_value = kfr::fn(entry.arguments[0]);                            \
                            uint64_t arg_diff      = Catch::ulpDistance(x_arg_value, entry.result);          \
                            error_sum += arg_diff;                                                           \
                            error_peak = std::max(error_peak, arg_diff);                                     \
                            CHECK_THAT(x_arg_value, Catch::Matchers::WithinULP(entry.result, maxulps));      \
                        }                                                                                    \
                        CHECK(checks_count > 0u);                                                            \
                        CHECK(error_sum / checks_count <= avgulps);                                          \
                        if (show_measured_accuracy)                                                          \
                            println("measured accuracy: ", tname(kfr::ctype<Tsub>), " ",                     \
                                    error_sum / checks_count, "(peak ", error_peak, ")");                    \
                    });                                                                                      \
    }

#define KFR_AUTO_TEST_2(fn, datafile, maxulps, avgulps)                                                      \
    TEST_CASE(KFR_STRINGIFY(fn##_##datafile))                                                                \
    {                                                                                                        \
        test_matrix(named("type") = vector_types(),                                                          \
                    [&](auto type)                                                                           \
                    {                                                                                        \
                        using T               = typename decltype(type)::type;                               \
                        using Tsub            = subtype<T>;                                                  \
                        double error_sum      = 0.0;                                                         \
                        uint64_t error_peak   = 0;                                                           \
                        uint64_t checks_count = 0;                                                           \
                        std::shared_ptr<file_reader<test_data_entry<Tsub, 2>>> reader =                      \
                            open_file_for_reading<test_data_entry<Tsub, 2>>(                                 \
                                std::string(KFR_SRC_DIR "/tests/data/" #fn "_") + tname(kfr::ctype<Tsub>) +  \
                                "_" #datafile);                                                              \
                        test_data_entry<Tsub, 2> entry;                                                      \
                        while (reader->read(entry))                                                          \
                        {                                                                                    \
                            INFO(as_string(entry.arguments[0], entry.arguments[1]));                         \
                            ++checks_count;                                                                  \
                            const auto x_arg_value = kfr::fn(entry.arguments[0], entry.arguments[1]);        \
                            uint64_t arg_diff      = Catch::ulpDistance(x_arg_value, entry.result);          \
                            error_sum += arg_diff;                                                           \
                            error_peak = std::max(error_peak, arg_diff);                                     \
                            CHECK_THAT(x_arg_value, Catch::Matchers::WithinULP(entry.result, maxulps));      \
                        }                                                                                    \
                        CHECK(checks_count > 0u);                                                            \
                        CHECK(error_sum / checks_count <= avgulps);                                          \
                        if (show_measured_accuracy)                                                          \
                            println("measured accuracy: ", tname(kfr::ctype<Tsub>), " ",                     \
                                    error_sum / checks_count, "(peak ", error_peak, ")");                    \
                    });                                                                                      \
    }
} // namespace KFR_ARCH_NAME
} // namespace kfr
