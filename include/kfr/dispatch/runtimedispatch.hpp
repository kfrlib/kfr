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

#include "../base/dispatch.hpp"
#include "../base/types.hpp"
#include "cpuid.hpp"

namespace kfr
{

namespace internal
{

template <typename Fn, typename... Args>
KFR_CPU_INTRIN(sse2)
auto with_cpu_impl(ccpu_t<cpu_t::sse2>, Fn&& fn, Args&&... args)
{
    return fn(std::forward<Args>(args)...);
}

template <typename Fn, typename... Args>
KFR_CPU_INTRIN(sse3)
auto with_cpu_impl(ccpu_t<cpu_t::sse3>, Fn&& fn, Args&&... args)
{
    return fn(std::forward<Args>(args)...);
}

template <typename Fn, typename... Args>
KFR_CPU_INTRIN(ssse3)
auto with_cpu_impl(ccpu_t<cpu_t::ssse3>, Fn&& fn, Args&&... args)
{
    return fn(std::forward<Args>(args)...);
}

template <typename Fn, typename... Args>
KFR_CPU_INTRIN(sse41)
auto with_cpu_impl(ccpu_t<cpu_t::sse41>, Fn&& fn, Args&&... args)
{
    return fn(std::forward<Args>(args)...);
}

template <typename Fn, typename... Args>
KFR_CPU_INTRIN(sse42)
auto with_cpu_impl(ccpu_t<cpu_t::sse42>, Fn&& fn, Args&&... args)
{
    return fn(std::forward<Args>(args)...);
}

template <typename Fn, typename... Args>
KFR_CPU_INTRIN(avx)
auto with_cpu_impl(ccpu_t<cpu_t::avx>, Fn&& fn, Args&&... args)
{
    return fn(std::forward<Args>(args)...);
}

template <typename Fn, typename... Args>
KFR_CPU_INTRIN(avx2)
auto with_cpu_impl(ccpu_t<cpu_t::avx2>, Fn&& fn, Args&&... args)
{
    return fn(std::forward<Args>(args)...);
}
}

template <cpu_t cpu, typename Fn, typename... Args>
KFR_INTRIN auto with_cpu(ccpu_t<cpu>, Fn&& fn, Args&&... args)
{
    return internal::with_cpu_impl(ccpu<cpu>, std::forward<Fn>(fn), std::forward<Args>(args)...);
}

template <cpu_t cpu, typename Fn>
struct fn_with_cpu
{
    template <typename... Args>
    KFR_INTRIN auto operator()(Args&&... args) -> decltype(std::declval<Fn>()(std::forward<Args>(args)...))
    {
        return internal::with_cpu_impl(ccpu<cpu>, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
    Fn fn;
};

template <cpu_t cpu, typename Fn>
KFR_INTRIN fn_with_cpu<cpu, Fn> make_with_cpu(ccpu_t<cpu>, Fn&& fn)
{
    return { std::forward<Fn>(fn) };
}

namespace internal
{

template <typename Fn, cpu_t, cpu_t...>
struct runtime_dispatcher;

template <typename Fn, cpu_t oldest>
struct runtime_dispatcher<Fn, oldest>
{
    using targetFn = retarget<Fn, oldest>;

    template <typename... Args>
    KFR_INLINE static result_of<targetFn(Args&&...)> call(Fn&& fn, cpu_t, Args&&... args)
    {
        return cpu_caller<oldest>::retarget_call(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};

template <typename Fn, cpu_t newest, cpu_t next, cpu_t... cpus>
struct runtime_dispatcher<Fn, newest, next, cpus...>
{
    using nextdispatcher = runtime_dispatcher<Fn, next, cpus...>;

    using targetFn = retarget<Fn, newest>;

    template <typename... Args,
              KFR_ENABLE_IF(is_callable<targetFn, Args&&...>::value&& is_enabled<targetFn>::value)>
    KFR_SINTRIN auto call(Fn&& fn, cpu_t set, Args&&... args)
        -> decltype(nextdispatcher::call(std::forward<Fn>(fn), set, std::forward<Args>(args)...))
    {
        return set >= newest
                   ? cpu_caller<newest>::retarget_call(std::forward<Fn>(fn), std::forward<Args>(args)...)
                   : nextdispatcher::call(std::forward<Fn>(fn), set, std::forward<Args>(args)...);
    }
    template <typename... Args,
              KFR_ENABLE_IF(!(is_callable<targetFn, Args&&...>::value && is_enabled<targetFn>::value))>
    KFR_SINTRIN auto call(Fn&& fn, cpu_t set, Args&&... args)
        -> decltype(nextdispatcher::call(std::forward<Fn>(fn), set, std::forward<Args>(args)...))
    {
        return nextdispatcher::call(std::forward<Fn>(fn), set, std::forward<Args>(args)...);
    }
};

template <typename Fn, cpu_t newest, cpu_t... cpus, typename... Args>
KFR_INLINE auto runtimedispatch(cvals_t<cpu_t, newest, cpus...>, Fn&& fn, Args&&... args)
    -> decltype(internal::runtime_dispatcher<Fn, newest, cpus...>::call(std::forward<Fn>(fn), get_cpu(),
                                                                        std::forward<Args>(args)...))
{
    return internal::runtime_dispatcher<Fn, newest, cpus...>::call(std::forward<Fn>(fn), get_cpu(),
                                                                   std::forward<Args>(args)...);
}

template <cpu_t c, typename Fn, typename... Args, KFR_ENABLE_IF(c == cpu_t::runtime)>
KFR_INLINE auto dispatch(Fn&& fn, Args&&... args) -> decltype(fn(std::forward<Args>(args)...))
{
    return runtimedispatch(std::forward<Fn>(fn), std::forward<Args>(args)...);
}
}

template <typename Fn, typename cpulist = decltype(cpu_all), typename... Args>
KFR_INLINE auto runtimedispatch(Fn&& fn, Args&&... args)
    -> decltype(internal::runtimedispatch<Fn>(cpulist(), std::forward<Fn>(fn), std::forward<Args>(args)...))
{
    return internal::runtimedispatch(cpulist(), std::forward<Fn>(fn), std::forward<Args>(args)...);
}
}
