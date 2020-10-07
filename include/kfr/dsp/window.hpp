/** @addtogroup window
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
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

#include "../base/pointer.hpp"
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

namespace internal
{

template <typename T>
struct window_linspace_0_1 : expression_linspace<T>
{
    window_linspace_0_1(size_t size, window_symmetry symmetry)
        : expression_linspace<T>(0, 1, size, symmetry == window_symmetry::symmetric)
    {
    }
};

template <typename T>
struct window_linspace_m1_1 : expression_linspace<T>
{
    window_linspace_m1_1(size_t size, window_symmetry symmetry)
        : expression_linspace<T>(-1, 1, size, symmetry == window_symmetry::symmetric)
    {
    }
};

template <typename T>
struct window_linspace_mpi_pi : expression_linspace<T>
{
    window_linspace_mpi_pi(size_t size, window_symmetry symmetry)
        : expression_linspace<T>(-c_pi<T>, +c_pi<T>, size, symmetry == window_symmetry::symmetric)
    {
    }
};

template <typename T>
struct window_linspace_m1_1_trunc : expression_linspace<T>
{
    window_linspace_m1_1_trunc(size_t size, window_symmetry symmetry)
        : expression_linspace<T>(-T(size - 1) / size, T(size - 1) / size, size,
                                 symmetry == window_symmetry::symmetric)
    {
    }
};

template <typename T>
struct window_linspace_m1_1_trunc2 : expression_linspace<T>
{
    window_linspace_m1_1_trunc2(size_t size, window_symmetry symmetry)
        : expression_linspace<T>(symmetric_linspace,
                                 (size & 1) ? T(size - 1) / T(size + 1) : T(size - 1) / (size), size,
                                 symmetry == window_symmetry::symmetric)
    {
    }
};

template <typename T>
struct expression_rectangular : input_expression
{
    using value_type = T;

    expression_rectangular(size_t size, T = T(), window_symmetry = window_symmetry::symmetric) : m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_rectangular& self, cinput_t, size_t index,
                                                vec_shape<T, N>)
    {
        using TI           = utype<T>;
        const vec<TI, N> i = enumerate(vec_shape<TI, N>()) + static_cast<TI>(index);
        return select(i < static_cast<TI>(self.m_size), T(1), T(0));
    }
    size_t size() const { return m_size; }

private:
    size_t m_size;
};

template <typename T>
struct expression_triangular : input_expression
{
    using value_type = T;

    expression_triangular(size_t size, T = T(), window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_triangular& self, cinput_t cinput,
                                                size_t index, vec_shape<T, N> y)
    {
        return 1 - abs(get_elements(self.linspace, cinput, index, y));
    }
    size_t size() const { return m_size; }

private:
    window_linspace_m1_1_trunc2<T> linspace;
    size_t m_size;
};

template <typename T>
struct expression_bartlett : input_expression
{
    using value_type = T;

    expression_bartlett(size_t size, T = T(), window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_bartlett& self, cinput_t cinput,
                                                size_t index, vec_shape<T, N> y)
    {
        return 1 - abs(get_elements(self.linspace, cinput, index, y));
    }
    size_t size() const { return m_size; }

private:
    window_linspace_m1_1<T> linspace;
    size_t m_size;
};

template <typename T>
struct expression_cosine : input_expression
{
    using value_type = T;

    expression_cosine(size_t size, T = T(), window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_cosine& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        return sin(c_pi<T> * get_elements(self.linspace, cinput, index, y));
    }
    size_t size() const { return m_size; }

private:
    window_linspace_0_1<T> linspace;
    size_t m_size;
};

template <typename T>
struct expression_hann : input_expression
{
    using value_type = T;

    expression_hann(size_t size, T = T(), window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_hann& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        return T(0.5) * (T(1) - cos(c_pi<T, 2> * get_elements(self.linspace, cinput, index, y)));
    }
    size_t size() const { return m_size; }

private:
    window_linspace_0_1<T> linspace;
    size_t m_size;
};

template <typename T>
struct expression_bartlett_hann : input_expression
{
    using value_type = T;

    expression_bartlett_hann(size_t size, T = T(), window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_bartlett_hann& self, cinput_t cinput,
                                                size_t index, vec_shape<T, N> y)
    {
        const vec<T, N> xx = get_elements(self.linspace, cinput, index, y);
        return T(0.62) - T(0.48) * abs(xx - T(0.5)) + T(0.38) * cos(c_pi<T, 2> * (xx - T(0.5)));
    }
    size_t size() const { return m_size; }

private:
    window_linspace_0_1<T> linspace;
    size_t m_size;
};

template <typename T>
struct expression_hamming : input_expression
{
    using value_type = T;

    expression_hamming(size_t size, T alpha = 0.54, window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), alpha(alpha), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_hamming& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        return self.alpha -
               (T(1.0) - self.alpha) * (cos(c_pi<T, 2> * get_elements(self.linspace, cinput, index, y)));
    }
    size_t size() const { return m_size; }

private:
    window_linspace_0_1<T> linspace;
    T alpha;
    size_t m_size;
};

template <typename T>
struct expression_bohman : input_expression
{
    using value_type = T;

    expression_bohman(size_t size, T = T(), window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_bohman& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        const vec<T, N> n = abs(get_elements(self.linspace, cinput, index, y));
        return (T(1) - n) * cos(c_pi<T> * n) + (T(1) / c_pi<T>)*sin(c_pi<T> * n);
    }
    size_t size() const { return m_size; }

private:
    window_linspace_m1_1<T> linspace;
    size_t m_size;
};

template <typename T>
struct expression_blackman : input_expression
{
    using value_type = T;

    expression_blackman(size_t size, T alpha = 0.16, window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), a0((1 - alpha) * 0.5), a1(0.5), a2(alpha * 0.5), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_blackman& self, cinput_t cinput,
                                                size_t index, vec_shape<T, N> y)
    {
        const vec<T, N> n = get_elements(self.linspace, cinput, index, y);
        return self.a0 - self.a1 * cos(c_pi<T, 2> * n) + self.a2 * cos(c_pi<T, 4> * n);
    }
    size_t size() const { return m_size; }

private:
    window_linspace_0_1<T> linspace;
    T a0, a1, a2;
    size_t m_size;
};

template <typename T>
struct expression_blackman_harris : input_expression
{
    using value_type = T;

    expression_blackman_harris(size_t size, T = T(), window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_blackman_harris& self, cinput_t cinput,
                                                size_t index, vec_shape<T, N> y)
    {
        const vec<T, N> n = get_elements(self.linspace, cinput, index, y) * c_pi<T, 2>;
        return T(0.35875) - T(0.48829) * cos(n) + T(0.14128) * cos(2 * n) - T(0.01168) * cos(3 * n);
    }
    size_t size() const { return m_size; }

private:
    window_linspace_0_1<T> linspace;
    size_t m_size;
};

template <typename T>
struct expression_kaiser : input_expression
{
    using value_type = T;

    expression_kaiser(size_t size, T beta = 0.5, window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), beta(beta), m(reciprocal(modzerobessel(make_vector(beta))[0])),
          m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_kaiser& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        return modzerobessel(self.beta * sqrt(1 - sqr(get_elements(self.linspace, cinput, index, y)))) *
               self.m;
    }
    size_t size() const { return m_size; }

private:
    window_linspace_m1_1<T> linspace;
    T beta;
    T m;
    size_t m_size;
};

template <typename T>
struct expression_flattop : input_expression
{
    using value_type = T;

    expression_flattop(size_t size, T = T(), window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_flattop& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        const vec<T, N> n = get_elements(self.linspace, cinput, index, y) * c_pi<T, 2>;
        constexpr T a0    = 0.21557895;
        constexpr T a1    = 0.41663158;
        constexpr T a2    = 0.277263158;
        constexpr T a3    = 0.083578947;
        constexpr T a4    = 0.006947368;
        return a0 - a1 * cos(n) + a2 * cos(2 * n) - a3 * cos(3 * n) + a4 * cos(4 * n);
    }
    size_t size() const { return m_size; }

private:
    window_linspace_0_1<T> linspace;
    size_t m_size;
};

template <typename T>
struct expression_gaussian : input_expression
{
    using value_type = T;

    expression_gaussian(size_t size, T alpha = 2.5, window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), alpha(alpha), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_gaussian& self, cinput_t cinput,
                                                size_t index, vec_shape<T, N> y)
    {
        return exp(T(-0.5) * sqr(self.alpha * get_elements(self.linspace, cinput, index, y)));
    }

    size_t size() const { return m_size; }

private:
    window_linspace_m1_1_trunc<T> linspace;
    T alpha;
    size_t m_size;
};

template <typename T>
struct expression_lanczos : input_expression
{
    using value_type = T;

    expression_lanczos(size_t size, T alpha = 2.5, window_symmetry symmetry = window_symmetry::symmetric)
        : linspace(size, symmetry), alpha(alpha), m_size(size)
    {
    }
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_lanczos& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        return sinc(get_elements(self.linspace, cinput, index, y));
    }
    size_t size() const { return m_size; }

private:
    window_linspace_mpi_pi<T> linspace;
    T alpha;
    size_t m_size;
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
#undef KFR_WINDOW_BY_TYPE
} // namespace internal

/**
 * @brief Returns template expression that generates Rrectangular window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_rectangular<T> window_rectangular(size_t size, ctype_t<T> = ctype_t<T>())
{
    return internal::expression_rectangular<T>(size, T());
}

/**
 * @brief Returns template expression that generates Triangular window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_triangular<T> window_triangular(size_t size, ctype_t<T> = ctype_t<T>())
{
    return internal::expression_triangular<T>(size);
}

/**
 * @brief Returns template expression that generates Bartlett window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_bartlett<T> window_bartlett(size_t size, ctype_t<T> = ctype_t<T>())
{
    return internal::expression_bartlett<T>(size);
}

/**
 * @brief Returns template expression that generates Cosine window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_cosine<T> window_cosine(size_t size, ctype_t<T> = ctype_t<T>())
{
    return internal::expression_cosine<T>(size);
}

/**
 * @brief Returns template expression that generates Hann window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_hann<T> window_hann(size_t size, ctype_t<T> = ctype_t<T>())
{
    return internal::expression_hann<T>(size);
}

/**
 * @brief Returns template expression that generates Bartlett-Hann window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_bartlett_hann<T> window_bartlett_hann(size_t size,
                                                                        ctype_t<T> = ctype_t<T>())
{
    return internal::expression_bartlett_hann<T>(size);
}

/**
 * @brief Returns template expression that generates Hamming window of length @c size where &alpha; = @c
 * alpha
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_hamming<T> window_hamming(size_t size, identity<T> alpha = 0.54,
                                                            ctype_t<T> = ctype_t<T>())
{
    return internal::expression_hamming<T>(size, alpha);
}

/**
 * @brief Returns template expression that generates Bohman window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_bohman<T> window_bohman(size_t size, ctype_t<T> = ctype_t<T>())
{
    return internal::expression_bohman<T>(size);
}

/**
 * @brief Returns template expression that generates Blackman window of length @c size where &alpha; = @c
 * alpha
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_blackman<T> window_blackman(
    size_t size, identity<T> alpha = 0.16, window_symmetry symmetry = window_symmetry::symmetric,
    ctype_t<T> = ctype_t<T>())
{
    return internal::expression_blackman<T>(size, alpha, symmetry);
}

/**
 * @brief Returns template expression that generates Blackman-Harris window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_blackman_harris<T> window_blackman_harris(
    size_t size, window_symmetry symmetry = window_symmetry::symmetric, ctype_t<T> = ctype_t<T>())
{
    return internal::expression_blackman_harris<T>(size, T(), symmetry);
}

/**
 * @brief Returns template expression that generates Kaiser window of length @c size where &beta; = @c
 * beta
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_kaiser<T> window_kaiser(size_t size, identity<T> beta = T(0.5),
                                                          ctype_t<T> = ctype_t<T>())
{
    return internal::expression_kaiser<T>(size, beta);
}

/**
 * @brief Returns template expression that generates Flat top window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_flattop<T> window_flattop(size_t size, ctype_t<T> = ctype_t<T>())
{
    return internal::expression_flattop<T>(size);
}

/**
 * @brief Returns template expression that generates Gaussian window of length @c size where &alpha; = @c
 * alpha
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_gaussian<T> window_gaussian(size_t size, identity<T> alpha = 2.5,
                                                              ctype_t<T> = ctype_t<T>())
{
    return internal::expression_gaussian<T>(size, alpha);
}

/**
 * @brief Returns template expression that generates Lanczos window of length @c size
 */
template <typename T = fbase>
KFR_FUNCTION internal::expression_lanczos<T> window_lanczos(size_t size, ctype_t<T> = ctype_t<T>())
{
    return internal::expression_lanczos<T>(size);
}

template <typename T           = fbase, window_type type,
          typename window_expr = typename internal::window_by_type<type>::template type<T>>
CMT_NOINLINE window_expr window(size_t size, cval_t<window_type, type>, identity<T> win_param = T(),
                                window_symmetry symmetry = window_symmetry::symmetric,
                                ctype_t<T>               = ctype_t<T>())
{
    return window_expr(size, win_param, symmetry);
}

template <typename T = fbase>
CMT_NOINLINE expression_pointer<T> window(size_t size, window_type type, identity<T> win_param,
                                          window_symmetry symmetry = window_symmetry::symmetric,
                                          ctype_t<T>               = ctype_t<T>())
{
    return cswitch(
        cvals_t<window_type, window_type::rectangular, window_type::triangular, window_type::bartlett,
                window_type::cosine, window_type::hann, window_type::bartlett_hann, window_type::hamming,
                window_type::bohman, window_type::blackman, window_type::blackman_harris, window_type::kaiser,
                window_type::flattop, window_type::gaussian, window_type::lanczos>(),
        type,
        [size, win_param, symmetry](auto win) {
            constexpr window_type window = val_of(decltype(win)());
            return to_pointer(
                typename internal::window_by_type<window>::template type<T>(size, win_param, symmetry));
        },
        fn_generic::returns<expression_pointer<T>>());
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
