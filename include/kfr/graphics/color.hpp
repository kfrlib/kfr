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
struct color
{
    constexpr static T min = std::numeric_limits<T>::min();
    constexpr static T max = std::numeric_limits<T>::max();

    using vec_type = vec<T, 4>;

    static_assert(std::is_floating_point_v<T> || std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t>,
                  "Incorrect type");

    constexpr color(int) = delete;
    constexpr explicit color(T grey, T alpha = maximum) : v(grey, grey, grey, alpha) {}
    constexpr color(T r, T g, T b, T a = maximum) : v(r, g, b, a) {}
#if defined(_MSC_VER) && !defined(__clang__)
    // MSVC Internal Compiler Error workaround
    constexpr color(const color& value) : v(value.v) {}
    constexpr color& operator=(const color& value)
    {
        v = value.v;
        return *this;
    }
#else
    constexpr color(const color&) = default;
#endif
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
        using Tcommon = std::conditional_t<std::is_floating_point_v<T>, T, findinttype<min * 3, max * 3>>;
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

    union
    {
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

CMT_INTRINSIC f32x3 lab_to_xyz(const f32x3& lab)
{
    const float y = (lab[0] + 16.f) / 116.f;
    const float x = lab[1] / 500.f + y;
    const float z = y - lab[2] / 200.f;

    const f32x3 w = { x, y, z };

    const f32x3 cube = w * w * w;
    return f32x3{ 0.95047f, 1.00000f, 1.08883f } *
           (select(cube > 216.f / 24389.f, cube, (w - 16.f / 116.f) / (24389.f / 27.f / 116.f)));
}

CMT_INTRINSIC f32x3 xyz_to_rgb(const f32x3& xyz)
{
    return xyz[0] * f32x3{ 3.2406f, -0.9689f, 0.0557f } + xyz[1] * f32x3{ -1.5372f, 1.8758f, -0.2040f } +
           xyz[2] * f32x3{ -0.4986f, 0.0415f, 1.0570f };
}

CMT_INTRINSIC f32x3 rgb_to_srgb(const f32x3& rgb)
{
    return select(rgb > 0.0031308f, 1.055f * kfr::pow(rgb, 1.f / 2.4f) - 0.055f, 12.92f * rgb);
}

CMT_INTRINSIC f32x3 srgb_to_rgb(const f32x3& srgb)
{
    return select(srgb > 0.04045f, kfr::pow((srgb + 0.055f) / 1.055f, 2.4f), srgb / 12.92f);
}

CMT_INTRINSIC f32x3 rgb_to_xyz(const f32x3& rgb)
{
    return (rgb[0] * f32x3{ 0.4124f, 0.2126f, 0.0193f } + rgb[1] * f32x3{ 0.3576f, 0.7152f, 0.1192f } +
            rgb[2] * f32x3{ 0.1805f, 0.0722f, 0.9505f }) /
           f32x3{ 0.95047f, 1.f, 1.08883f };
}

CMT_INTRINSIC f32x3 xyz_to_lab(const f32x3& xyz)
{
    const f32x3 w = select(xyz > 0.008856f, kfr::pow(xyz, 0.33333f), (7.787f * xyz) + 16.f / 116.f);

    return { (116.f * w[1]) - 16.f, 500.f * (w[0] - w[1]), 200.f * (w[1] - w[2]) };
}

//

CMT_INTRINSIC f32color lab_to_srgb(const f32x3& lab, float alpha = 1.f)
{
    const f32x3 srgb = clamp(rgb_to_srgb(xyz_to_rgb(lab_to_xyz(lab))), 0.f, 1.f);
    return f32x4(srgb, f32x1{ alpha });
}

CMT_INTRINSIC f32color lab_to_linear_rgb(const f32x3& lab, float alpha = 1.f)
{
    const f32x3 rgb = clamp(xyz_to_rgb(lab_to_xyz(lab)), 0.f, 1.f);
    return f32x4(rgb, f32x1{ alpha });
}

CMT_INTRINSIC f32x3 srgb_to_lab(const f32color& srgb)
{
    return xyz_to_lab(rgb_to_xyz(srgb_to_rgb(slice<0, 3>(srgb.v))));
}

CMT_INTRINSIC f32x3 lab_to_lch(const f32x3& lab)
{
    return f32x3{ lab[0], kfr::sqrt(lab[1] * lab[1] + lab[2] * lab[2]), kfr::atan2deg(lab[1], lab[2]) };
}

CMT_INTRINSIC f32x3 lch_to_lab(const f32x3& lch)
{
    return f32x3{ lch[0], kfr::sindeg(lch[2]) * lch[1], kfr::cosdeg(lch[2]) * lch[1] };
}

CMT_INTRINSIC f32color adjust_lab(const f32color& srgb, float lum = 0.f, float chroma = 1.f)
{
    f32x3 lab = srgb_to_lab(srgb);
    lab[0]    = kfr::clamp(lab[0] + lum, 0.f, 100.f);
    lab[1]    = lab[1] * chroma;
    lab[2]    = lab[2] * chroma;
    return lab_to_srgb(lab, srgb.alpha);
}
CMT_INTRINSIC f32color adjust_lab_luminance(const f32color& srgb, float lum = 0.f)
{
    f32x3 lab = srgb_to_lab(srgb);
    lab[0]    = kfr::clamp(lab[0] + lum, 0.f, 100.f);
    return lab_to_srgb(lab, srgb.alpha);
}

// [0..360), [0..100], [0..100]
CMT_INTRINSIC f32color hsv_to_srgb(const f32x3& hsv, float alpha = 1.f)
{
    float v    = hsv[2];
    int hi     = int(hsv[0] / 60.f);
    int lo     = int(hsv[0]) % 60;
    float vmin = ((100.f - hsv[1]) * v) / 100.f;
    float a    = (v - vmin) * lo / 60.f;
    float vinc = vmin + a;
    float vdec = v - a;
    v /= 100.f;
    vmin /= 100.f;
    vinc /= 100.f;
    vdec /= 100.f;
    switch (hi % 6)
    {
    case 0:
        return { v, vinc, vmin, alpha };
    case 1:
        return { vdec, v, vmin, alpha };
    case 2:
        return { vmin, v, vinc, alpha };
    case 3:
        return { vmin, vdec, v, alpha };
    case 4:
        return { vinc, vmin, v, alpha };
    case 5:
        return { v, vmin, vdec, alpha };
    default:
        return { 0, 0, 0, 0 };
    }
}

} // namespace CMT_ARCH_NAME
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
