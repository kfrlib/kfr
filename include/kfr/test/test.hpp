/** @addtogroup testo
 *  @{
 */
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

template <typename T1, typename T2>
void assert_is_same()
{
    static_assert(std::is_same_v<T1, T2>, "");
}
template <typename T1, typename T2>
void assert_is_same_decay()
{
    static_assert(std::is_same_v<std::decay_t<T1>, std::decay_t<T2>>, "");
}

template <typename T, size_t NArgs>
struct test_data_entry
{
    T arguments[NArgs];
    T result;
};

} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)
