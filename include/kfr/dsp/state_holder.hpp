/** @addtogroup fir
 *  @{
 */
/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */
#pragma once

#include "../cident.h"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
namespace internal
{

template <typename T, bool stateless>
struct state_holder
{
    state_holder()                    = delete;
    state_holder(const state_holder&) = default;
    state_holder(state_holder&&)      = default;
    constexpr state_holder(const T& state) CMT_NOEXCEPT : s(state) {}
    T s;
};

template <typename T>
struct state_holder<T, true>
{
    state_holder()                    = delete;
    state_holder(const state_holder&) = default;
    state_holder(state_holder&&)      = default;
    constexpr state_holder(const T& state) CMT_NOEXCEPT : s(state) {}
    const T& s;
};
} // namespace internal
} // namespace CMT_ARCH_NAME
} // namespace kfr
