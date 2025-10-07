/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  KFR is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with KFR.

  If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
  Buying a commercial license is mandatory as soon as you develop commercial activities without
  disclosing the source code of your own applications.
  See https://www.kfrlib.com for details.
 */
#pragma once

#include <kfr/base/basic_expressions.hpp>
#include <kfr/math/complex_math.hpp>
#include <kfr/test/assert.hpp>
#include <kfr/dft/cache.hpp>
#include <kfr/dft/fft.hpp>
#include "bitrev.hpp"
#include "ft.hpp"

namespace kfr
{

inline namespace KFR_ARCH_NAME
{
namespace intr
{
struct name_test_impl
{
};
} // namespace intr
} // namespace KFR_ARCH_NAME

template <typename T, cpu_t cpu>
struct dft_name_impl
{
};

template <typename Class>
inline const char* dft_name(Class*)
{
    constexpr static size_t prefix_len = ctype_name<intr::name_test_impl>().length() - 14;
    static constexpr cstring full_name = ctype_name<std::decay_t<Class>>();
    static constexpr cstring name_arch =
        concat_cstring(full_name.slice(csize<prefix_len>), make_cstring("("),
                       make_cstring(KFR_STRINGIFY(KFR_ARCH_NAME)), make_cstring(")"));
    return name_arch.c_str();
}

#define DFT_STAGE_FN                                                                                         \
    KFR_MEM_INTRINSIC void do_execute(cdirect_t, complex<T>* out, const complex<T>* in, u8* temp) final      \
    {                                                                                                        \
        return do_execute<false>(out, in, temp);                                                             \
    }                                                                                                        \
    KFR_MEM_INTRINSIC void do_execute(cinvert_t, complex<T>* out, const complex<T>* in, u8* temp) final      \
    {                                                                                                        \
        return do_execute<true>(out, in, temp);                                                              \
    }
#define DFT_STAGE_FN_NONFINAL                                                                                \
    void do_execute(cdirect_t, complex<T>* out, const complex<T>* in, u8* temp) override                     \
    {                                                                                                        \
        return do_execute<false>(out, in, temp);                                                             \
    }                                                                                                        \
    void do_execute(cinvert_t, complex<T>* out, const complex<T>* in, u8* temp) override                     \
    {                                                                                                        \
        return do_execute<true>(out, in, temp);                                                              \
    }

inline namespace KFR_ARCH_NAME
{

#define DFT_ASSERT KFR_ASSERT_INACTIVE

template <typename T>
constexpr size_t fft_vector_width = vector_width<T>;

KFR_PRAGMA_GNU(GCC diagnostic push)
#if KFR_HAS_WARNING("-Wassume")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wassume")
#endif

template <typename Stage, bool add_stages = true, typename T, typename... Args>
void add_stage(dft_plan<T>* plan, Args... args)
{
    dft_stage<T>* stage = new Stage(args...);
    stage->need_reorder = true;
    plan->data_size += stage->data_size;
    plan->temp_size += stage->temp_size;
    plan->all_stages.push_back(dft_stage_ptr<T>(stage));
    if constexpr (add_stages)
    {
        plan->stages[0].push_back(stage);
        plan->stages[1].push_back(stage);
    }
}

} // namespace KFR_ARCH_NAME

} // namespace kfr
