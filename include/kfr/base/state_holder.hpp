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
struct state_holder;

template <typename T>
struct state_holder<T, false>
{
    static_assert(!std::is_const_v<T>, "state_holder: T must not be const");

    constexpr state_holder()                    = delete;
    constexpr state_holder(const state_holder&) = default;
    constexpr state_holder(state_holder&&)      = default;
    constexpr state_holder(T state) CMT_NOEXCEPT : s(std::move(state)) {}
    constexpr state_holder(std::reference_wrapper<T> state)       = delete;
    constexpr state_holder(std::reference_wrapper<const T> state) = delete;
    constexpr state_holder(state_holder<T, true> stateless) : s(*stateless) {}
    T s;

    const T* operator->() const { return &s; }
    T* operator->() { return &s; }
    const T& operator*() const { return s; }
    T& operator*() { return s; }
};

template <typename T>
struct state_holder<T, true>
{
    static_assert(!std::is_const_v<T>, "state_holder: T must not be const");
    
    constexpr state_holder()                            = delete;
    constexpr state_holder(const state_holder&)         = default;
    constexpr state_holder(state_holder&&)              = default;
    constexpr state_holder(T state) CMT_NOEXCEPT        = delete;
    constexpr state_holder(const T& state) CMT_NOEXCEPT = delete;
    constexpr state_holder(T& state) CMT_NOEXCEPT       = delete;
    constexpr state_holder(T&& state) CMT_NOEXCEPT      = delete;
    constexpr state_holder(std::reference_wrapper<T> state) CMT_NOEXCEPT : s(&state.get()) {}
    T* s;

    const T* operator->() const { return s; }
    T* operator->() { return s; }
    const T& operator*() const { return *s; }
    T& operator*() { return *s; }
};

static_assert(std::is_copy_constructible_v<state_holder<float, true>>);
static_assert(std::is_copy_constructible_v<state_holder<float, false>>);

static_assert(std::is_move_constructible_v<state_holder<float, true>>);
static_assert(std::is_move_constructible_v<state_holder<float, false>>);

} // namespace kfr
