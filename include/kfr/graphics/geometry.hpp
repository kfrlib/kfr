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
inline namespace CMT_ARCH_NAME
{

template <typename T, int maximum = 1>
struct point
{
    constexpr point() noexcept = default;
    constexpr point(const vec<T, 2>& v) noexcept : v(v) {}
    constexpr point(T x, T y) noexcept : v(x, y) {}

    template <typename U, int Umax>
    operator point<U, Umax>() const
    {
        return convert_scaled<U, Umax, maximum>(v);
    }
    constexpr bool operator==(const point& c) const { return all(v == c.v); }
    constexpr bool operator!=(const point& c) const { return !operator==(c); }

    constexpr friend point operator+(const point& p1, const point& p2) { return p1.v + p2.v; }
    constexpr friend point operator-(const point& p1, const point& p2) { return p1.v - p2.v; }
    constexpr friend point operator*(const point& p1, const point& p2) { return p1.v * p2.v; }
    constexpr friend point operator-(const point& p1) { return -p1.v; }

    constexpr point flipped() const { return kfr::swap(v); }

    T distance(const point& pt) const { return std::sqrt(sqr(pt.x - x) + sqr(pt.y - y)); }

    T operator[](size_t i) const { return v[i]; }
    T& operator[](size_t i) { return a[i]; }

    constexpr point round() const { return kfr::round(v); }
    constexpr point floor() const { return kfr::floor(v); }
    constexpr point ceil() const { return kfr::ceil(v); }
    constexpr point trunc() const { return kfr::trunc(v); }

    union
    {
        struct
        {
            T x;
            T y;
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

template <typename T, int maximum = 1>
struct size
{
    constexpr size() noexcept = default;
    constexpr size(T x, T y) noexcept : v(x, y) {}
    constexpr explicit size(T xy) noexcept : v(xy, xy) {}
    constexpr size(const vec<T, 2>& v) noexcept : v(v) {}

    template <typename U, int Umax>
    operator size<U, Umax>() const noexcept
    {
        return convert_scaled<U, Umax, maximum>(v);
    }

    constexpr size round() const { return kfr::round(v); }
    constexpr size floor() const { return kfr::floor(v); }
    constexpr size ceil() const { return kfr::ceil(v); }
    constexpr size trunc() const { return kfr::trunc(v); }

    constexpr size flipped() const { return kfr::swap(v); }

    constexpr friend size operator+(const size& s1, const size& s2) { return s1.v + s2.v; }
    constexpr friend size operator-(const size& s1, const size& s2) { return s1.v - s2.v; }
    constexpr friend size operator*(const size& s1, const size& s2) { return s1.v * s2.v; }
    constexpr friend size operator*(const size& s1, T s2) { return s1.v * s2; }
    constexpr friend size operator*(T s1, const size& s2) { return s1 * s2.v; }
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

template <typename T, int maximum = 1>
struct border
{
    constexpr border() noexcept : v() {}
    constexpr explicit border(T value) noexcept : v(value) {}
    constexpr border(T h, T v) noexcept : v(h, v, h, v) {}
    constexpr border(T x1, T y1, T x2, T y2) noexcept : v(x1, y1, x2, y2) {}

    constexpr border round() const { return kfr::round(v); }
    constexpr border floor() const { return kfr::floor(v); }
    constexpr border ceil() const { return kfr::ceil(v); }
    constexpr border trunc() const { return kfr::trunc(v); }

    kfr::size<T> size() const { return { x1 + x2, y1 + y2 }; }

    T horizontal() const { return x1 + x2; }
    T vertical() const { return y1 + y2; }

    point<T> starting() const { return { x1, y1 }; }
    point<T> trailing() const { return { x2, y2 }; }

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
    };
};

using i32border = border<i32>;
using f32border = border<f32>;

template <typename T, int maximum = 1>
struct vector4
{
    constexpr vector4(T x, T y, T z, T w) : v(x, y, z, w) {}
    constexpr vector4(const vec<T, 4>& v) : v(v) {}
    constexpr vector4() = default;

    constexpr bool operator==(const vector4& c) const { return all(v == c.v); }
    constexpr bool operator!=(const vector4& c) const { return !operator==(c); }

    union
    {
        struct
        {
            T x;
            T y;
            T z;
            T w;
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
    };
};

template <typename T, int maximum = 1>
struct rectangle
{
    constexpr rectangle(T x1, T y1, T x2, T y2) : v(x1, y1, x2, y2) {}
    constexpr rectangle(const vec<T, 4>& v) : v(v) {}

    template <typename U, int Umax>
    operator rectangle<U, Umax>() const
    {
        return convert_scaled<U, Umax, maximum>(v);
    }
    constexpr rectangle(const point<T>& point, const size<T>& size) : v(point.v, point.v + size.v) {}
    constexpr rectangle(const point<T>& point1, const point<T>& point2) : v(point1.v, point2.v) {}
    constexpr rectangle(const point<T>& base, const kfr::size<T>& dim, const point<T>& alignment)
        : v(base.v - dim.v * alignment.v, base.v + dim.v * (1 - alignment.v))
    {
    }
    constexpr rectangle() = default;

    constexpr bool empty() const noexcept { return width() <= 0 || height() <= 0; }

    constexpr kfr::size<T, maximum> size() const { return high(v) - low(v); }

    constexpr T area() const { return size().area(); }
    constexpr T width() const { return x2 - x1; }
    constexpr T height() const { return y2 - y1; }

    constexpr T min_side() const { return std::min(width(), height()); }
    constexpr T max_side() const { return std::max(width(), height()); }

    point<T, maximum> center() const { return at(0.5f, 0.5f); }

    point<T> to_norm_coord(const point<T>& pt) const { return (pt.v - p1.v) / (p1.v - p1.v); }
    point<T> to_norm_coord(const point<T>& pt, const point<T>& ifoutside) const
    {
        if (!contains(pt))
            return ifoutside;
        return (pt.v - p1.v) / (p2.v - p1.v);
    }

    rectangle split(const point<float>& point1, const kfr::size<float>& size) const noexcept
    {
        const vec<float, 2> point2 = point1.v + size.v;
        return concat(cast<T>(p1.v + this->size().v * point1.v), cast<T>(p1.v + this->size().v * point2));
    }
    rectangle split(float x, float y, float w, float h) const noexcept { return split({ x, y }, { w, h }); }

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

    point<T> at(const point<float>& pt) const noexcept { return p1.v + point<T>(pt.v * size().v); }
    point<T> at(float x, float y) const noexcept
    {
        return p1.v + point<T>(point<float>{ x, y }.v * size().v);
    }

    void apply_start(const point<T, maximum>& p) { v = concat(p.v, p.v + size()); }
    void apply_start(T x, T y) { v = concat(pack(x, y), pack(x, y) + size()); }
    void apply_size(const kfr::size<T, maximum>& s) { v = concat(low(v), low(v) + s.v); }
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

    rectangle aligned_rect(const kfr::size<T>& inner_size, const point<float>& alignment) const
    {
        const kfr::size<T> sz  = inner_size; // kfr::min(inner_size.v, this->size().v);
        const kfr::size<T> gap = size() - sz;
        const vec<T, 2> p      = p1.v + cast<T>(gap.v * alignment.v);
        return rectangle(concat(p, p + sz.v));
    }
    rectangle aligned_rect(T width, T height, float align_x, float align_y) const
    {
        return aligned_rect({ width, height }, { align_x, align_y });
    }

    constexpr rectangle with_start(const point<T, maximum>& p) const { return concat(p.v, p.v + size()); }
    constexpr rectangle with_start(T x, T y) const { return concat(pack(x, y), pack(x, y) + size()); }
    constexpr rectangle with_size(const kfr::size<T, maximum>& s) const
    {
        return concat(low(v), low(v) + s.v);
    }
    constexpr rectangle with_size(T w, T h) const { return concat(low(v), low(v) + pack(w, h)); }
    constexpr rectangle with_width(T w) const { return with_size(w, height()); }
    constexpr rectangle with_height(T h) const { return with_size(width(), h); }
    constexpr rectangle with_offset(const point<T>& p) const { return v + vec<T, 4>(p.v, p.v); }
    constexpr rectangle with_offset(T x, T y) const { return v + vec<T, 4>(x, y, x, y); }
    constexpr rectangle with_scale(T x, T y) const { return v * vec<T, 4>(x, y, x, y); }
    constexpr rectangle with_margin(T h, T v) const { return this->v + vec<T, 4>(-h, -v, +h, +v); }
    constexpr rectangle with_padding(T h, T v) const { return this->v + vec<T, 4>(+h, +v, -h, -v); }
    constexpr rectangle with_padding(T x1, T y1, T x2, T y2) const
    {
        return v + vec<T, 4>(+x1, +y1, -x2, -y2);
    }
    constexpr rectangle with_margin(T m) const { return v + vec<T, 4>(-m, -m, +m, +m); }
    constexpr rectangle with_padding(T p) const { return v + vec<T, 4>(+p, +p, -p, -p); }

    constexpr rectangle with_padding(const border<T>& p) const { return v + vec<T, 4>(1, 1, -1, -1) * p.v; }
    constexpr rectangle with_margin(const border<T>& m) const { return v + vec<T, 4>(-1, -1, 1, 1) * m.v; }

    constexpr rectangle flipped() const { return kfr::swap(v); }

    constexpr rectangle round() const { return kfr::round(v); }
    constexpr rectangle floor() const { return kfr::floor(v); }
    constexpr rectangle ceil() const { return kfr::ceil(v); }
    constexpr rectangle trunc() const { return kfr::trunc(v); }

    bool contains(const point<T, maximum>& pt) const { return all(pt.v >= p1.v && pt.v < p2.v); }

    bool operator<<(const point<T, maximum>& pt) const { return contains(pt); }

    constexpr bool operator==(const rectangle& c) const { return all(v == c.v); }
    constexpr bool operator!=(const rectangle& c) const { return !(*this == c); }

    constexpr rectangle operator&(const rectangle& c) const
    {
        return blend<0, 0, 1, 1>(min(v, c.v), max(v, c.v));
    }
    constexpr rectangle operator|(const rectangle& c) const
    {
        return blend<1, 1, 0, 0>(min(v, c.v), max(v, c.v));
    }

    rectangle& operator&=(const rectangle& c) { return *this = *this & c; }
    rectangle& operator|=(const rectangle& c) { return *this = *this | c; }

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
    };
};

using f32rectangle = rectangle<f32>;
using f32point     = point<f32>;
using i32point     = point<i32>;
using f32size      = size<f32>;
using i32size      = size<i32>;
using i32rectangle = rectangle<i32>;
using i32vector4   = vector4<i32>;
using f32vector4   = vector4<f32>;

template <typename T>
CMT_INTRINSIC size<T> min(const size<T>& a, const size<T>& b)
{
    return { min(a.x, b.x), min(a.y, b.y) };
}
template <typename T>
CMT_INTRINSIC size<T> max(const size<T>& a, const size<T>& b)
{
    return { max(a.x, b.x), max(a.y, b.y) };
}

/// @brief 2D matrix
template <typename T>
struct matrix
{
    union
    {
        vec<T, 6> v;
        struct
        {
            T a, b, c, d, e, f;
        };
    };

    matrix() : v{ 1, 0, 0, 1, 0, 0 } {}

    matrix(T a, T b, T c, T d, T e, T f) : v{ a, b, c, d, e, f } {}

    static matrix translate(T x, T y) { return matrix{ 1, 0, 0, 1, x, y }; }
    static matrix scale(T x, T y) { return matrix{ x, 0, 0, y, 0, 0 }; }
    static matrix rotate(T angle) { return matrix{ cos(angle), sin(angle), -sin(angle), cos(angle), 0, 0 }; }

    static matrix rotate90(int angle)
    {
        static const matrix m[4] = {
            { 1, 0, 0, 1, 0, 0 }, { 0, 1, -1, 0, 0, 0 }, { -1, 0, 0, -1, 0, 0 }, { 0, -1, 1, 0, 0, 0 }
        };
        return m[angle % 4];
    }

    std::array<std::array<T, 3>, 3> full() const
    {
        return { { { a, b, T() }, { c, d, T() }, { e, f, T(1) } } };
    }

    friend matrix<T> operator*(const matrix<T>& m, const matrix<T>& n)
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

    friend point<T> operator*(const point<T>& pt, const matrix<T>& m)
    {
        return { pt.x * m.a + pt.y * m.c + m.e, pt.y * m.d + pt.x * m.b + m.f };
    }
};

using f32matrix = matrix<f32>;
} // namespace CMT_ARCH_NAME
} // namespace kfr

namespace cometa
{
template <typename T, int maximum>
struct representation<kfr::point<T, maximum>>
{
    using type = std::string;
    static std::string get(const kfr::point<T, maximum>& value) noexcept
    {
        return as_string("point(", value.x, ", ", value.y, ")");
    }
};
template <typename T, int maximum>
struct representation<kfr::size<T, maximum>>
{
    using type = std::string;
    static std::string get(const kfr::size<T, maximum>& value) noexcept
    {
        return as_string("size(", value.x, ", ", value.y, ")");
    }
};
template <typename T, int maximum>
struct representation<kfr::border<T, maximum>>
{
    using type = std::string;
    static std::string get(const kfr::border<T, maximum>& value) noexcept
    {
        return as_string("border(", value.x1, ", ", value.y1, ", ", value.x2, ", ", value.y2, ")");
    }
};
template <typename T, int maximum>
struct representation<kfr::rectangle<T, maximum>>
{
    using type = std::string;
    static std::string get(const kfr::rectangle<T, maximum>& value) noexcept
    {
        return as_string("rectangle(", value.x1, ", ", value.y1, ", ", value.x2, ", ", value.y2, ")");
    }
};
} // namespace cometa
