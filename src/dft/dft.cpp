/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
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

#include <kfr/dft/fft.hpp>
#include <kfr/multiarch.h>

namespace kfr
{

template <typename T>
void dft_plan<T>::dump() const
{
    for (const std::unique_ptr<dft_stage<T>>& s : all_stages)
    {
        s->dump();
    }
}

template <typename T>
void dft_plan<T>::calc_disposition()
{
    for (bool inverse : { false, true })
    {
        auto&& stages = this->stages[inverse];
        bitset can_inplace_per_stage;
        for (int i = 0; i < stages.size(); ++i)
        {
            can_inplace_per_stage[i] = stages[i]->can_inplace;
        }

        disposition_inplace[static_cast<int>(inverse)] =
            precompute_disposition(stages.size(), can_inplace_per_stage, true);
        disposition_outofplace[static_cast<int>(inverse)] =
            precompute_disposition(stages.size(), can_inplace_per_stage, false);
    }
}

template <typename T>
typename dft_plan<T>::bitset dft_plan<T>::precompute_disposition(int num_stages, bitset can_inplace_per_stage,
                                                                 bool inplace_requested)
{
    static bitset even{ 0x5555555555555555ull };
    bitset mask = ~bitset() >> (DFT_MAX_STAGES - num_stages);
    bitset result;
    // disposition indicates where is input for corresponding stage
    // first bit : 0 - input,  1 - scratch
    // other bits: 0 - output, 1 - scratch

    // build disposition that works always
    if (num_stages % 2 == 0)
    { // even
        result = ~even & mask;
    }
    else
    { // odd
        result = even & mask;
    }

    int num_inplace = can_inplace_per_stage.count();

#ifdef KFR_DFT_ELIMINATE_MEMCPY
    if (num_inplace > 0 && inplace_requested)
    {
        if (result.test(0)) // input is in scratch
        {
            // num_inplace must be odd
            if (num_inplace % 2 == 0)
                --num_inplace;
        }
        else
        {
            // num_inplace must be even
            if (num_inplace % 2 != 0)
                --num_inplace;
        }
    }
#endif
    if (num_inplace > 0)
    {
        for (int i = num_stages - 1; i >= 0; --i)
        {
            if (can_inplace_per_stage.test(i))
            {
                result ^= ~bitset() >> (DFT_MAX_STAGES - (i + 1));

                if (--num_inplace == 0)
                    break;
            }
        }
    }

    if (!inplace_requested) // out-of-place first stage; IN->OUT
        result.reset(0);

    return result;
}

template struct dft_plan<float>;
template struct dft_plan<double>;

CMT_MULTI_PROTO(namespace impl {
    template <typename T>
    void dft_initialize(dft_plan<T> & plan);
    template <typename T>
    void dft_real_initialize(dft_plan_real<T> & plan);
    template <typename T, bool inverse>
    void dft_execute(const dft_plan<T>& plan, cbool_t<inverse>, complex<T>* out, const complex<T>* in,
                     u8* temp);
    template <typename T>
    void dft_initialize_transpose(internal_generic::fn_transpose<T> & transpose);
})

#ifdef CMT_MULTI_NEEDS_GATE

namespace internal_generic
{

template <typename T>
void dft_initialize(dft_plan<T>& plan)
{
    CMT_MULTI_GATE(ns::impl::dft_initialize(plan));
}
template <typename T>
void dft_real_initialize(dft_plan_real<T>& plan)
{
    CMT_MULTI_GATE(ns::impl::dft_real_initialize(plan));
}
template <typename T, bool inverse>
void dft_execute(const dft_plan<T>& plan, cbool_t<inverse>, complex<T>* out, const complex<T>* in, u8* temp)
{
    CMT_MULTI_GATE(ns::impl::dft_execute(plan, cbool<inverse>, out, in, temp));
}
template <typename T>
void dft_initialize_transpose(fn_transpose<T>& transpose)
{
    CMT_MULTI_GATE(ns::impl::dft_initialize_transpose(transpose));
}

template void dft_initialize<float>(dft_plan<float>&);
template void dft_initialize<double>(dft_plan<double>&);
template void dft_real_initialize<float>(dft_plan_real<float>&);
template void dft_real_initialize<double>(dft_plan_real<double>&);
template void dft_execute<float>(const dft_plan<float>&, cbool_t<false>, complex<float>*,
                                 const complex<float>*, u8*);
template void dft_execute<float>(const dft_plan<float>&, cbool_t<true>, complex<float>*,
                                 const complex<float>*, u8*);
template void dft_execute<double>(const dft_plan<double>&, cbool_t<false>, complex<double>*,
                                  const complex<double>*, u8*);
template void dft_execute<double>(const dft_plan<double>&, cbool_t<true>, complex<double>*,
                                  const complex<double>*, u8*);
template void dft_initialize_transpose<float>(fn_transpose<float>&);
template void dft_initialize_transpose<double>(fn_transpose<double>&);

} // namespace internal_generic

#endif

} // namespace kfr
