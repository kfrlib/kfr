/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "../dft_c.h"

#include "../../base/basic_expressions.hpp"
#include "../../math/complex_math.hpp"
#include "../../testo/assert.hpp"
#include "../cache.hpp"
#include "../fft.hpp"
#include "bitrev.hpp"
#include "ft.hpp"

namespace kfr
{

inline namespace CMT_ARCH_NAME
{

#define DFT_ASSERT TESTO_ASSERT_INACTIVE

template <typename T>
constexpr size_t fft_vector_width = vector_width<T>;

using cdirect_t = cfalse_t;
using cinvert_t = ctrue_t;

template <typename T>
struct dft_stage
{
    size_t radix      = 0;
    size_t stage_size = 0;
    size_t data_size  = 0;
    size_t temp_size  = 0;
    u8* data          = nullptr;
    size_t repeats    = 1;
    size_t out_offset = 0;
    size_t blocks     = 0;
    const char* name  = nullptr;
    bool recursion    = false;
    bool can_inplace  = true;
    bool inplace      = false;
    bool to_scratch   = false;
    bool need_reorder = true;

    void initialize(size_t size) { do_initialize(size); }

    virtual void dump() const
    {
        printf("%s: \n\t%5zu,%5zu,%5zu,%5zu,%5zu,%5zu,%5zu, %d, %d, %d, %d\n", name ? name : "unnamed", radix,
               stage_size, data_size, temp_size, repeats, out_offset, blocks, recursion, can_inplace, inplace,
               to_scratch);
    }

    KFR_MEM_INTRINSIC void execute(cdirect_t, complex<T>* out, const complex<T>* in, u8* temp)
    {
        do_execute(cdirect_t(), out, in, temp);
    }
    KFR_MEM_INTRINSIC void execute(cinvert_t, complex<T>* out, const complex<T>* in, u8* temp)
    {
        do_execute(cinvert_t(), out, in, temp);
    }
    virtual ~dft_stage() {}

protected:
    virtual void do_initialize(size_t) {}
    virtual void do_execute(cdirect_t, complex<T>*, const complex<T>*, u8* temp) = 0;
    virtual void do_execute(cinvert_t, complex<T>*, const complex<T>*, u8* temp) = 0;
};

#define DFT_STAGE_FN                                                                                         \
    void do_execute(cdirect_t, complex<T>* out, const complex<T>* in, u8* temp) override                     \
    {                                                                                                        \
        return do_execute<false>(out, in, temp);                                                             \
    }                                                                                                        \
    void do_execute(cinvert_t, complex<T>* out, const complex<T>* in, u8* temp) override                     \
    {                                                                                                        \
        return do_execute<true>(out, in, temp);                                                              \
    }

CMT_PRAGMA_GNU(GCC diagnostic push)
#if CMT_HAS_WARNING("-Wassume")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wassume")
#endif

template <typename T>
template <typename Stage, typename... Args>
void dft_plan<T>::add_stage(Args... args)
{
    dft_stage<T>* stage = new Stage(args...);
    stage->need_reorder = need_reorder;
    this->data_size += stage->data_size;
    this->temp_size += stage->temp_size;
    stages.push_back(dft_stage_ptr(stage));
}

} // namespace CMT_ARCH_NAME

} // namespace kfr
