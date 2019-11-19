/** @addtogroup basic_math
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

#include "scaled.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T, int maximum = 1>
struct color
{
    constexpr static T min = std::numeric_limits<T>::min();
    constexpr static T max = std::numeric_limits<T>::max();

    using vec_type = vec<T, 4>;

    static_assert(std::is_floating_point<T>::value || std::is_same<T, uint8_t>::value ||
                      std::is_same<T, uint16_t>::value,
                  "Incorrect type");

    constexpr color(int) = delete;
    constexpr explicit color(T grey, T alpha = maximum) : v(grey, grey, grey, alpha) {}
    constexpr color(T r, T g, T b, T a = maximum) : v(r, g, b, a) {}
    constexpr color(const color&) = default;
    constexpr color(const vec<T, 4>& v) : v(v) {}
    constexpr color(const vec<T, 3>& v, T a = maximum) : v(concat(v, vec<T, 1>(a))) {}
    constexpr color(const vec<T, 4>& v, T a) : v(concat(slice<0, 3>(v), vec<T, 1>(a))) {}

    // @brief Convert from 0xAARRGGBB representation (BGRA in little endian)
    static constexpr color from_argb(uint32_t c)
    {
        return color(convert_scaled<T, maximum, 255>(bitcast<u8>(u32x1(static_cast<uint32_t>(c))))
                         .shuffle(csizes_t<2, 1, 0, 3>()));
    }
    template <typename U, int Umax>
    operator color<U, Umax>() const
    {
        return convert_scaled<U, Umax, maximum>(v);
    }
    constexpr color() = default;

    constexpr color lighter(double n = 1.2) const noexcept { return color(v * vec<T, 4>(n), a); }
    constexpr color darker(double n = 1.2) const noexcept { return color(v / vec<T, 4>(n), a); }

    constexpr T lightness() const
    {
        using Tcommon = conditional<std::is_floating_point<T>::value, T, findinttype<min * 3, max * 3>>;
        return (Tcommon(r) + g + b) / 3;
    }

    constexpr T lightness_perc() const
    {
        return static_cast<T>(sqrt(0.299 * r * r + 0.587 * g * g + 0.114 * b * b));
    }

    constexpr color normalize() const { return v / lightness_perc(); }

    constexpr void apply_red(T r) noexcept { this->r = r; }
    constexpr void apply_green(T g) noexcept { this->g = g; }
    constexpr void apply_blue(T b) noexcept { this->b = b; }
    constexpr void apply_alpha(T a) noexcept { this->a = a; }

    constexpr color with_red(T r) const noexcept
    {
        return blend(v, broadcast<4>(r), elements_t<1, 0, 0, 0>());
    }
    constexpr color with_green(T g) const noexcept
    {
        return blend(v, broadcast<4>(g), elements_t<0, 1, 0, 0>());
    }
    constexpr color with_blue(T b) const noexcept
    {
        return blend(v, broadcast<4>(b), elements_t<0, 0, 1, 0>());
    }
    constexpr color with_alpha(T a) const noexcept
    {
        return blend(v, broadcast<4>(a), elements_t<0, 0, 0, 1>());
    }
    constexpr color with_alpha_premul(T a) const noexcept { return v * a; }

    constexpr bool operator==(const color& c) const { return all(v == c.v); }
    constexpr bool operator!=(const color& c) const { return !(*this == c); }

    union {
        struct
        {
            T r;
            T g;
            T b;
            T a;
        };
        struct
        {
            T red;
            T green;
            T blue;
            T alpha;
        };
        struct
        {
            vec_type v;
        };
    };
};

using f32color = color<f32>;
using u8color  = color<u8, 255>;

CMT_INTRINSIC f32color mix(float t, const f32color& a, const f32color& b) { return kfr::mix(t, a.v, b.v); }

CMT_INTRINSIC f32color from_srgb_approx(f32color color)
{
    float a = color.a;
    color.v = color.v * (color.v * (color.v * 0.305306011f + 0.682171111f) + 0.012522878f);
    color.a = a;
    return color;
}

CMT_INTRINSIC f32color to_srgb_approx(f32color color)
{
    float a  = color.a;
    f32x4 S1 = kfr::sqrt(color.v);
    f32x4 S2 = kfr::sqrt(S1);
    f32x4 S3 = kfr::sqrt(S2);
    color    = 0.585122381f * S1 + 0.783140355f * S2 - 0.368262736f * S3;
    color.a  = a;
    return color;
}

constexpr inline f32color operator""_argb(unsigned long long argb) { return f32color::from_argb(argb); }

constexpr inline f32color operator""_rgb(unsigned long long rgb)
{
    return f32color::from_argb(0xFF000000u | rgb);
}

}
} // namespace kfr

namespace cometa
{
template <typename T, int maximum>
struct representation<kfr::color<T, maximum>>
{
    using type = std::string;
    static std::string get(const kfr::color<T, maximum>& value) noexcept
    {
        return as_string("color(", value.r, ", ", value.g, ", ", value.b, ", ", value.a, ")");
    }
};
} // namespace cometa
