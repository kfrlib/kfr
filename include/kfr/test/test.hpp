#pragma once

#include "comparison.hpp"

#include <algorithm>
#include <ctime>
#include <functional>
#include <sstream>
#include <utility>
#include <vector>
#include <cassert>
#include <chrono>
#include <cmath>

#include "../thirdparty/catch/catch_amalgamated.hpp"

KFR_PRAGMA_GNU(GCC diagnostic push)
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wpragmas")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wexit-time-destructors")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wpadded")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wparentheses")

namespace kfr
{

/**
 * @brief Runs a test body for every value of a single named argument.
 *
 * Iterates over @p arg0.value with @ref cforeach, emitting a Catch2 @c INFO
 * record that names the argument and its current value before invoking @p fn.
 * On assertion failure the recorded info is printed alongside the failure.
 *
 * @tparam Arg0 Type of the named argument's stored value.
 * @tparam Fn   Callable invoked as @c fn(v0) for each value @c v0.
 * @param arg0 Named argument (see @ref named and @c operator""_arg).
 * @param fn   Test body to invoke for each value.
 */
template <typename Arg0, typename Fn>
void test_matrix(named_arg<Arg0>&& arg0, Fn&& fn)
{
    cforeach(std::forward<Arg0>(arg0.value),
             [&](auto v0)
             {
                 INFO(as_string(arg0.name, " = ", v0));
                 fn(v0);
             });
}

/**
 * @brief Runs a test body for every combination of two named arguments.
 *
 * Iterates over the Cartesian product of @p arg0.value and @p arg1.value with
 * @ref cforeach, emitting a Catch2 @c INFO record naming both arguments and
 * their current values before invoking @p fn.
 *
 * @tparam Arg0 Type of the first named argument's stored value.
 * @tparam Arg1 Type of the second named argument's stored value.
 * @tparam Fn   Callable invoked as @c fn(v0, v1) for each combination.
 * @param arg0 First named argument.
 * @param arg1 Second named argument.
 * @param fn   Test body to invoke for each combination.
 */
template <typename Arg0, typename Arg1, typename Fn>
void test_matrix(named_arg<Arg0>&& arg0, named_arg<Arg1>&& arg1, Fn&& fn)
{
    cforeach(std::forward<Arg0>(arg0.value), std::forward<Arg1>(arg1.value),
             [&](auto v0, auto v1)
             {
                 INFO(as_string(arg0.name, " = ", v0, ", ", arg1.name, " = ", v1));
                 fn(v0, v1);
             });
}

/**
 * @brief Runs a test body for every combination of three named arguments.
 *
 * Iterates over the Cartesian product of @p arg0.value, @p arg1.value and
 * @p arg2.value with @ref cforeach, emitting a Catch2 @c INFO record naming all
 * three arguments and their current values before invoking @p fn.
 *
 * @tparam Arg0 Type of the first named argument's stored value.
 * @tparam Arg1 Type of the second named argument's stored value.
 * @tparam Arg2 Type of the third named argument's stored value.
 * @tparam Fn   Callable invoked as @c fn(v0, v1, v2) for each combination.
 * @param arg0 First named argument.
 * @param arg1 Second named argument.
 * @param arg2 Third named argument.
 * @param fn   Test body to invoke for each combination.
 */
template <typename Arg0, typename Arg1, typename Arg2, typename Fn>
void test_matrix(named_arg<Arg0>&& arg0, named_arg<Arg1>&& arg1, named_arg<Arg2>&& arg2, Fn&& fn)
{
    cforeach(std::forward<Arg0>(arg0.value), std::forward<Arg1>(arg1.value), std::forward<Arg2>(arg2.value),
             [&](auto v0, auto v1, auto v2)
             {
                 INFO(
                     as_string(arg0.name, " = ", v0, ", ", arg1.name, " = ", v1, ", ", arg2.name, " = ", v2));
                 fn(v0, v1, v2);
             });
}

/**
 * @brief Runs a test body for every combination of four named arguments.
 *
 * Iterates over the Cartesian product of @p arg0.value, @p arg1.value,
 * @p arg2.value and @p arg3.value with @ref cforeach, emitting a Catch2 @c INFO
 * record naming all four arguments and their current values before invoking
 * @p fn.
 *
 * @tparam Arg0 Type of the first named argument's stored value.
 * @tparam Arg1 Type of the second named argument's stored value.
 * @tparam Arg2 Type of the third named argument's stored value.
 * @tparam Arg3 Type of the fourth named argument's stored value.
 * @tparam Fn   Callable invoked as @c fn(v0, v1, v2, v3) for each combination.
 * @param arg0 First named argument.
 * @param arg1 Second named argument.
 * @param arg2 Third named argument.
 * @param arg3 Fourth named argument.
 * @param fn   Test body to invoke for each combination.
 */
template <typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Fn>
void test_matrix(named_arg<Arg0>&& arg0, named_arg<Arg1>&& arg1, named_arg<Arg2>&& arg2,
                 named_arg<Arg3>&& arg3, Fn&& fn)
{
    cforeach(std::forward<Arg0>(arg0.value), std::forward<Arg1>(arg1.value), std::forward<Arg2>(arg2.value),
             std::forward<Arg3>(arg3.value),
             [&](auto v0, auto v1, auto v2, auto v3)
             {
                 INFO(as_string(arg0.name, " = ", v0, ", ", arg1.name, " = ", v1, ", ", arg2.name, " = ", v2,
                                arg3.name, " = ", v3));
                 fn(v0, v1, v2, v3);
             });
}

/**
 * @brief Compile-time assertion that two types are identical.
 *
 * Emits a @c static_assert failure when @c std::is_same_v<T1, T2> is @c false.
 *
 * @tparam T1 First type to compare.
 * @tparam T2 Second type to compare.
 */
template <typename T1, typename T2>
void assert_is_same()
{
    static_assert(std::is_same_v<T1, T2>, "");
}

/**
 * @brief Compile-time assertion that two types are identical after decay.
 *
 * Applies @c std::decay_t to both @p T1 and @p T2 before the @c is_same check,
 * so references and top-level cv-qualifiers are ignored.
 *
 * @tparam T1 First type to compare.
 * @tparam T2 Second type to compare.
 */
template <typename T1, typename T2>
void assert_is_same_decay()
{
    static_assert(std::is_same_v<std::decay_t<T1>, std::decay_t<T2>>, "");
}

/**
 * @brief A single row of tabulated test data.
 *
 * Stores @p NArgs input arguments together with the expected @p result, used
 * to drive table-driven tests where a function is exercised against a fixed
 * set of input/output pairs.
 *
 * @tparam T     Value type shared by all arguments and the result.
 * @tparam NArgs Number of input arguments stored in @ref arguments.
 */
template <typename T, size_t NArgs>
struct test_data_entry
{
    T arguments[NArgs]; /**< Input arguments for the test row. */
    T result; /**< Expected result for the test row. */
};

} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)
