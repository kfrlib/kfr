/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */
#pragma once

#include "../cident.h"
#include <functional>

namespace kfr
{

/**
 * @brief Holds a state value, either by ownership or by reference, depending
 *        on the @p Stateless template parameter.
 *
 * @tparam T          The state value type. Must not be const-qualified.
 * @tparam Stateless  When @c false, the holder owns a copy of the state.
 *                    When @c true, the holder stores a (non-owning) pointer to
 *                    an externally owned state object.
 *
 * The holder exposes pointer- and dereference-style access (@c operator-> and
 * @c operator*) so that call sites can treat owned and referenced state
 * uniformly.
 */
template <typename T, bool Stateless>
struct state_holder;

/**
 * @brief Owning specialization of @ref state_holder.
 *
 * Stores a copy of the state value of type @p T. The default constructor is
 * deleted; a holder must be initialized with a value (or converted from a
 * stateless holder, in which case it copies the referenced state).
 */
template <typename T>
struct state_holder<T, false>
{
    static_assert(!std::is_const_v<T>, "state_holder: T must not be const");

    constexpr state_holder()                    = delete;
    constexpr state_holder(const state_holder&) = default;
    constexpr state_holder(state_holder&&)      = default;
    /// Constructs the holder by moving in a state value.
    constexpr state_holder(T state) noexcept : s(std::move(state)) {}
    constexpr state_holder(std::reference_wrapper<T> state)       = delete;
    constexpr state_holder(std::reference_wrapper<const T> state) = delete;
    /// Converts a stateless holder into an owning one by copying the
    /// referenced state.
    constexpr state_holder(state_holder<T, true> stateless) : s(*stateless) {}
    /// The owned state value.
    T s;

    /// @brief Returns a pointer to the state (const overload).
    const T* operator->() const { return &s; }
    /// @brief Returns a pointer to the state (non-const overload).
    T* operator->() { return &s; }
    /// @brief Returns a reference to the state (const overload).
    const T& operator*() const { return s; }
    /// @brief Returns a reference to the state (non-const overload).
    T& operator*() { return s; }
};

/**
 * @brief Stateless (non-owning) specialization of @ref state_holder.
 *
 * Stores a pointer to an externally owned state object of type @p T. The
 * holder does not manage the lifetime of the pointee. Value-taking
 * constructors are deleted; the holder must be constructed from a
 * @c std::reference_wrapper.
 */
template <typename T>
struct state_holder<T, true>
{
    static_assert(!std::is_const_v<T>, "state_holder: T must not be const");

    constexpr state_holder()                        = delete;
    constexpr state_holder(const state_holder&)     = default;
    constexpr state_holder(state_holder&&)          = default;
    constexpr state_holder(T state) noexcept        = delete;
    constexpr state_holder(const T& state) noexcept = delete;
    constexpr state_holder(T& state) noexcept       = delete;
    constexpr state_holder(T&& state) noexcept      = delete;
    /// Constructs the holder from a reference wrapper, storing a pointer to
    /// the referenced object.
    constexpr state_holder(std::reference_wrapper<T> state) noexcept : s(&state.get()) {}
    /// Pointer to the externally owned state. Never null for a valid holder.
    T* s;

    /// @brief Returns a pointer to the state (const overload).
    const T* operator->() const { return s; }
    /// @brief Returns a pointer to the state (non-const overload).
    T* operator->() { return s; }
    /// @brief Returns a reference to the state (const overload).
    const T& operator*() const { return *s; }
    /// @brief Returns a reference to the state (non-const overload).
    T& operator*() { return *s; }
};

static_assert(std::is_copy_constructible_v<state_holder<float, true>>);
static_assert(std::is_copy_constructible_v<state_holder<float, false>>);

static_assert(std::is_move_constructible_v<state_holder<float, true>>);
static_assert(std::is_move_constructible_v<state_holder<float, false>>);

} // namespace kfr
