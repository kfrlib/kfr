/** @addtogroup filter
 *  @{
 */
/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */
#pragma once

#include "../cident.h"
#include <functional>

namespace kfr
{

template <typename T, bool Stateless>
struct state_holder
{
    constexpr state_holder()                    = delete;
    constexpr state_holder(const state_holder&) = default;
    constexpr state_holder(state_holder&&)      = default;
    constexpr state_holder(const T& state) CMT_NOEXCEPT : s(state) {}
    constexpr state_holder(std::reference_wrapper<const T> state) CMT_NOEXCEPT : s(state) {}
    T s;

    const T* operator->() const { return &s; }
    T* operator->() { return &s; }
    const T& operator*() const { return s; }
    T& operator*() { return s; }
};

template <typename T>
struct state_holder<T, true>
{
    constexpr state_holder()                    = delete;
    constexpr state_holder(const state_holder&) = default;
    constexpr state_holder(state_holder&&)      = default;
    constexpr state_holder(T& state) CMT_NOEXCEPT : s(state) {}
    constexpr state_holder(std::reference_wrapper<T> state) CMT_NOEXCEPT : s(state) {}
    T& s;

    const T* operator->() const { return &s; }
    T* operator->() { return &s; }
    const T& operator*() const { return s; }
    T& operator*() { return s; }
};

} // namespace kfr
