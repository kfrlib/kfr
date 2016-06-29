/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
 */
#pragma once

#include "kfr.h"

#include "types.hpp"

namespace kfr
{

namespace internal
{

template <typename Fn, cpu_t newcpu, typename = void>
struct retarget_impl
{
    using type = Fn;
};

template <typename Fn, cpu_t newcpu>
struct retarget_impl<Fn, newcpu, void_t<typename Fn::template retarget_this<newcpu>>>
{
    using type = typename Fn::template retarget_this<newcpu>;
};
}

template <typename Fn, cpu_t newcpu>
using retarget = typename internal::retarget_impl<Fn, newcpu>::type;

template <cpu_t newcpu, typename Fn, typename NewFn = retarget<Fn, newcpu>,
          KFR_ENABLE_IF(std::is_constructible<NewFn, Fn&&>::value)>
KFR_INLINE NewFn retarget_func(Fn&& fn)
{
    return NewFn(std::move(fn));
}

template <cpu_t newcpu, typename Fn, typename NewEmptyFn = retarget<Fn, newcpu>,
          KFR_ENABLE_IF(!std::is_constructible<NewEmptyFn, Fn&&>::value && std::is_empty<NewEmptyFn>::value &&
                        std::is_constructible<NewEmptyFn>::value)>
KFR_INLINE NewEmptyFn retarget_func(Fn&&)
{
    return NewEmptyFn();
}

namespace internal
{

template <cpu_t a>
struct cpu_caller;

template <>
struct cpu_caller<cpu_t::avx2>
{
    constexpr static cpu_t a = cpu_t::avx2;

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(avx2) result_of<Fn(Args...)> call(Fn&& fn, Args&&... args)
    {
        return fn(std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(avx2) result_of<Fn(Args...)> retarget_call(Fn&& fn, Args&&... args)
    {
        return (retarget_func<a>(std::forward<Fn>(fn)))(std::forward<Args>(args)...);
    }
};

template <>
struct cpu_caller<cpu_t::avx1>
{
    constexpr static cpu_t a = cpu_t::avx1;

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(avx) result_of<Fn(Args...)> call(Fn&& fn, Args&&... args)
    {
        return fn(std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(avx) result_of<Fn(Args...)> retarget_call(Fn&& fn, Args&&... args)
    {
        return (retarget_func<a>(std::forward<Fn>(fn)))(std::forward<Args>(args)...);
    }
};

template <>
struct cpu_caller<cpu_t::sse41>
{
    constexpr static cpu_t a = cpu_t::sse41;

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(sse41) result_of<Fn(Args...)> call(Fn&& fn, Args&&... args)
    {
        return fn(std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(sse41) result_of<Fn(Args...)> retarget_call(Fn&& fn, Args&&... args)
    {
        return (retarget_func<a>(std::forward<Fn>(fn)))(std::forward<Args>(args)...);
    }
};

template <>
struct cpu_caller<cpu_t::ssse3>
{
    constexpr static cpu_t a = cpu_t::ssse3;

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(ssse3) result_of<Fn(Args...)> call(Fn&& fn, Args&&... args)
    {
        return fn(std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(ssse3) result_of<Fn(Args...)> retarget_call(Fn&& fn, Args&&... args)
    {
        return (retarget_func<a>(std::forward<Fn>(fn)))(std::forward<Args>(args)...);
    }
};

template <>
struct cpu_caller<cpu_t::sse3>
{
    constexpr static cpu_t a = cpu_t::sse3;

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(sse3) result_of<Fn(Args...)> call(Fn&& fn, Args&&... args)
    {
        return fn(std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(sse3) result_of<Fn(Args...)> retarget_call(Fn&& fn, Args&&... args)
    {
        return (retarget_func<a>(std::forward<Fn>(fn)))(std::forward<Args>(args)...);
    }
};

template <>
struct cpu_caller<cpu_t::sse2>
{
    constexpr static cpu_t a = cpu_t::sse2;

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(sse2) result_of<Fn(Args...)> call(Fn&& fn, Args&&... args)
    {
        return fn(std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    KFR_NOINLINE static KFR_USE_CPU(sse2) result_of<Fn(Args...)> retarget_call(Fn&& fn, Args&&... args)
    {
        return (retarget_func<a>(std::forward<Fn>(fn)))(std::forward<Args>(args)...);
    }
};

template <cpu_t c, typename Fn, typename... Args, KFR_ENABLE_IF(c == cpu_t::native)>
KFR_INLINE auto dispatch_impl(Fn&& fn, Args&&... args) -> decltype(fn(std::forward<Args>(args)...))
{
    using targetFn = retarget<Fn, cpu_t::native>;
    targetFn newfn = retarget_func<c>(std::forward<Fn>(fn));
    return newfn(std::forward<Args>(args)...);
}

template <cpu_t c, typename Fn, typename... Args, KFR_ENABLE_IF(c != cpu_t::native && c != cpu_t::runtime)>
KFR_INLINE auto dispatch_impl(Fn&& fn, Args&&... args) -> decltype(fn(std::forward<Args>(args)...))
{
    return internal::cpu_caller<c>::retarget_call(std::forward<Fn>(fn), std::forward<Args>(args)...);
}
}

template <cpu_t c, typename Fn, typename... Args>
KFR_INLINE auto dispatch(Fn&& fn, Args&&... args) -> decltype(fn(std::forward<Args>(args)...))
{
    return internal::dispatch_impl<c>(std::forward<Fn>(fn), std::forward<Args>(args)...);
}
}
