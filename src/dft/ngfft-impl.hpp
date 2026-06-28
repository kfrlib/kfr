/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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

#include "ngfft.hpp"
#include "sandwich.hpp"

#include "kfr/runtime/time.hpp"
#include <kfr/simd/bitshuffle.hpp>
#ifdef KFR_DFT_MEASURE_STAGE_TIME
#include <kfr/runtime/time.hpp>
#endif

namespace kfr
{

template <typename T, dft_family family>
using dft_initializer = void (*)(const ngfft_plan<T>& plan, const dft_config<family>& cfg) noexcept;

template <typename T>
using dft_function = void (*)(const ngfft_plan<T>& plan, std::complex<T>* out,
                              const std::complex<T>* in) noexcept;

template <typename T, dft_family family>
struct dft_specialization
{
    dft_config<family> config;
    size_t twiddle_count;
    dft_initializer<T, family> initializer;
    dft_function<T> forward;
    dft_function<T> backward;

    size_t real_twiddle_count;
    dft_initializer<T, family> real_initializer;
    dft_function<T> post_real_forward;
    dft_function<T> pre_real_backward;
};

inline namespace KFR_ARCH_NAME
{
namespace dft_internal
{

template <dft_traits traits>
constexpr uint8_t l2minfftsize() noexcept
{
    if constexpr (traits::algo == dft_algorithm::fourstep)
        return 6;
    else
        return traits::l2basewidth + traits::l2baseradix;
}
template <dft_traits traits>
constexpr uint8_t numfftsizes() noexcept
{
    return maxfftsize<typename traits::type> + 1;
}

} // namespace dft_internal

template <uint8_t l2size, bool inverse, typename T>
void ng_do_small_dft(const ngfft_plan<T>& plan, std::complex<T>* out, const std::complex<T>* in) noexcept
{
    intr::bfly_small<l2size, inverse>(out, in);
}

constexpr static size_t real_twiddle_count(size_t l2fftsize) noexcept
{
    if (l2fftsize < 2) [[unlikely]]
        return 0;
    else
    {
        // Must match the number of complex twiddles written by ng_do_init_real_dft,
        // which fills count = (real_size / 2 + 1) / 2 entries, where real_size = 2^(l2fftsize+1).
        // That simplifies to (2^l2fftsize + 1) / 2 == 2^(l2fftsize - 1).
        const size_t real_size = (size_t(1) << l2fftsize) * 2;
        return (real_size / 2 + 1) / 2;
    }
}

template <dft_traits traits>
static void ng_do_init_real_dft(const ngfft_plan<typename traits::type>& plan,
                                const dft_config<to_family(traits::algo)>& cfg) noexcept
{
    using T                = typename traits::type;
    constexpr size_t width = vector_width<T> * 2;
    size_t real_size       = (size_t(1) << plan.l2fftsize) * 2;
    const size_t count     = (real_size / 2 + 1) / 2;
    using namespace intr;
    auto* rtwiddle = plan.twiddles;
    block_process(count, csizes_t<width, 1>(),
                  [=](size_t i, auto w)
                  {
                      constexpr size_t width = val_of(decltype(w)());
                      cwrite<width>(rtwiddle + i, cossin(dup(-constants<T>::pi *
                                                             ((enumerate<T, width>() + i + real_size / T(4)) /
                                                              (real_size / 2)))));
                  });
}

template <dft_traits traits, uint8_t l2fftsize = UINT8_MAX>
static void ng_post_real_forward(const ngfft_plan<typename traits::type>& plan,
                                 complex<typename traits::type>* out,
                                 const complex<typename traits::type>* in) noexcept
{
    using T = typename traits::type;
    using namespace intr;
    if constexpr (l2fftsize == 0) // real_size=2, csize=1
    {
        const cvec<T, 1> dc = cread<1>(in);
        cwrite<1>(out, addsub(dupeven(dc), dupodd(dc)));
    }
    else if constexpr (l2fftsize == 1) // real_size=4, csize=2
    {
        const cvec<T, 1> dc    = cread<1>(in);
        const cvec<T, 1> inmid = cread<1>(in + 1);
        cwrite<1>(out + 1, negodd(inmid));
        cwrite<1>(out, addsub(dupeven(dc), dupodd(dc)));
    }
    else
    {
        constexpr size_t width = l2fftsize == UINT8_MAX ? vector_width<T> * 2 : (size_t(1) << l2fftsize) / 2;
        const size_t real_size = (size_t(1) << (l2fftsize == UINT8_MAX ? plan.l2fftsize : l2fftsize)) * 2;
        auto* rtwiddle         = plan.twiddles;

        size_t csize = real_size / 2;

        const size_t count = (csize + 1) / 2;
        KFR_ASSUME(count > 1);

        cvec<T, 1> inmid = cread<1>(in + csize / 2);
        block_process(count /*  - 1 */, csizes_t<width, 1>(),
                      [&](size_t i, auto w)
                      {
                          i++;
                          constexpr size_t width   = val_of(decltype(w)());
                          constexpr size_t widthm1 = width - 1;
                          const cvec<T, width> tw  = cread<width>(rtwiddle + i);
                          const cvec<T, width> fpk = cread<width>(in + i);
                          const cvec<T, width> fpnk =
                              reverse<2>(negodd(cread<width>(in + csize - i - widthm1)));

                          const cvec<T, width> f1k = fpk + fpnk;
                          const cvec<T, width> f2k = fpk - fpnk;
                          const cvec<T, width> t   = cmul(f2k, tw);
                          cwrite<width>(out + i, T(0.5) * (f1k + t));
                          cwrite<width>(out + csize - i - widthm1, reverse<2>(negodd(T(0.5) * (f1k - t))));
                      });

        cwrite<1>(out + csize / 2, negodd(inmid));

        const cvec<T, 1> dc = cread<1>(in);
        cwrite<1>(out, addsub(dupeven(dc), dupodd(dc)));
    }
}
template <dft_traits traits, uint8_t l2fftsize = UINT8_MAX>
static void ng_pre_real_backward(const ngfft_plan<typename traits::type>& plan,
                                 complex<typename traits::type>* out,
                                 const complex<typename traits::type>* in) noexcept
{
    using T = typename traits::type;
    using namespace intr;
    if constexpr (l2fftsize == 0) // real_size=2, csize=1, count=n/a
    {
        cvec<T, 1> dc = cread<1>(in);
        dc            = addsub(dupeven(dc), dupodd(dc));
        cwrite<1>(out, dc);
    }
    else if constexpr (l2fftsize == 1) // real_size=4, csize=2, count=1
    {
        cvec<T, 1> dc    = cread<1>(in);
        dc               = addsub(dupeven(dc), dupodd(dc));
        cvec<T, 1> inmid = cread<1>(in + 1);
        cwrite<1>(out + 1, 2 * negodd(inmid));
        cwrite<1>(out, dc);
    }
    else
    {
        constexpr size_t width = l2fftsize == UINT8_MAX ? vector_width<T> * 2 : (size_t(1) << l2fftsize) / 2;

        const size_t real_size = (size_t(1) << (l2fftsize == UINT8_MAX ? plan.l2fftsize : l2fftsize)) * 2;
        auto* rtwiddle         = plan.twiddles;

        const size_t csize = real_size / 2;

        const size_t count = (csize + 1) / 2;
        KFR_ASSUME(count > 1);

        cvec<T, 1> inmid = cread<1>(in + csize / 2);

        block_process(count /*  - 1 */, csizes_t<width, 1>(),
                      [&](size_t i, auto w)
                      {
                          i++;
                          constexpr size_t width   = val_of(decltype(w)());
                          constexpr size_t widthm1 = width - 1;
                          const cvec<T, width> tw  = cread<width>(rtwiddle + i);
                          const cvec<T, width> fpk = cread<width>(in + i);
                          const cvec<T, width> fpnk =
                              reverse<2>(negodd(cread<width>(in + csize - i - widthm1)));

                          const cvec<T, width> f1k = fpk + fpnk;
                          const cvec<T, width> f2k = fpk - fpnk;
                          const cvec<T, width> t   = cmul_conj(f2k, tw);
                          cwrite<width>(out + i, f1k + t);
                          cwrite<width>(out + csize - i - widthm1, reverse<2>(negodd(f1k - t)));
                      });

        cwrite<1>(out + csize / 2, 2 * negodd(inmid));
        cvec<T, 1> dc = cread<1>(in);
        dc            = addsub(dupeven(dc), dupodd(dc));
        cwrite<1>(out, dc);
    }
}

template <dft_traits traits, uint8_t numfftsizes = dft_internal::numfftsizes<traits>()>
constexpr std::array<dft_specialization<typename traits::type, to_family(traits::algo)>, numfftsizes>
generate_dft_specializations() noexcept
{
    using T         = typename traits::type;
    using cfamily_t = cval_t<dft_family, to_family(traits::algo)>;
    constexpr cfamily_t cfamily{};
    constexpr uint8_t l2minsize = dft_internal::l2minfftsize<traits>();
    std::array<dft_specialization<T, to_family(traits::algo)>, numfftsizes> specs{};

    KFR_FOR(i, 0, numfftsizes)
    {
        constexpr uint8_t l2fftsize = i;
        if constexpr (l2fftsize < l2minsize)
        {
            specs[i].config            = {};
            specs[i].twiddle_count     = 0;
            specs[i].initializer       = nullptr;
            specs[i].forward           = &ng_do_small_dft<l2fftsize, false, T>;
            specs[i].backward          = &ng_do_small_dft<l2fftsize, true, T>;
            specs[i].pre_real_backward = &ng_pre_real_backward<traits, l2fftsize>;
            specs[i].post_real_forward = &ng_post_real_forward<traits, l2fftsize>;
        }
        else
        {
            constexpr dft_config_t<traits> cfg = ng_config<traits>(cfamily, l2fftsize);

            specs[i].config        = cfg;
            specs[i].twiddle_count = ng_twiddle_count<traits>(l2fftsize, cfg);
            if (specs[i].twiddle_count)
                specs[i].initializer = &ng_do_init_dft<traits>;
            if constexpr (l2fftsize <= numfixedfftsizes)
            {
                // FFT size known at compile time, can use fixed-size routine
                specs[i].forward  = &ng_do_fixed_dft<traits, cfg, l2fftsize, false>;
                specs[i].backward = &ng_do_fixed_dft<traits, cfg, l2fftsize, true>;
            }
            else
            {
                // FFT size not known at compile time, use variable-size routine
                specs[i].forward  = &ng_do_dft<traits, cfg, false>;
                specs[i].backward = &ng_do_dft<traits, cfg, true>;
            }
            specs[i].pre_real_backward = &ng_pre_real_backward<traits>;
            specs[i].post_real_forward = &ng_post_real_forward<traits>;
        }
        specs[i].real_twiddle_count = real_twiddle_count(l2fftsize);
        if (specs[i].real_twiddle_count)
            specs[i].real_initializer = &ng_do_init_real_dft<traits>;
    };

    return specs;
}

template <typename T_, dft_decomp dir_, uint8_t l2baseradix_, uint8_t l2maxradix_, uint8_t l2basewidth_,
          uint8_t l2basesplitwidth_ = 0, uint8_t l2minsplitwidth_ = l2basewidth_>
struct make_dft_traits
{
    using type                                = T_;
    constexpr static dft_decomp dir           = dir_;
    constexpr static uint8_t l2baseradix      = l2baseradix_;
    constexpr static uint8_t l2maxradix       = l2maxradix_;
    constexpr static uint8_t l2basewidth      = l2basewidth_;
    constexpr static uint8_t l2basesplitwidth = l2basesplitwidth_;
    constexpr static uint8_t l2minsplitwidth  = l2minsplitwidth_;
};

struct simd_arch
{
    uint8_t l2regbitwidth; // Log-2 of the number of bits in a SIMD register (e.g., 8 for AVX, 9 for AVX-512)
    uint8_t l2regcount; // Log-2 of the number of SIMD registers available (e.g., 4 for x86-64, 5 for
                        // AVX-512/ARM64)
};

constexpr inline simd_arch native_arch{
    uint8_t(ilog2(platform<>::native_float_vector_size * 8)),
    uint8_t(ilog2(platform<>::simd_register_count)),
};

constexpr inline std::array<simd_arch, 7> all_possible_archs = {
    simd_arch{ 7, 3 }, // sse on 32-bit
    simd_arch{ 7, 4 }, // sse on 64-bit, arm32
    simd_arch{ 7, 5 }, // arm64, rvv64
    simd_arch{ 8, 3 }, // avx on 32-bit
    simd_arch{ 8, 4 }, // avx on 64-bit
    simd_arch{ 9, 3 }, // avx-512 on 32-bit
    simd_arch{ 9, 5 }, // avx-512 on 64-bit
};

template <simd_arch arch, typename T_, dft_algorithm algo_>
struct make_dft_traits_for_arch
{
    using type                                  = T_;
    constexpr static uint8_t l2complex_bitwidth = std::countr_zero(sizeof(std::complex<T_>) * 8);
    constexpr static dft_algorithm algo         = algo_;
    constexpr static uint8_t l2baseradix        = 2;

    constexpr static uint8_t compute_l2maxradix() noexcept { return 3; }
    constexpr static uint8_t compute_l2maxsingleradix() noexcept
    {
        if constexpr (std::is_same_v<T_, float>)
            return 4;
        else
            return 3;
    }

    constexpr static uint8_t l2basewidth =
        arch.l2regbitwidth + arch.l2regcount - l2complex_bitwidth - 1 - l2baseradix;

    constexpr static uint8_t l2radixlimit = l2basewidth + l2baseradix;

    constexpr static uint8_t l2minsplitwidth  = arch.l2regbitwidth - l2complex_bitwidth + 1;
    constexpr static uint8_t l2basesplitwidth = l2basewidth >= l2minsplitwidth ? l2basewidth : 0;

    constexpr static uint8_t l2maxradix = std::min(compute_l2maxradix(), l2radixlimit);

    constexpr static uint8_t l2maxsingleradix = std::min(compute_l2maxsingleradix(), l2radixlimit);
};

namespace dft_internal
{

template <dft_traits traits>
constexpr inline auto specs = generate_dft_specializations<traits>();

} // namespace dft_internal

namespace impl
{

template <dft_traits traits>
static size_t ngfft_twiddle_count_internal(ngfft_plan<typename traits::type>& plan)
{
    using namespace dft_internal;

    if (plan.l2fftsize >= specs<traits>.size()) [[unlikely]]
    {
        return SIZE_MAX; // Invalid FFT size, return max size to indicate error
    }

    return specs<traits>[plan.l2fftsize].twiddle_count + specs<traits>[plan.l2fftsize].real_twiddle_count;
}

template <dft_traits traits>
static bool ngfft_initialize_internal(ngfft_plan<typename traits::type>& plan)
{
    using namespace dft_internal;
    if (plan.l2fftsize >= specs<traits>.size()) [[unlikely]]
    {
        return false; // Invalid FFT size, do nothing
    }

    if (specs<traits>[plan.l2fftsize].twiddle_count + specs<traits>[plan.l2fftsize].real_twiddle_count > 0 &&
        plan.twiddles == nullptr)
    {
        // User did not provide twiddle buffer, but specialization requires twiddles - cannot initialize
        return false;
    }

    if (specs<traits>[plan.l2fftsize].initializer) [[likely]]
        specs<traits>[plan.l2fftsize].initializer(plan, specs<traits>[plan.l2fftsize].config);
    if (specs<traits>[plan.l2fftsize].real_initializer) [[likely]]
        specs<traits>[plan.l2fftsize].real_initializer(
            ngfft_plan<typename traits::type>{ plan.l2fftsize,
                                               plan.twiddles + specs<traits>[plan.l2fftsize].twiddle_count },
            specs<traits>[plan.l2fftsize].config);
    return true;
}

template <dft_traits traits, bool inverse>
static void ngfft_execute_internal(const ngfft_plan<typename traits::type>& plan, cbool_t<inverse>,
                                   complex<typename traits::type>* out,
                                   const complex<typename traits::type>* in)
{
    using namespace dft_internal;
    if (plan.l2fftsize >= specs<traits>.size()) [[unlikely]]
    {
        return; // Invalid FFT size, do nothing
    }

    if constexpr (!inverse)
        specs<traits>[plan.l2fftsize].forward(plan, out, in);
    else
        specs<traits>[plan.l2fftsize].backward(plan, out, in);
}
template <dft_traits traits, bool inverse>
static void ngfft_real_execute_internal(const ngfft_plan<typename traits::type>& plan, cbool_t<inverse>,
                                        complex<typename traits::type>* out,
                                        const complex<typename traits::type>* in)
{
    using namespace dft_internal;
    if (plan.l2fftsize >= specs<traits>.size()) [[unlikely]]
    {
        return; // Invalid FFT size, do nothing
    }
    const ngfft_plan<typename traits::type> rplan{
        plan.l2fftsize, plan.twiddles + specs<traits>[plan.l2fftsize].twiddle_count
    };

    if constexpr (!inverse)
    {
        specs<traits>[plan.l2fftsize].forward(plan, out, in);
        specs<traits>[plan.l2fftsize].post_real_forward(rplan, out, out);
    }
    else
    {
        specs<traits>[plan.l2fftsize].pre_real_backward(rplan, out, in);
        specs<traits>[plan.l2fftsize].backward(plan, out, out);
    }
}

template <dft_traits traits>
const dft_specialization<typename traits::type, to_family(traits::algo)>& ngfft_spec_internal(
    uint8_t l2fftsize) noexcept
{
    using namespace dft_internal;
    return specs<traits>[l2fftsize];
}

template <dft_algorithm algo, typename T>
struct ngfft_traits : make_dft_traits_for_arch<native_arch, T, algo>
{
};

template <typename T, dft_algorithm algo>
size_t ngfft_twiddle_count(ngfft_plan<T>& plan, cval_t<dft_algorithm, algo>)
{
    using traits = ngfft_traits<algo, T>;

    return ngfft_twiddle_count_internal<traits>(plan);
}

template <typename T, dft_algorithm algo>
bool ngfft_initialize(ngfft_plan<T>& plan, cval_t<dft_algorithm, algo>)
{
    using traits = ngfft_traits<algo, T>;

    return ngfft_initialize_internal<traits>(plan);
}

template <typename T, dft_algorithm algo, bool inverse>
void ngfft_execute(const ngfft_plan<T>& plan, cval_t<dft_algorithm, algo>, cbool_t<inverse>, complex<T>* out,
                   const complex<T>* in)
{
    using traits = ngfft_traits<algo, T>;

    return ngfft_execute_internal<traits>(plan, cbool<inverse>, out, in);
}

template <typename T, dft_algorithm algo>
void ngfft_real_execute(const ngfft_plan<T>& plan, cval_t<dft_algorithm, algo>, complex<T>* out, const T* in)
{
    using traits = ngfft_traits<algo, T>;

    return ngfft_real_execute_internal<traits>(plan, cfalse, out, ptr_cast<complex<T>>(in));
}

template <typename T, dft_algorithm algo>
void ngfft_real_execute(const ngfft_plan<T>& plan, cval_t<dft_algorithm, algo>, T* out, const complex<T>* in)
{
    using traits = ngfft_traits<algo, T>;

    ngfft_real_execute_internal<traits>(plan, ctrue, ptr_cast<complex<T>>(out), in);
}

} // namespace impl

} // namespace KFR_ARCH_NAME

} // namespace kfr
