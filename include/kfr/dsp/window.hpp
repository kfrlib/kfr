/** @addtogroup window
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
#pragma once

#include "../base/handle.hpp"
#include "../math/log_exp.hpp"
#include "../math/modzerobessel.hpp"
#include "../math/sin_cos.hpp"
#include "../math/sqrt.hpp"
#include "../simd/vec.hpp"

namespace kfr
{

enum class window_type
{
    rectangular     = 1,
    triangular      = 2,
    bartlett        = 3,
    cosine          = 4,
    hann            = 5,
    bartlett_hann   = 6,
    hamming         = 7,
    bohman          = 8,
    blackman        = 9,
    blackman_harris = 10,
    kaiser          = 11,
    flattop         = 12,
    gaussian        = 13,
    lanczos         = 14,
    cosine_np       = 15,
    planck_taper    = 16,
    tukey           = 17,
};

template <window_type type>
using cwindow_type_t = cval_t<window_type, type>;

template <window_type type>
constexpr cwindow_type_t<type> cwindow_type{};

enum class window_symmetry
{
    periodic,
    symmetric
};

inline namespace CMT_ARCH_NAME
{

enum class window_metrics
{
    metrics_0_1,
    metrics_m1_1,
    metrics_mpi_pi,
    metrics_m1_1_trunc,
    metrics_m1_1_trunc2,
};

template <typename T>
struct window_linspace : expression_linspace<T>
{
    window_linspace(cval_t<window_metrics, window_metrics::metrics_0_1>, size_t size,
                    window_symmetry symmetry)
        : expression_linspace<T>{ 0, 1, size, symmetry == window_symmetry::symmetric }
    {
    }
    window_linspace(cval_t<window_metrics, window_metrics::metrics_m1_1>, size_t size,
                    window_symmetry symmetry)
        : expression_linspace<T>{ -1, 1, size, symmetry == window_symmetry::symmetric }
    {
    }
    window_linspace(cval_t<window_metrics, window_metrics::metrics_mpi_pi>, size_t size,
                    window_symmetry symmetry)
        : expression_linspace<T>{ -c_pi<T>, +c_pi<T>, size, symmetry == window_symmetry::symmetric }
    {
    }
    window_linspace(cval_t<window_metrics, window_metrics::metrics_m1_1_trunc>, size_t size,
                    window_symmetry symmetry)
        : expression_linspace<T>{ symmetric_linspace, calc_p(size, symmetry == window_symmetry::symmetric),
                                  size, symmetry == window_symmetry::symmetric }
    {
    }
    window_linspace(cval_t<window_metrics, window_metrics::metrics_m1_1_trunc2>, size_t size,
                    window_symmetry symmetry)
        : expression_linspace<T>{ symmetric_linspace, calc_p2(size, symmetry == window_symmetry::symmetric),
                                  size, symmetry == window_symmetry::symmetric }
    {
    }
    static T calc_p(size_t size, bool sym)
    {
        if (!sym)
            ++size;
        return T(size - 1) / (size);
    }
    static T calc_p2(size_t size, bool sym)
    {
        if (!sym)
            ++size;
        return (size & 1) ? T(size - 1) / T(size + 1) : T(size - 1) / (size);
    }
};

template <typename T>
struct expression_window : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 1;
    constexpr static shape<dims> get_shape(const expression_window<T>& self)
    {
        return shape<dims>(self.m_size);
    }
    constexpr static shape<dims> get_shape() { return shape<1>(undefined_size); }

    constexpr expression_window(size_t size) : m_size(size) {}

    size_t m_size;
    size_t size() const { return m_size; }
};

template <typename T>
struct expression_rectangular : expression_window<T>
{
    expression_rectangular(size_t size, T = T(), window_symmetry symmetry = window_symmetry::symmetric)
        : expression_window<T>(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_rectangular& self, shape<1> index,
                                                axis_params<0, N>)
    {
        using TI           = utype<T>;
        const vec<TI, N> i = enumerate(vec_shape<TI, N>()) + static_cast<TI>(index.front());
        return select(i < static_cast<TI>(self.m_size), T(1), T(0));
    }
};

template <typename T, window_metrics metrics>
struct expression_window_with_metrics : expression_window<T>
{
    expression_window_with_metrics(size_t size, T arg = T(),
                                   window_symmetry symmetry = window_symmetry::symmetric)
        : expression_window<T>(size), linspace(cval<window_metrics, metrics>, size, symmetry), arg(arg)
    {
    }

protected:
    window_linspace<T> linspace;
    T arg;
};

template <typename T>
struct expression_triangular : expression_window_with_metrics<T, window_metrics::metrics_m1_1_trunc2>
{
    using expression_window_with_metrics<T,
                                         window_metrics::metrics_m1_1_trunc2>::expression_window_with_metrics;

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_triangular& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        return 1 - abs(get_elements(self.linspace, index, sh));
    }
};

template <typename T>
struct expression_bartlett : expression_window_with_metrics<T, window_metrics::metrics_m1_1>
{
    using expression_window_with_metrics<T, window_metrics::metrics_m1_1>::expression_window_with_metrics;
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_bartlett& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        return 1 - abs(get_elements(self.linspace, index, sh));
    }
};

template <typename T>
struct expression_cosine : expression_window_with_metrics<T, window_metrics::metrics_0_1>
{
    using expression_window_with_metrics<T, window_metrics::metrics_0_1>::expression_window_with_metrics;

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_cosine& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        return sin(c_pi<T> * (get_elements(self.linspace, index, sh)));
    }
};
template <typename T>
struct expression_cosine_np : expression_window_with_metrics<T, window_metrics::metrics_m1_1_trunc>
{
    using expression_window_with_metrics<T,
                                         window_metrics::metrics_m1_1_trunc>::expression_window_with_metrics;

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_cosine_np& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        return sin(c_pi<T, 1, 2> * (1 + get_elements(self.linspace, index, sh)));
    }
};

template <typename T>
struct expression_hann : expression_window_with_metrics<T, window_metrics::metrics_0_1>
{
    using expression_window_with_metrics<T, window_metrics::metrics_0_1>::expression_window_with_metrics;

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_hann& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        return T(0.5) * (T(1) - cos(c_pi<T, 2> * get_elements(self.linspace, index, sh)));
    }
};

template <typename T>
struct expression_bartlett_hann : expression_window_with_metrics<T, window_metrics::metrics_0_1>
{
    using expression_window_with_metrics<T, window_metrics::metrics_0_1>::expression_window_with_metrics;

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_bartlett_hann& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        const vec<T, N> xx = get_elements(self.linspace, index, sh);
        return T(0.62) - T(0.48) * abs(xx - T(0.5)) + T(0.38) * cos(c_pi<T, 2> * (xx - T(0.5)));
    }
};

template <typename T>
struct expression_hamming : expression_window_with_metrics<T, window_metrics::metrics_0_1>
{
    expression_hamming(size_t size, T alpha = 0.54, window_symmetry symmetry = window_symmetry::symmetric)
        : expression_window_with_metrics<T, window_metrics::metrics_0_1>(size, alpha, symmetry)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_hamming& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        return self.arg - (T(1.0) - self.arg) * (cos(c_pi<T, 2> * get_elements(self.linspace, index, sh)));
    }
};

template <typename T>
struct expression_bohman : expression_window_with_metrics<T, window_metrics::metrics_m1_1>
{
    using expression_window_with_metrics<T, window_metrics::metrics_m1_1>::expression_window_with_metrics;

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_bohman& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        const vec<T, N> n = abs(get_elements(self.linspace, index, sh));
        return (T(1) - n) * cos(c_pi<T> * n) + (T(1) / c_pi<T>)*sin(c_pi<T> * n);
    }
};

template <typename T>
struct expression_blackman : expression_window_with_metrics<T, window_metrics::metrics_0_1>
{
    using expression_window_with_metrics<T, window_metrics::metrics_0_1>::expression_window_with_metrics;

    expression_blackman(size_t size, T alpha = 0.16, window_symmetry symmetry = window_symmetry::symmetric)
        : expression_window_with_metrics<T, window_metrics::metrics_0_1>(size, alpha, symmetry),
          a0((1 - alpha) * 0.5), a1(0.5), a2(alpha * 0.5)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_blackman& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        const vec<T, N> n = get_elements(self.linspace, index, sh);
        return self.a0 - self.a1 * cos(c_pi<T, 2> * n) + self.a2 * cos(c_pi<T, 4> * n);
    }

private:
    T a0, a1, a2;
};

template <typename T>
struct expression_blackman_harris : expression_window_with_metrics<T, window_metrics::metrics_0_1>
{
    using expression_window_with_metrics<T, window_metrics::metrics_0_1>::expression_window_with_metrics;

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_blackman_harris& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        const vec<T, N> n = get_elements(self.linspace, index, sh) * c_pi<T, 2>;
        return T(0.35875) - T(0.48829) * cos(n) + T(0.14128) * cos(2 * n) - T(0.01168) * cos(3 * n);
    }
};

template <typename T>
struct expression_kaiser : expression_window_with_metrics<T, window_metrics::metrics_m1_1>
{
    expression_kaiser(size_t size, T beta = 0.5, window_symmetry symmetry = window_symmetry::symmetric)
        : expression_window_with_metrics<T, window_metrics::metrics_m1_1>(size, beta, symmetry),
          m(reciprocal(modzerobessel(make_vector(beta))[0]))
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_kaiser& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        return modzerobessel(self.arg * sqrt(1 - sqr(get_elements(self.linspace, index, sh)))) * self.m;
    }

private:
    T m;
};

template <typename T>
struct expression_flattop : expression_window_with_metrics<T, window_metrics::metrics_0_1>
{
    using expression_window_with_metrics<T, window_metrics::metrics_0_1>::expression_window_with_metrics;

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_flattop& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        const vec<T, N> n = get_elements(self.linspace, index, sh) * c_pi<T, 2>;
        constexpr T a0    = 0.21557895;
        constexpr T a1    = 0.41663158;
        constexpr T a2    = 0.277263158;
        constexpr T a3    = 0.083578947;
        constexpr T a4    = 0.006947368;
        return a0 - a1 * cos(n) + a2 * cos(2 * n) - a3 * cos(3 * n) + a4 * cos(4 * n);
    }
};

template <typename T>
struct expression_gaussian : expression_window_with_metrics<T, window_metrics::metrics_m1_1_trunc>
{
    /// alpha = std / 2N
    expression_gaussian(size_t size, T alpha = 2.5, window_symmetry symmetry = window_symmetry::symmetric)
        : expression_window_with_metrics<T, window_metrics::metrics_m1_1_trunc>(size, alpha, symmetry)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_gaussian& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        return exp(T(-0.5) * sqr(self.arg * get_elements(self.linspace, index, sh)));
    }
};

template <typename T>
struct expression_lanczos : expression_window_with_metrics<T, window_metrics::metrics_mpi_pi>
{
    using expression_window_with_metrics<T, window_metrics::metrics_mpi_pi>::expression_window_with_metrics;
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_lanczos& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        return sinc(get_elements(self.linspace, index, sh));
    }
};

template <typename T>
struct expression_planck_taper : expression_window_with_metrics<T, window_metrics::metrics_m1_1>
{
    expression_planck_taper(size_t size, T epsilon, window_symmetry symmetry = window_symmetry::symmetric)
        : expression_window_with_metrics<T, window_metrics::metrics_m1_1>(size, epsilon, symmetry)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_planck_taper& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        vec<T, N> x   = (T(1) - abs(get_elements(self.linspace, index, sh))) / (T(2) * self.arg);
        vec<T, N> val = T(1) / (T(1) + exp(T(1) / x - T(1) / (T(1) - x)));
        return select(x <= T(0), T(0), select(x >= T(1), T(1), val));
    }
};

template <typename T>
struct expression_tukey : expression_window_with_metrics<T, window_metrics::metrics_m1_1>
{
    expression_tukey(size_t size, T epsilon, window_symmetry symmetry = window_symmetry::symmetric)
        : expression_window_with_metrics<T, window_metrics::metrics_m1_1>(size, epsilon, symmetry)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_tukey& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        vec<T, N> x   = (T(1) - abs(get_elements(self.linspace, index, sh))) / self.arg;
        vec<T, N> val = T(0.5) * (T(1) - cos(c_pi<T> * x));
        return select(x <= T(0), T(0), select(x >= T(1), T(1), val));
    }
};

template <window_type>
struct window_by_type;

#define KFR_WINDOW_BY_TYPE(win)                                                                              \
    template <>                                                                                              \
    struct window_by_type<window_type::win>                                                                  \
    {                                                                                                        \
        template <typename T>                                                                                \
        using type = expression_##win<T>;                                                                    \
    };
KFR_WINDOW_BY_TYPE(rectangular)
KFR_WINDOW_BY_TYPE(triangular)
KFR_WINDOW_BY_TYPE(bartlett)
KFR_WINDOW_BY_TYPE(cosine)
KFR_WINDOW_BY_TYPE(hann)
KFR_WINDOW_BY_TYPE(bartlett_hann)
KFR_WINDOW_BY_TYPE(hamming)
KFR_WINDOW_BY_TYPE(bohman)
KFR_WINDOW_BY_TYPE(blackman)
KFR_WINDOW_BY_TYPE(blackman_harris)
KFR_WINDOW_BY_TYPE(kaiser)
KFR_WINDOW_BY_TYPE(flattop)
KFR_WINDOW_BY_TYPE(gaussian)
KFR_WINDOW_BY_TYPE(lanczos)
KFR_WINDOW_BY_TYPE(cosine_np)
KFR_WINDOW_BY_TYPE(planck_taper)
KFR_WINDOW_BY_TYPE(tukey)
#undef KFR_WINDOW_BY_TYPE

/**
 * @brief Returns template expression that generates Rrectangular window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_rectangular<T> window_rectangular(size_t size, ctype_t<T> = ctype_t<T>())
{
    return expression_rectangular<T>(size, T());
}

/**
 * @brief Returns template expression that generates Triangular window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_triangular<T> window_triangular(size_t size, ctype_t<T> = ctype_t<T>())
{
    return expression_triangular<T>(size);
}

/**
 * @brief Returns template expression that generates Bartlett window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_bartlett<T> window_bartlett(size_t size, ctype_t<T> = ctype_t<T>())
{
    return expression_bartlett<T>(size);
}

/**
 * @brief Returns template expression that generates Cosine window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_cosine<T> window_cosine(size_t size, ctype_t<T> = ctype_t<T>())
{
    return expression_cosine<T>(size);
}

/**
 * @brief Returns template expression that generates Cosine window (numpy compatible) of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_cosine_np<T> window_cosine_np(size_t size, ctype_t<T> = ctype_t<T>())
{
    return expression_cosine_np<T>(size);
}

/**
 * @brief Returns template expression that generates Hann window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_hann<T> window_hann(size_t size, ctype_t<T> = ctype_t<T>())
{
    return expression_hann<T>(size);
}

/**
 * @brief Returns template expression that generates Bartlett-Hann window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_bartlett_hann<T> window_bartlett_hann(size_t size, ctype_t<T> = ctype_t<T>())
{
    return expression_bartlett_hann<T>(size);
}

/**
 * @brief Returns template expression that generates Hamming window of length @c size where &alpha; = @c
 * alpha
 */
template <typename T = fbase>
KFR_FUNCTION expression_hamming<T> window_hamming(size_t size, identity<T> alpha = 0.54,
                                                  ctype_t<T> = ctype_t<T>())
{
    return expression_hamming<T>(size, alpha);
}

/**
 * @brief Returns template expression that generates Bohman window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_bohman<T> window_bohman(size_t size, ctype_t<T> = ctype_t<T>())
{
    return expression_bohman<T>(size);
}

/**
 * @brief Returns template expression that generates Blackman window of length @c size where &alpha; = @c
 * alpha
 */
template <typename T = fbase>
KFR_FUNCTION expression_blackman<T> window_blackman(size_t size, identity<T> alpha = 0.16,
                                                    window_symmetry symmetry = window_symmetry::symmetric,
                                                    ctype_t<T>               = ctype_t<T>())
{
    return expression_blackman<T>(size, alpha, symmetry);
}

/**
 * @brief Returns template expression that generates Blackman-Harris window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_blackman_harris<T> window_blackman_harris(
    size_t size, window_symmetry symmetry = window_symmetry::symmetric, ctype_t<T> = ctype_t<T>())
{
    return expression_blackman_harris<T>(size, T(), symmetry);
}

/**
 * @brief Returns template expression that generates Kaiser window of length @c size where &beta; = @c
 * beta
 */
template <typename T = fbase>
KFR_FUNCTION expression_kaiser<T> window_kaiser(size_t size, identity<T> beta = T(0.5),
                                                ctype_t<T> = ctype_t<T>())
{
    return expression_kaiser<T>(size, beta);
}

/**
 * @brief Returns template expression that generates Flat top window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_flattop<T> window_flattop(size_t size, ctype_t<T> = ctype_t<T>())
{
    return expression_flattop<T>(size);
}

/**
 * @brief Returns template expression that generates Gaussian window of length @c size where &alpha; = @c
 * alpha
 */
template <typename T = fbase>
KFR_FUNCTION expression_gaussian<T> window_gaussian(size_t size, identity<T> alpha = 2.5,
                                                    ctype_t<T> = ctype_t<T>())
{
    return expression_gaussian<T>(size, alpha);
}

/**
 * @brief Returns template expression that generates Lanczos window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_lanczos<T> window_lanczos(size_t size, ctype_t<T> = ctype_t<T>())
{
    return expression_lanczos<T>(size);
}

/**
 * @brief Returns template expression that generates Planck-taper window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION expression_planck_taper<T> window_planck_taper(
    size_t size, identity<T> epsilon, window_symmetry symmetry = window_symmetry::symmetric,
    ctype_t<T> = ctype_t<T>())
{
    return expression_planck_taper<T>(size, epsilon, symmetry);
}

/**
 * @brief Returns template expression that generates Tukey window of length @c size (numpy compatible)
 */
template <typename T = fbase>
KFR_FUNCTION expression_tukey<T> window_tukey(size_t size, identity<T> alpha,
                                              window_symmetry symmetry = window_symmetry::symmetric,
                                              ctype_t<T>               = ctype_t<T>())
{
    return expression_tukey<T>(size, alpha, symmetry);
}

template <typename T           = fbase, window_type type,
          typename window_expr = typename window_by_type<type>::template type<T>>
CMT_NOINLINE window_expr window(size_t size, cval_t<window_type, type>, identity<T> win_param = T(),
                                window_symmetry symmetry = window_symmetry::symmetric,
                                ctype_t<T>               = ctype_t<T>())
{
    return window_expr(size, win_param, symmetry);
}

template <typename T = fbase>
CMT_NOINLINE expression_handle<T> window(size_t size, window_type type, identity<T> win_param,
                                         window_symmetry symmetry = window_symmetry::symmetric,
                                         ctype_t<T>               = ctype_t<T>())
{
    return cswitch(
        cvals_t<window_type, window_type::rectangular, window_type::triangular, window_type::bartlett,
                window_type::cosine, window_type::hann, window_type::bartlett_hann, window_type::hamming,
                window_type::bohman, window_type::blackman, window_type::blackman_harris, window_type::kaiser,
                window_type::flattop, window_type::gaussian, window_type::lanczos, window_type::cosine_np,
                window_type::planck_taper, window_type::tukey>(),
        type,
        [size, win_param, symmetry](auto win)
        {
            constexpr window_type window = val_of(decltype(win)());
            return to_handle(typename window_by_type<window>::template type<T>(size, win_param, symmetry));
        },
        fn_generic::returns<expression_handle<T>>());
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
