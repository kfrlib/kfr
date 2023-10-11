/** @addtogroup basic_math
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

#include "impl/scaled.hpp"

namespace kfr
{

template <typename T>
struct point;
template <typename T>
struct size;
template <typename T>
struct border;
template <typename T>
struct rectangle;
template <typename T>
struct matrix2d;

struct percents
{
    double value;
    template <typename T>
    inline constexpr T calc(T total) const
    {
        return static_cast<T>(total * value / 100.0);
    }
};

constexpr inline percents operator""_perc(long double v) { return { static_cast<double>(v) }; }
constexpr inline percents operator""_perc(unsigned long long v) { return { static_cast<double>(v) }; }

template <typename T>
struct point
{
    using Tfloat = std::common_type_t<T, float>;

    constexpr point() noexcept : v() {}
    constexpr explicit point(const vec<T, 2>& v) noexcept : v(v) {}
    constexpr point(T x, T y) noexcept : v(x, y) {}
    constexpr point(const size<T>& sz) noexcept : v(sz.v) {}
#if CMT_COMPILER_IS_MSVC
    constexpr point(const point& p) noexcept: v(p.v) {}
#else
    constexpr point(const point& p) noexcept = default;
#endif

    template <typename U>
    operator point<U>() const
    {
        return point<U>(convert_scaled<U, 1, 1>(v));
    }
    constexpr bool operator==(const point& c) const { return all(v == c.v); }
    constexpr bool operator!=(const point& c) const { return !operator==(c); }

    constexpr friend point operator+(const point& p1, const point& p2) { return point(p1.v + p2.v); }
    constexpr friend point operator-(const point& p1, const point& p2) { return point(p1.v - p2.v); }
    constexpr friend point operator*(const point& p1, const point& p2) { return point(p1.v * p2.v); }
    constexpr friend point operator/(const point& p1, const point& p2) { return point(p1.v / p2.v); }
    constexpr friend point operator-(const point& p1) { return point(-p1.v); }
    constexpr friend point operator*(const point& p1, const T& v2) { return point(p1.v * v2); }
    constexpr friend point operator*(const T& v1, const point& p2) { return point(v1 * p2.v); }
    constexpr friend point operator/(const point& p1, const T& v2) { return point(p1.v / v2); }
    constexpr friend point operator/(const T& v1, const point& p2) { return point(v1 / p2.v); }

    constexpr point flipped() const { return point(kfr::swap(v)); }

    T distance(const point& pt) const { return std::sqrt(sqr(pt.x - x) + sqr(pt.y - y)); }

    T manhattan_distance(const point& pt) const { return std::max(std::abs(pt.x - x), std::abs(pt.y - y)); }

    T operator[](size_t i) const { return v[i]; }
    T& operator[](size_t i) { return a[i]; }

    rectangle<T> aligned_rect(const kfr::size<T>& inner_size, const point<Tfloat>& alignment) const
    {
        const kfr::size<T> sz  = inner_size;
        const kfr::size<T> gap = -sz;
        const vec<T, 2> p      = v + cast<T>(gap.v * alignment.v);
        return rectangle(concat(p, p + sz.v));
    }
    rectangle<T> aligned_rect(T width, T height, Tfloat align_x, Tfloat align_y) const
    {
        return aligned_rect({ width, height }, { align_x, align_y });
    }

    constexpr point round() const { return point(kfr::round(v)); }
    constexpr point floor() const { return point(kfr::floor(v)); }
    constexpr point ceil() const { return point(kfr::ceil(v)); }
    constexpr point trunc() const { return point(kfr::trunc(v)); }

    union
    {
        struct
        {
            vec<T, 2> v;
        };
        struct
        {
            T x;
            T y;
        };
        struct
        {
            T a[2];
        };
    };
};

template <typename T>
CMT_INLINE point<T> min(const point<T>& a, const point<T>& b)
{
    return point<T>(min(a.v, b.v));
}
template <typename T>
CMT_INLINE point<T> max(const point<T>& a, const point<T>& b)
{
    return point<T>(max(a.v, b.v));
}

template <typename T>
struct size
{
    constexpr size() noexcept : v() {}
    constexpr size(T x, T y) noexcept : v(x, y) {}
    constexpr explicit size(T xy) noexcept : v(xy, xy) {}
    constexpr size(const vec<T, 2>& v) noexcept : v(v) {}
#if CMT_COMPILER_IS_MSVC
    constexpr size(const size& p) noexcept: v(p.v) {}
#else
    constexpr size(const size& p) noexcept = default;
#endif

    template <typename U>
    operator size<U>() const noexcept
    {
        return size<U>(convert_scaled<U, 1, 1>(v));
    }

    constexpr size round() const { return size(kfr::round(v)); }
    constexpr size floor() const { return size(kfr::floor(v)); }
    constexpr size ceil() const { return size(kfr::ceil(v)); }
    constexpr size trunc() const { return size(kfr::trunc(v)); }

    constexpr size flipped() const { return size(kfr::swap(v)); }

    constexpr friend size operator+(const size& s1, const size& s2) { return size(s1.v + s2.v); }
    constexpr friend size operator-(const size& s1, const size& s2) { return size(s1.v - s2.v); }
    constexpr friend size operator*(const size& s1, const size& s2) { return size(s1.v * s2.v); }
    constexpr friend size operator/(const size& s1, const size& s2) { return size(s1.v / s2.v); }
    constexpr friend size operator*(const size& s1, T s2) { return size(s1.v * s2); }
    constexpr friend size operator*(T s1, const size& s2) { return size(s1 * s2.v); }
    constexpr friend size operator/(const size& s1, T s2) { return size(s1.v / s2); }
    constexpr friend size operator/(T s1, const size& s2) { return size(s1 / s2.v); }
    constexpr friend size operator-(const size& s1) { return size(-s1.v); }
    constexpr T area() const { return x * y; }

    T operator[](size_t i) const { return v[i]; }
    T& operator[](size_t i) { return a[i]; }

    constexpr bool operator==(const size& c) const { return all(v == c.v); }
    constexpr bool operator!=(const size& c) const { return !operator==(c); }
    union
    {
        struct
        {
            T x;
            T y;
        };
        struct
        {
            T width;
            T height;
        };
        struct
        {
            vec<T, 2> v;
        };
        struct
        {
            T a[2];
        };
    };
};

template <typename T>
CMT_INLINE size<T> min(const size<T>& a, const size<T>& b)
{
    return size<T>(min(a.v, b.v));
}
template <typename T>
CMT_INLINE size<T> max(const size<T>& a, const size<T>& b)
{
    return size<T>(max(a.v, b.v));
}

template <typename T>
struct border
{
    constexpr border() noexcept : v() {}
    constexpr explicit border(T value) noexcept : v(value) {}
    constexpr border(T h, T v) noexcept : v(h, v, h, v) {}
    constexpr border(T x1, T y1, T x2, T y2) noexcept : v(x1, y1, x2, y2) {}
    constexpr explicit border(const vec<T, 4>& v) : v(v) {}
#if CMT_COMPILER_IS_MSVC
    constexpr border(const border& p) noexcept: v(p.v) {}
#else
    constexpr border(const border& p) noexcept = default;
#endif

    template <typename U>
    operator border<U>() const
    {
        return border<U>(convert_scaled<U, 1, 1>(v));
    }

    constexpr border round() const { return border(kfr::round(v)); }
    constexpr border floor() const { return border(kfr::floor(v)); }
    constexpr border ceil() const { return border(kfr::ceil(v)); }
    constexpr border trunc() const { return border(kfr::trunc(v)); }

    kfr::size<T> size() const { return kfr::size<T>(low(v) + high(v)); }

    T horizontal() const { return size().x; }
    T vertical() const { return size().y; }

    point<T> leading() const { return point<T>(slice<0, 2>(v)); }
    point<T> trailing() const { return point<T>(slice<2, 2>(v)); }

    T operator[](size_t i) const { return v[i]; }
    T& operator[](size_t i) { return a[i]; }

    constexpr friend border operator+(const border& b1, const border& b2) { return border(b1.v + b2.v); }

    constexpr bool operator==(const border& c) const { return all(v == c.v); }
    constexpr bool operator!=(const border& c) const { return !(operator==(c)); }
    union
    {
        struct
        {
            T x1;
            T y1;
            T x2;
            T y2;
        };
        struct
        {
            vec<T, 4> v;
        };
        struct
        {
            T a[4];
        };
    };
};

template <typename T>
CMT_INLINE border<T> min(const border<T>& a, const border<T>& b)
{
    return border<T>(min(a.v, b.v));
}
template <typename T>
CMT_INLINE border<T> max(const border<T>& a, const border<T>& b)
{
    return border<T>(max(a.v, b.v));
}

template <typename T>
struct rectangle
{
    using Tfloat = std::common_type_t<T, float>;

    constexpr rectangle(T x1, T y1, T x2, T y2) : v(x1, y1, x2, y2) {}
    constexpr explicit rectangle(const vec<T, 4>& v) : v(v) {}
#if CMT_COMPILER_IS_MSVC
    constexpr rectangle(const rectangle& p) noexcept: v(p.v) {}
#else
    constexpr rectangle(const rectangle& p) noexcept = default;
#endif

    template <typename U>
    operator rectangle<U>() const
    {
        return rectangle<U>(convert_scaled<U, 1, 1>(v));
    }
    constexpr rectangle(const point<T>& point, const size<T>& size) : v(point.v, point.v + size.v) {}
    constexpr rectangle(const point<T>& point1, const point<T>& point2) : v(point1.v, point2.v) {}
    constexpr rectangle(const point<T>& base, const kfr::size<T>& dim, const point<Tfloat>& alignment)
        : rectangle(base.aligned_rect(dim, alignment))
    {
    }
    constexpr rectangle() : v() {}

    constexpr bool empty() const noexcept { return any(size().v <= 0); }

    constexpr kfr::size<T> size() const { return high(v) - low(v); }

    constexpr T area() const { return size().area(); }
    constexpr T width() const { return x2 - x1; }
    constexpr T height() const { return y2 - y1; }

    constexpr T min_side() const { return hmin(size().v); }
    constexpr T max_side() const { return hmax(size().v); }

    point<T> center() const { return at(0.5f, 0.5f); }

    point<Tfloat> to_norm_coord(const point<T>& pt) const { return point<T>((pt.v - p1.v) / (p1.v - p1.v)); }
    point<Tfloat> to_norm_coord(const point<T>& pt, const point<T>& ifoutside) const
    {
        if (!contains(pt))
            return ifoutside;
        return point<T>((pt.v - p1.v) / (p2.v - p1.v));
    }

    rectangle split(const point<Tfloat>& point1, const kfr::size<Tfloat>& size) const noexcept
    {
        const vec<Tfloat, 2> point2 = point1.v + size.v;
        return rectangle(
            concat(cast<T>(p1.v + this->size().v * point1.v), cast<T>(p1.v + this->size().v * point2)));
    }
    rectangle split(Tfloat x, Tfloat y, Tfloat w, Tfloat h) const noexcept { return split({ x, y }, { w, h }); }

    rectangle cut_h_start(T width, bool partial = false)
    {
        if (empty() || (!partial && this->width() < width))
            return rectangle();
        width                  = std::min(width, this->width());
        const rectangle result = with_width(width);
        apply_padding(width, 0, 0, 0);
        return result;
    }
    rectangle cut_v_start(T height, bool partial = false)
    {
        if (empty() || (!partial && this->height() < height))
            return rectangle();
        height                 = std::min(height, this->height());
        const rectangle result = with_height(height);
        apply_padding(0, height, 0, 0);
        return result;
    }
    rectangle cut_h_end(T width, bool partial = false)
    {
        if (empty() || (!partial && this->width() < width))
            return rectangle();
        width                  = std::min(width, this->width());
        width                  = this->width() - width;
        const rectangle result = with_padding(width, 0, 0, 0);
        apply_width(width);
        return result;
    }
    rectangle cut_v_end(T height, bool partial = false)
    {
        if (empty() || (!partial && this->height() < height))
            return rectangle();
        height                 = std::min(height, this->height());
        height                 = this->height() - height;
        const rectangle result = with_padding(0, height, 0, 0);
        apply_height(height);
        return result;
    }

    rectangle cut_h_start(percents width) { return cut_h_start(width.calc(this->width())); }
    rectangle cut_v_start(percents height) { return cut_v_start(height.calc(this->height())); }
    rectangle cut_h_end(percents width) { return cut_h_end(width.calc(this->width())); }
    rectangle cut_v_end(percents height) { return cut_v_end(height.calc(this->height())); }

    point<T> at(const point<Tfloat>& pt) const noexcept { return p1 + point<T>(pt.v * size().v); }
    point<T> at(Tfloat x, Tfloat y) const noexcept
    {
        return p1 + point<T>(point<Tfloat>{ x, y }.v * size().v);
    }

    void apply_start(const point<T>& p) { v = concat(p.v, p.v + size()); }
    void apply_start(T x, T y) { v = concat(pack(x, y), pack(x, y) + size()); }
    void apply_size(const kfr::size<T>& s) { v = concat(low(v), low(v) + s.v); }
    void apply_size(T w, T h) { v = concat(low(v), low(v) + pack(w, h)); }
    void apply_width(T w) { apply_size(w, height()); }
    void apply_height(T h) { apply_size(width(), h); }
    void apply_offset(T x, T y) { v += vec<T, 4>(x, y, x, y); }
    void apply_offset(const point<T>& p) { v += repeat<2>(p.v); }
    void apply_scale(T x, T y) { v *= vec<T, 4>(x, y, x, y); }
    void apply_margin(T h, T v) { v += vec<T, 4>(-h, -v, +h, +v); }
    void apply_padding(T h, T v) { v += vec<T, 4>(+h, +v, -h, -v); }
    void apply_margin(T m) { v += vec<T, 4>(-m, -m, +m, +m); }
    void apply_padding(T p) { v += vec<T, 4>(+p, +p, -p, -p); }
    void apply_margin(const border<T>& m) { v += vec<T, 4>(-1, -1, 1, 1) * m.v; }
    void apply_padding(const border<T>& p) { v += vec<T, 4>(1, 1, -1, -1) * p.v; }
    void apply_padding(T x1, T y1, T x2, T y2) { v += vec<T, 4>(+x1, +y1, -x2, -y2); }

    rectangle aligned_rect(const kfr::size<T>& inner_size, const point<Tfloat>& alignment) const
    {
        const kfr::size<T> sz  = inner_size; // kfr::min(inner_size.v, this->size().v);
        const kfr::size<T> gap = size() - sz;
        const vec<T, 2> p      = p1.v + cast<T>(gap.v * alignment.v);
        return rectangle(concat(p, p + sz.v));
    }
    rectangle aligned_rect(T width, T height, Tfloat align_x, Tfloat align_y) const
    {
        return aligned_rect({ width, height }, { align_x, align_y });
    }

    constexpr rectangle with_start(const point<T>& p) const { return rectangle(concat(p.v, p.v + size())); }
    constexpr rectangle with_start(T x, T y) const
    {
        return rectangle(concat(pack(x, y), pack(x, y) + size()));
    }
    constexpr rectangle with_size(const kfr::size<T>& s) const
    {
        return rectangle(concat(low(v), low(v) + s.v));
    }
    constexpr rectangle with_size(T w, T h) const { return rectangle(concat(low(v), low(v) + pack(w, h))); }
    constexpr rectangle with_width(T w) const { return with_size(w, height()); }
    constexpr rectangle with_height(T h) const { return with_size(width(), h); }
    constexpr rectangle with_offset(const point<T>& p) const { return rectangle(v + vec<T, 4>(p.v, p.v)); }
    constexpr rectangle with_offset(T x, T y) const { return rectangle(v + vec<T, 4>(x, y, x, y)); }
    constexpr rectangle with_scale(T x, T y) const { return rectangle(v * vec<T, 4>(x, y, x, y)); }
    constexpr rectangle with_margin(T h, T v) const { return rectangle(this->v + vec<T, 4>(-h, -v, +h, +v)); }
    constexpr rectangle with_padding(T h, T v) const
    {
        return rectangle(this->v + vec<T, 4>(+h, +v, -h, -v));
    }
    constexpr rectangle with_padding(T x1, T y1, T x2, T y2) const
    {
        return rectangle(v + vec<T, 4>(+x1, +y1, -x2, -y2));
    }
    constexpr rectangle with_margin(T m) const { return rectangle(v + vec<T, 4>(-m, -m, +m, +m)); }
    constexpr rectangle with_padding(T p) const { return rectangle(v + vec<T, 4>(+p, +p, -p, -p)); }

    constexpr rectangle with_padding(const border<T>& p) const
    {
        return rectangle(v + vec<T, 4>(1, 1, -1, -1) * p.v);
    }
    constexpr rectangle with_margin(const border<T>& m) const
    {
        return rectangle(v + vec<T, 4>(-1, -1, 1, 1) * m.v);
    }

    constexpr rectangle flipped() const { return rectangle(kfr::swap(v)); }

    constexpr rectangle round() const { return rectangle(kfr::round(v)); }
    constexpr rectangle floor() const { return rectangle(kfr::floor(v)); }
    constexpr rectangle ceil() const { return rectangle(kfr::ceil(v)); }
    constexpr rectangle trunc() const { return rectangle(kfr::trunc(v)); }

    bool contains(const point<T>& pt) const { return all(pt.v >= p1.v && pt.v < p2.v); }

    bool operator<<(const point<T>& pt) const { return contains(pt); }

    constexpr bool operator==(const rectangle& c) const { return all(v == c.v); }
    constexpr bool operator!=(const rectangle& c) const { return !(*this == c); }

    constexpr rectangle union_(const rectangle& c) const
    {
        return rectangle(blend<0, 0, 1, 1>(min(v, c.v), max(v, c.v)));
    }
    constexpr rectangle intersection(const rectangle& c) const
    {
        return rectangle(blend<1, 1, 0, 0>(min(v, c.v), max(v, c.v)));
    }

    T operator[](size_t i) const { return v[i]; }
    T& operator[](size_t i) { return a[i]; }

    union
    {
        struct
        {
            T x1;
            T y1;
            T x2;
            T y2;
        };
        struct
        {
            point<T> p1;
            point<T> p2;
        };
        struct
        {
            vec<T, 4> v;
        };
        struct
        {
            T a[4];
        };
    };
};

/// @brief 2D matrix
template <typename T>
struct matrix2d
{
    static_assert(std::is_floating_point_v<T>);
    union
    {
        vec<T, 6> v;
        struct
        {
            T a, b, c, d, e, f;
        };
    };

    matrix2d() : v{ 1, 0, 0, 1, 0, 0 } {}

    matrix2d(T a, T b, T c, T d, T e, T f) : v{ a, b, c, d, e, f } {}
    constexpr matrix2d(const matrix2d& m) : v(m.v) {}

    explicit matrix2d(const vec<T, 6>& v) : v(v) {}

    static matrix2d translate(T x, T y) { return matrix2d{ 1, 0, 0, 1, x, y }; }
    static matrix2d scale(T x, T y) { return matrix2d{ x, 0, 0, y, 0, 0 }; }
    static matrix2d rotate(T angle)
    {
        return matrix2d{ cos(angle), sin(angle), -sin(angle), cos(angle), 0, 0 };
    }

    static matrix2d rotate90(int angle)
    {
        static constexpr portable_vec<T, 6> m[4] = {
            { 1, 0, 0, 1, 0, 0 }, { 0, 1, -1, 0, 0, 0 }, { -1, 0, 0, -1, 0, 0 }, { 0, -1, 1, 0, 0, 0 }
        };
        return matrix2d(vec<T, 6>(m[angle % 4]));
    }

    std::array<std::array<T, 3>, 3> full() const
    {
        return { { { a, b, T() }, { c, d, T() }, { e, f, T(1) } } };
    }

    friend matrix2d<T> operator*(const matrix2d<T>& m, const matrix2d<T>& n)
    {
        const std::array<std::array<T, 3>, 3> mm = m.full();
        const std::array<std::array<T, 3>, 3> nn = n.full();
        std::array<std::array<T, 3>, 3> a;
        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < 3; j++)
            {
                T sum = 0;
                for (size_t k = 0; k < 3; k++)
                {
                    sum += mm[i][k] * nn[k][j];
                }
                a[i][j] = sum;
            }
        }
        return { a[0][0], a[0][1], a[1][0], a[1][1], a[2][0], a[2][1] };
    }

    friend point<T> operator*(const point<T>& pt, const matrix2d<T>& m)
    {
        return { pt.x * m.a + pt.y * m.c + m.e, pt.y * m.d + pt.x * m.b + m.f };
    }
};

using i32point = point<i32>;
using f32point = point<f32>;
using f64point = point<f64>;

using i32size = size<i32>;
using f32size = size<f32>;
using f64size = size<f64>;

using i32border = border<i32>;
using f32border = border<f32>;
using f64border = border<f64>;

using i32rectangle = rectangle<i32>;
using f32rectangle = rectangle<f32>;
using f64rectangle = rectangle<f64>;

using f32matrix2d = matrix2d<f32>;
using f64matrix2d = matrix2d<f64>;
} // namespace kfr

namespace cometa
{
template <typename T>
struct representation<kfr::point<T>>
{
    using type = std::string;
    static std::string get(const kfr::point<T>& value) noexcept
    {
        return as_string("point(", value.x, ", ", value.y, ")");
    }
};
template <typename T>
struct representation<kfr::size<T>>
{
    using type = std::string;
    static std::string get(const kfr::size<T>& value) noexcept
    {
        return as_string("size(", value.x, ", ", value.y, ")");
    }
};
template <typename T>
struct representation<kfr::border<T>>
{
    using type = std::string;
    static std::string get(const kfr::border<T>& value) noexcept
    {
        return as_string("border(", value.x1, ", ", value.y1, ", ", value.x2, ", ", value.y2, ")");
    }
};
template <typename T>
struct representation<kfr::rectangle<T>>
{
    using type = std::string;
    static std::string get(const kfr::rectangle<T>& value) noexcept
    {
        return as_string("rectangle(", value.x1, ", ", value.y1, ", ", value.x2, ", ", value.y2, ")");
    }
};
} // namespace cometa
