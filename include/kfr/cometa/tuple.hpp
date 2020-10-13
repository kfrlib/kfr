/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cident.h"

#include <cstdint>
#include <tuple>

namespace cometa
{

using std::ptrdiff_t;
using std::size_t;

template <typename T, T...>
struct cvals_t;

template <size_t... values>
using csizes_t = cvals_t<size_t, values...>;

struct swallow;

namespace details
{

template <typename T, size_t Nsize, T Nstart, std::ptrdiff_t Nstep>
struct cvalseq_impl;

template <typename... Ts, typename Fn, size_t... indices>
void cforeach_tuple_impl(const std::tuple<Ts...>& tuple, Fn&& fn, csizes_t<indices...>)
{
    swallow{ (fn(std::get<indices>(tuple)), void(), 0)... };
}
} // namespace details

template <typename... Ts, typename Fn>
void cforeach(const std::tuple<Ts...>& tuple, Fn&& fn)
{
    details::cforeach_tuple_impl(tuple, std::forward<Fn>(fn),
                                 typename details::cvalseq_impl<size_t, sizeof...(Ts), 0, 1>::type());
}
} // namespace cometa

#include "../cometa.hpp"
