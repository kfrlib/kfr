/** @addtogroup types
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

#include "kfr.h"

#include "constants.hpp"
#include "types.hpp"

namespace kfr
{

template <typename T, size_t N, size_t Nout = prev_poweroftwo(N - 1)>
CMT_INLINE vec<T, Nout> low(const vec<T, N>& x);
template <typename T, size_t N, size_t Nout = N - prev_poweroftwo(N - 1)>
CMT_INLINE vec<T, Nout> high(const vec<T, N>& x);
}

#ifdef CMT_COMPILER_CLANG
#include "simd_clang.hpp"
#else
#include "simd_intrin.hpp"
#ifdef CMT_ARCH_X86
#include "simd_x86.hpp"
#endif
#endif

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpragmas")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wfloat-equal")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wc++98-compat-local-type-template-args")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpacked")

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4814))

namespace kfr
{

template <typename T>
using maskfor = typename T::mask_t;

template <typename T, size_t N>
struct mask : vec<T, N>
{
    using base                          = vec<T, N>;
    KFR_I_CE mask() noexcept            = default;
    KFR_I_CE mask(const mask&) noexcept = default;
    KFR_I_CE mask& operator=(const mask&) noexcept = default;
    using simd_type                                = typename base::simd_type;

    KFR_I_CE mask(const base& v) noexcept : base(v) {}

    KFR_I_CE mask(const simd_type& simd) : base(simd) {}
    template <typename U, KFR_ENABLE_IF(sizeof(T) == sizeof(U))>
    KFR_I_CE mask(const mask<U, N>& m) : base(base::frombits(m))
    {
    }
    template <typename U, KFR_ENABLE_IF(sizeof(T) == sizeof(U))>
    KFR_I_CE mask(const vec<U, N>& m) : base(base::frombits(m))
    {
    }
    KFR_I_CE mask operator&&(const mask& y) const noexcept
    {
        return static_cast<const base&>(*this) & static_cast<const base&>(y);
    }
    KFR_I_CE mask operator||(const mask& y) const noexcept
    {
        return static_cast<const base&>(*this) | static_cast<const base&>(y);
    }
    KFR_I_CE mask operator^(const mask& y) const noexcept
    {
        return static_cast<const base&>(*this) ^ static_cast<const base&>(y);
    }
    KFR_I_CE mask operator^(const base& y) const noexcept { return static_cast<const base&>(*this) ^ y; }

    bool operator[](size_t index) const noexcept;

    constexpr base asvec() const noexcept { return static_cast<const base&>(*this); }
};

namespace internal
{

constexpr inline size_t scale_get_index(size_t counter, size_t groupsize, size_t index)
{
    return index == index_undefined ? index_undefined : (counter % groupsize + groupsize * index);
}

template <size_t counter, size_t groupsize, size_t... indices>
constexpr inline size_t scale_get_index()
{
    return scale_get_index(counter, groupsize, csizes_t<indices...>().get(csize_t<counter / groupsize>()));
}

template <size_t... indices, size_t... counter, size_t groupsize = sizeof...(counter) / sizeof...(indices)>
constexpr inline auto scale_impl(csizes_t<indices...> ind, csizes_t<counter...> cnt) noexcept
    -> csizes_t<scale_get_index<counter, groupsize, indices...>()...>
{
    return {};
}
}

template <size_t groupsize, size_t... indices>
constexpr inline auto scale() noexcept
{
    return internal::scale_impl(csizes_t<indices...>(), csizeseq_t<sizeof...(indices) * groupsize>());
}

template <typename T, size_t Nin, size_t N>
struct vec<vec<T, Nin>, N> : private vec<T, Nin * N>
{
    using base = vec<T, Nin * N>;

    using value_type = vec<T, Nin>;
    constexpr static size_t size() noexcept { return N; }

    using scalar_type = T;
    constexpr static size_t scalar_size() noexcept { return Nin * N; }

    using simd_type = typename base::simd_type;

    constexpr vec() noexcept           = default;
    constexpr vec(const vec&) noexcept = default;
    CMT_GNU_CONSTEXPR vec& operator=(const vec&) CMT_GNU_NOEXCEPT = default;
    constexpr vec(const simd_type& simd) noexcept : base(simd) {}
    constexpr vec(czeros_t) noexcept : base(czeros) {}
    constexpr vec(cones_t) noexcept : base(cones) {}

    constexpr vec(const value_type& v) noexcept : base(v.shuffle(csizeseq_t<Nin * N>() % csize_t<Nin>())) {}

    template <int = 0>
    explicit constexpr vec(const vec<T, Nin * N>& v) noexcept : base(v)
    {
    }

    // from list of vectors
    template <typename... Us>
    constexpr vec(const value_type& s0, const value_type& s1, const Us&... rest) noexcept
        : base(s0, s1, rest...)
    {
    }

    template <typename U>
    constexpr vec(const vec<vec<U, Nin>, N>& v) noexcept : base(static_cast<vec<T, Nin * N>>(v.flatten()))
    {
    }

    template <typename U, size_t M, KFR_ENABLE_IF(sizeof(U) * M == sizeof(T) * N)>
    constexpr static vec frombits(const vec<U, M>& v) noexcept
    {
        return vec(base::frombits(v.flatten()));
    }

    // math / bitwise / comparison operators
    constexpr friend vec operator+(const vec& x) noexcept { return x; }
    constexpr friend vec operator-(const vec& x) noexcept { return base::operator-(x); }
    constexpr friend vec operator~(const vec& x) noexcept { return base::operator~(x); }

#define KFR_B(x) static_cast<const base&>(x)

    constexpr friend vec operator+(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) + KFR_B(y)); }
    constexpr friend vec operator-(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) - KFR_B(y)); }
    constexpr friend vec operator*(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) * KFR_B(y)); }
    constexpr friend vec operator/(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) / KFR_B(y)); }

    constexpr friend vec operator<<(const vec& x, int shift) noexcept { return vec(KFR_B(x) << shift); }
    constexpr friend vec operator>>(const vec& x, int shift) noexcept { return vec(KFR_B(x) >> shift); }
    constexpr friend vec operator&(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) & KFR_B(y)); }
    constexpr friend vec operator|(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) | KFR_B(y)); }
    constexpr friend vec operator^(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) ^ KFR_B(y)); }

#undef KFR_B

    constexpr friend vec& operator+=(vec& x, const vec& y) noexcept { return x = x + y; }
    constexpr friend vec& operator-=(vec& x, const vec& y) noexcept { return x = x - y; }
    constexpr friend vec& operator*=(vec& x, const vec& y) noexcept { return x = x * y; }
    constexpr friend vec& operator/=(vec& x, const vec& y) noexcept { return x = x / y; }

    constexpr friend vec& operator<<=(vec& x, int shift) noexcept { return x = x << shift; }
    constexpr friend vec& operator>>=(vec& x, int shift) noexcept { return x = x >> shift; }
    constexpr friend vec& operator&=(vec& x, const vec& y) noexcept { return x = x & y; }
    constexpr friend vec& operator|=(vec& x, const vec& y) noexcept { return x = x | y; }
    constexpr friend vec& operator^=(vec& x, const vec& y) noexcept { return x = x ^ y; }

    constexpr friend vec& operator++(vec& x) noexcept { return x = x + vec(1); }
    constexpr friend vec& operator--(vec& x) noexcept { return x = x - vec(1); }
    constexpr friend vec operator++(vec& x, int)noexcept
    {
        const vec z = x;
        ++x;
        return z;
    }
    constexpr friend vec operator--(vec& x, int)noexcept
    {
        const vec z = x;
        --x;
        return z;
    }

    // shuffle
    template <size_t... indices>
    constexpr vec<value_type, sizeof...(indices)> shuffle(csizes_t<indices...>) const noexcept
    {
        return *base::shuffle(scale<Nin, indices...>());
    }
    template <size_t... indices>
    constexpr vec<value_type, sizeof...(indices)> shuffle(const vec& y, csizes_t<indices...>) const noexcept
    {
        return *base::shuffle(y, scale<Nin, indices...>());
    }

    // element access
    struct element;
    CMT_GNU_CONSTEXPR value_type operator[](size_t index) const noexcept { return get(index); }
    CMT_GNU_CONSTEXPR element operator[](size_t index) noexcept { return { *this, index }; }

    CMT_GNU_CONSTEXPR value_type get(size_t index) const noexcept
    {
        return reinterpret_cast<const value_type(&)[N]>(*this)[index];
    }
    CMT_GNU_CONSTEXPR void set(size_t index, const value_type& s) noexcept
    {
        reinterpret_cast<value_type(&)[N]>(*this)[index] = s;
    }
    template <size_t index>
    CMT_GNU_CONSTEXPR value_type get(csize_t<index>) const noexcept
    {
        return static_cast<const base&>(*this).shuffle(csizeseq_t<Nin, index * Nin>());
    }
    template <size_t index>
    CMT_GNU_CONSTEXPR void set(csize_t<index>, const value_type& s) noexcept
    {
        *this = vec(static_cast<const base&>(*this))
                    .shuffle(s, csizeseq_t<N>() +
                                    (csizeseq_t<N>() >= csize_t<index * Nin>() &&
                                     csizeseq_t<N>() < csize_t<(index + 1) * Nin>()) *
                                        N);
    }
    struct element
    {
        constexpr operator value_type() const noexcept { return v.get(index); }
        element& operator=(const value_type& s) noexcept
        {
            v.set(index, s);
            return *this;
        }
        vec& v;
        size_t index;
    };

    template <bool aligned = false>
    explicit constexpr vec(const value_type* src, cbool_t<aligned> = cbool_t<aligned>()) noexcept
        : base(ptr_cast<T>(src), cbool_t<aligned>())
    {
    }
    template <bool aligned = false>
    const vec& write(value_type* dest, cbool_t<aligned> = cbool_t<aligned>()) const noexcept
    {
        base::write(ptr_cast<T>(dest), cbool_t<aligned>());
        return *this;
    }

    const base& flatten() const noexcept { return *this; }
    simd_type operator*() const noexcept { return base::operator*(); }
    simd_type& operator*() noexcept { return base::operator*(); }
};

namespace internal
{

template <typename T>
constexpr inline T maskbits(bool value)
{
    return value ? constants<T>::allones() : T();
}

template <typename T, size_t N>
struct flt_type_impl<vec<T, N>>
{
    using type = vec<typename flt_type_impl<T>::type, N>;
};

template <typename T>
struct is_vec_impl : std::false_type
{
};

template <typename T, size_t N>
struct is_vec_impl<vec<T, N>> : std::true_type
{
};
}

template <typename T>
using is_vec = internal::is_vec_impl<T>;

template <typename To, typename From, size_t N,
          KFR_ENABLE_IF(std::is_same<subtype<From>, subtype<To>>::value),
          size_t Nout = N* compound_type_traits<From>::width / compound_type_traits<To>::width>
constexpr CMT_INLINE vec<To, Nout> compcast(const vec<From, N>& value) noexcept
{
    return vec<To, Nout>(value.flatten());
}

#ifdef KFR_ENABLE_SWIZZLE
namespace swizzle
{
template <size_t>
struct swiz
{
    constexpr swiz() {}
};

constexpr swiz<0> x{};
constexpr swiz<1> y{};
constexpr swiz<2> z{};
constexpr swiz<3> w{};
constexpr swiz<0> r{};
constexpr swiz<1> g{};
constexpr swiz<2> b{};
constexpr swiz<3> a{};
constexpr swiz<0> s{};
constexpr swiz<1> t{};
constexpr swiz<2> p{};
constexpr swiz<3> q{};

constexpr swiz<0> s0{};
constexpr swiz<1> s1{};
constexpr swiz<2> s2{};
constexpr swiz<3> s3{};
constexpr swiz<4> s4{};
constexpr swiz<5> s5{};
constexpr swiz<6> s6{};
constexpr swiz<7> s7{};
constexpr swiz<8> s8{};
constexpr swiz<9> s9{};
constexpr swiz<10> s10{};
constexpr swiz<11> s11{};
constexpr swiz<12> s12{};
constexpr swiz<13> s13{};
constexpr swiz<14> s14{};
constexpr swiz<15> s15{};
}
#endif

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wold-style-cast")

template <size_t N, typename T>
constexpr CMT_INLINE vec<T, N> broadcast(T x)
{
    return x;
}

CMT_PRAGMA_GNU(GCC diagnostic pop)

namespace internal
{

template <typename To, typename From, size_t N, typename Tsub = deep_subtype<To>,
          size_t Nout = N* compound_type_traits<To>::deep_width>
constexpr CMT_INLINE vec<To, N> builtin_convertvector(const vec<From, N>& value) noexcept
{
    return vec<To, N>(value);
}

// scalar to scalar
template <typename To, typename From>
struct conversion
{
    static_assert(std::is_convertible<From, To>::value, "");
    static To cast(const From& value) { return value; }
};

// vector to vector
template <typename To, typename From, size_t N>
struct conversion<vec<To, N>, vec<From, N>>
{
    static_assert(!is_compound<To>::value, "");
    static_assert(!is_compound<From>::value, "");
    static vec<To, N> cast(const vec<From, N>& value) { return builtin_convertvector<To>(value); }
};

// vector<vector> to vector<vector>
template <typename To, typename From, size_t N1, size_t N2>
struct conversion<vec<vec<To, N1>, N2>, vec<vec<From, N1>, N2>>
{
    static_assert(!is_compound<To>::value, "");
    static_assert(!is_compound<From>::value, "");
    static vec<vec<To, N1>, N2> cast(const vec<vec<From, N1>, N2>& value)
    {
        return builtin_convertvector<vec<To, N1>>(value);
    }
};

// scalar to vector
template <typename To, typename From, size_t N>
struct conversion<vec<To, N>, From>
{
    static_assert(std::is_convertible<From, To>::value, "");
    static vec<To, N> cast(const From& value) { return broadcast<N>(static_cast<To>(value)); }
};
}

template <typename T>
constexpr size_t size_of() noexcept
{
    return sizeof(deep_subtype<T>) * compound_type_traits<T>::deep_width;
}

template <typename From, size_t N, typename Tsub = deep_subtype<From>,
          size_t Nout = N* size_of<From>() / size_of<Tsub>()>
constexpr CMT_INLINE vec<Tsub, Nout> flatten(const vec<From, N>& x) noexcept
{
    return x.flatten();
}

template <typename To, typename From,
          typename Tout = typename compound_type_traits<From>::template deep_rebind<To>>
constexpr CMT_INLINE Tout cast(const From& value) noexcept
{
    return static_cast<Tout>(value);
}

template <typename To, typename From>
CMT_GNU_CONSTEXPR CMT_INLINE To bitcast(const From& value) noexcept
{
    static_assert(sizeof(From) == sizeof(To), "bitcast: Incompatible types");
    union {
        From from;
        To to;
    } u{ value };
    return u.to;
}

template <typename To, typename From, size_t N, size_t Nout = N* size_of<From>() / size_of<To>()>
CMT_GNU_CONSTEXPR CMT_INLINE vec<To, Nout> bitcast(const vec<From, N>& value) noexcept
{
    return vec<To, Nout>::frombits(value);
}

template <typename From, typename To = utype<From>, KFR_ENABLE_IF(!is_compound<From>::value)>
constexpr CMT_INLINE To ubitcast(const From& value) noexcept
{
    return bitcast<To>(value);
}

template <typename From, typename To = itype<From>, KFR_ENABLE_IF(!is_compound<From>::value)>
constexpr CMT_INLINE To ibitcast(const From& value) noexcept
{
    return bitcast<To>(value);
}

template <typename From, typename To = ftype<From>, KFR_ENABLE_IF(!is_compound<From>::value)>
constexpr CMT_INLINE To fbitcast(const From& value) noexcept
{
    return bitcast<To>(value);
}

template <typename From, size_t N, typename To = utype<From>,
          size_t Nout = size_of<From>() * N / size_of<To>()>
constexpr CMT_INLINE vec<To, Nout> ubitcast(const vec<From, N>& value) noexcept
{
    return vec<To, Nout>::frombits(value);
}

template <typename From, size_t N, typename To = itype<From>,
          size_t Nout = size_of<From>() * N / size_of<To>()>
constexpr CMT_INLINE vec<To, Nout> ibitcast(const vec<From, N>& value) noexcept
{
    return vec<To, Nout>::frombits(value);
}

template <typename From, size_t N, typename To = ftype<From>,
          size_t Nout = size_of<From>() * N / size_of<To>()>
constexpr CMT_INLINE vec<To, Nout> fbitcast(const vec<From, N>& value) noexcept
{
    return vec<To, Nout>::frombits(value);
}

template <typename T, size_t N>
inline bool mask<T, N>::operator[](size_t index) const noexcept
{
    return ibitcast(base::operator[](index)) < 0;
}

constexpr CMT_INLINE size_t vector_alignment(size_t size) { return next_poweroftwo(size); }

namespace internal
{
template <size_t start = 0, size_t stride = 1>
struct shuffle_index
{
    constexpr CMT_INLINE size_t operator()(size_t index) const { return start + index * stride; }
};

template <size_t count, size_t start = 0, size_t stride = 1>
struct shuffle_index_wrap
{
    constexpr inline size_t operator()(size_t index) const { return (start + index * stride) % count; }
};
}

template <size_t count, typename T, size_t N, size_t Nout = N* count>
CMT_INLINE vec<T, Nout> repeat(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<Nout>() % csize_t<N>());
}
KFR_FN(repeat)

template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(Nout != N)>
CMT_INLINE vec<T, Nout> resize(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<Nout>() % csize_t<N>());
}
template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(Nout == N)>
constexpr CMT_INLINE vec<T, Nout> resize(const vec<T, N>& x)
{
    return x;
}
KFR_FN(resize)

template <typename T, size_t N>
struct pkd_vec
{
    constexpr pkd_vec() noexcept {}
    pkd_vec(const vec<T, N>& value) noexcept { value.write(v); }
    template <typename... Ts>
    constexpr pkd_vec(Ts... init) noexcept : v{ static_cast<T>(init)... }
    {
        static_assert(N <= sizeof...(Ts), "Too few initializers for pkd_vec");
    }

private:
    T v[N];
    friend struct vec<T, N>;
}
#ifdef CMT_GNU_ATTRIBUTES
__attribute__((packed))
#endif
;

namespace internal
{

template <size_t, typename T>
constexpr CMT_INLINE T make_vector_get_n()
{
    return T();
}
template <size_t index, typename T, typename... Args>
constexpr CMT_INLINE T make_vector_get_n(const T& arg, const Args&... args)
{
    return index == 0 ? arg : make_vector_get_n<index - 1, T>(args...);
}

template <typename T, typename... Args, size_t... indices, size_t N = sizeof...(Args)>
CMT_GNU_CONSTEXPR CMT_INLINE vec<T, N> make_vector_impl(csizes_t<indices...>, const Args&... args)
{
    const T list[] = { static_cast<T>(args)... };
    return vec<T, N>(list[indices]...);
}
}

/// Create vector from scalar values
/// @code
/// CHECK( make_vector( 1, 2, 3, 4 ) == i32x4{1, 2, 3, 4} );
/// @encode
template <typename Type = void, typename Arg, typename... Args, size_t N = (sizeof...(Args) + 1),
          typename SubType = conditional<is_void<Type>::value, common_type<Arg, Args...>, Type>>
constexpr CMT_INLINE vec<SubType, N> make_vector(const Arg& x, const Args&... rest)
{
    return internal::make_vector_impl<SubType>(cvalseq_t<size_t, N>(), static_cast<SubType>(x),
                                               static_cast<SubType>(rest)...);
}
template <typename T, size_t N>
constexpr CMT_INLINE vec<T, N> make_vector(const vec<T, N>& x)
{
    return x;
}
template <typename T, T... Values, size_t N = sizeof...(Values)>
constexpr CMT_INLINE vec<T, N> make_vector(cvals_t<T, Values...>)
{
    return make_vector<T>(Values...);
}
KFR_FN(make_vector)

template <typename Type = void, typename Arg, typename... Args, size_t N = (sizeof...(Args) + 1),
          typename SubType = conditional<is_void<Type>::value, common_type<Arg, Args...>, Type>,
          KFR_ENABLE_IF(is_number<subtype<SubType>>::value)>
constexpr CMT_INLINE vec<SubType, N> pack(const Arg& x, const Args&... rest)
{
    return internal::make_vector_impl<SubType>(csizeseq_t<N * widthof<SubType>()>(), static_cast<SubType>(x),
                                               static_cast<SubType>(rest)...);
}
KFR_FN(pack)

namespace operators
{
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator+(const vec<T1, N>& x, const T2& y)
{
    return static_cast<vec<C, N>>(x) + static_cast<vec<C, N>>(y);
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator-(const vec<T1, N>& x, const T2& y)
{
    return static_cast<vec<C, N>>(x) - static_cast<vec<C, N>>(y);
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator*(const vec<T1, N>& x, const T2& y)
{
    return static_cast<vec<C, N>>(x) * static_cast<vec<C, N>>(y);
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator/(const vec<T1, N>& x, const T2& y)
{
    return static_cast<vec<C, N>>(x) / static_cast<vec<C, N>>(y);
}

template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator+(const T1& x, const vec<T2, N>& y)
{
    return static_cast<vec<C, N>>(x) + static_cast<vec<C, N>>(y);
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator-(const T1& x, const vec<T2, N>& y)
{
    return static_cast<vec<C, N>>(x) - static_cast<vec<C, N>>(y);
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator*(const T1& x, const vec<T2, N>& y)
{
    return static_cast<vec<C, N>>(x) * static_cast<vec<C, N>>(y);
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator/(const T1& x, const vec<T2, N>& y)
{
    return static_cast<vec<C, N>>(x) / static_cast<vec<C, N>>(y);
}

template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator+(const vec<T1, N>& x, const vec<T2, N>& y)
{
    return static_cast<vec<C, N>>(x) + static_cast<vec<C, N>>(y);
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator-(const vec<T1, N>& x, const vec<T2, N>& y)
{
    return static_cast<vec<C, N>>(x) - static_cast<vec<C, N>>(y);
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator*(const vec<T1, N>& x, const vec<T2, N>& y)
{
    return static_cast<vec<C, N>>(x) * static_cast<vec<C, N>>(y);
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator/(const vec<T1, N>& x, const vec<T2, N>& y)
{
    return static_cast<vec<C, N>>(x) / static_cast<vec<C, N>>(y);
}

template <typename T1, size_t N>
constexpr CMT_INLINE vec<T1, N> operator&&(const T1& x, const vec<T1, N>& y)
{
    return static_cast<vec<T1, N>>(x) && y;
}
template <typename T1, size_t N>
constexpr CMT_INLINE vec<T1, N> operator||(const T1& x, const vec<T1, N>& y)
{
    return static_cast<vec<T1, N>>(x) || y;
}
template <typename T1, size_t N>
constexpr CMT_INLINE vec<T1, N> operator&(const T1& x, const vec<T1, N>& y)
{
    return static_cast<vec<T1, N>>(x) & y;
}
template <typename T1, size_t N>
constexpr CMT_INLINE vec<T1, N> operator|(const T1& x, const vec<T1, N>& y)
{
    return static_cast<vec<T1, N>>(x) | y;
}
template <typename T1, size_t N>
constexpr CMT_INLINE vec<T1, N> operator^(const T1& x, const vec<T1, N>& y)
{
    return static_cast<vec<T1, N>>(x) ^ y;
}
}

using namespace operators;

template <typename T, size_t N1, size_t N2 = N1>
using mat = vec<vec<T, N1>, N2>;

namespace internal
{

template <size_t start, size_t count>
struct shuffle_index_extend
{
    constexpr CMT_INLINE size_t operator()(size_t index) const
    {
        return index >= start && index < start + count ? index - start : index_undefined;
    }
};

template <typename T, size_t Nout, size_t N1, size_t... indices>
constexpr vec<T, Nout> partial_mask_helper(csizes_t<indices...>)
{
    return make_vector(maskbits<T>(indices < N1)...);
}
template <typename T, size_t Nout, size_t N1>
constexpr vec<T, Nout> partial_mask()
{
    return internal::partial_mask_helper<T, Nout, N1>(csizeseq_t<Nout>());
}
}

template <typename T>
using optvec = vec<T, platform<T>::vector_capacity / 4>;

using f32x1  = vec<f32, 1>;
using f32x2  = vec<f32, 2>;
using f32x3  = vec<f32, 3>;
using f32x4  = vec<f32, 4>;
using f32x8  = vec<f32, 8>;
using f32x16 = vec<f32, 16>;
using f32x32 = vec<f32, 32>;
using f64x1  = vec<f64, 1>;
using f64x2  = vec<f64, 2>;
using f64x3  = vec<f64, 3>;
using f64x4  = vec<f64, 4>;
using f64x8  = vec<f64, 8>;
using f64x16 = vec<f64, 16>;
using f64x32 = vec<f64, 32>;
using i8x1   = vec<i8, 1>;
using i8x2   = vec<i8, 2>;
using i8x3   = vec<i8, 3>;
using i8x4   = vec<i8, 4>;
using i8x8   = vec<i8, 8>;
using i8x16  = vec<i8, 16>;
using i8x32  = vec<i8, 32>;
using i16x1  = vec<i16, 1>;
using i16x2  = vec<i16, 2>;
using i16x3  = vec<i16, 3>;
using i16x4  = vec<i16, 4>;
using i16x8  = vec<i16, 8>;
using i16x16 = vec<i16, 16>;
using i16x32 = vec<i16, 32>;
using i32x1  = vec<i32, 1>;
using i32x2  = vec<i32, 2>;
using i32x3  = vec<i32, 3>;
using i32x4  = vec<i32, 4>;
using i32x8  = vec<i32, 8>;
using i32x16 = vec<i32, 16>;
using i32x32 = vec<i32, 32>;
using i64x1  = vec<i64, 1>;
using i64x2  = vec<i64, 2>;
using i64x3  = vec<i64, 3>;
using i64x4  = vec<i64, 4>;
using i64x8  = vec<i64, 8>;
using i64x16 = vec<i64, 16>;
using i64x32 = vec<i64, 32>;
using u8x1   = vec<u8, 1>;
using u8x2   = vec<u8, 2>;
using u8x3   = vec<u8, 3>;
using u8x4   = vec<u8, 4>;
using u8x8   = vec<u8, 8>;
using u8x16  = vec<u8, 16>;
using u8x32  = vec<u8, 32>;
using u16x1  = vec<u16, 1>;
using u16x2  = vec<u16, 2>;
using u16x3  = vec<u16, 3>;
using u16x4  = vec<u16, 4>;
using u16x8  = vec<u16, 8>;
using u16x16 = vec<u16, 16>;
using u16x32 = vec<u16, 32>;
using u32x1  = vec<u32, 1>;
using u32x2  = vec<u32, 2>;
using u32x3  = vec<u32, 3>;
using u32x4  = vec<u32, 4>;
using u32x8  = vec<u32, 8>;
using u32x16 = vec<u32, 16>;
using u32x32 = vec<u32, 32>;
using u64x1  = vec<u64, 1>;
using u64x2  = vec<u64, 2>;
using u64x3  = vec<u64, 3>;
using u64x4  = vec<u64, 4>;
using u64x8  = vec<u64, 8>;
using u64x16 = vec<u64, 16>;
using u64x32 = vec<u64, 32>;

using u8x2x2  = vec<vec<u8, 2>, 2>;
using i8x2x2  = vec<vec<i8, 2>, 2>;
using u16x2x2 = vec<vec<u16, 2>, 2>;
using i16x2x2 = vec<vec<i16, 2>, 2>;
using u32x2x2 = vec<vec<u32, 2>, 2>;
using i32x2x2 = vec<vec<i32, 2>, 2>;
using u64x2x2 = vec<vec<u64, 2>, 2>;
using i64x2x2 = vec<vec<i64, 2>, 2>;
using f32x2x2 = vec<vec<f32, 2>, 2>;
using f64x2x2 = vec<vec<f64, 2>, 2>;

using u8x4x4  = vec<vec<u8, 4>, 4>;
using i8x4x4  = vec<vec<i8, 4>, 4>;
using u16x4x4 = vec<vec<u16, 4>, 4>;
using i16x4x4 = vec<vec<i16, 4>, 4>;
using u32x4x4 = vec<vec<u32, 4>, 4>;
using i32x4x4 = vec<vec<i32, 4>, 4>;
using u64x4x4 = vec<vec<u64, 4>, 4>;
using i64x4x4 = vec<vec<i64, 4>, 4>;
using f32x4x4 = vec<vec<f32, 4>, 4>;
using f64x4x4 = vec<vec<f64, 4>, 4>;

namespace glsl_names
{
using vec2  = f32x2;
using vec3  = f32x3;
using vec4  = f32x4;
using dvec2 = f64x2;
using dvec3 = f64x3;
using dvec4 = f64x4;
using ivec2 = i32x2;
using ivec3 = i32x3;
using ivec4 = i32x4;
using uvec2 = u32x2;
using uvec3 = u32x3;
using uvec4 = u32x4;
}
namespace opencl_names
{
using char2   = i8x2;
using char3   = i8x3;
using char4   = i8x4;
using char8   = i8x8;
using char16  = i8x16;
using uchar2  = u8x2;
using uchar3  = u8x3;
using uchar4  = u8x4;
using uchar8  = u8x8;
using uchar16 = u8x16;

using short2   = i16x2;
using short3   = i16x3;
using short4   = i16x4;
using short8   = i16x8;
using short16  = i16x16;
using ushort2  = u16x2;
using ushort3  = u16x3;
using ushort4  = u16x4;
using ushort8  = u16x8;
using ushort16 = u16x16;

using int2   = i32x2;
using int3   = i32x3;
using int4   = i32x4;
using int8   = i32x8;
using int16  = i32x16;
using uint2  = u32x2;
using uint3  = u32x3;
using uint4  = u32x4;
using uint8  = u32x8;
using uint16 = u32x16;

using long2   = i64x2;
using long3   = i64x3;
using long4   = i64x4;
using long8   = i64x8;
using long16  = i64x16;
using ulong2  = u64x2;
using ulong3  = u64x3;
using ulong4  = u64x4;
using ulong8  = u64x8;
using ulong16 = u64x16;

using float2  = f32x2;
using float3  = f32x3;
using float4  = f32x4;
using float8  = f32x8;
using float16 = f32x16;

using double2  = f64x2;
using double3  = f64x3;
using double4  = f64x4;
using double8  = f64x8;
using double16 = f64x16;
}

namespace internal
{

template <typename T, size_t N>
struct vec_type
{
    using type = vec<T, N>;
};

template <typename T, size_t Nmax>
struct maxvec
{
    constexpr static size_t size = Nmax;
    vec<T, size> vmax;
    maxvec(T initial) : vmax(initial) {}
    template <int N>
    vec<T, N>& v()
    {
        static_assert(N <= size, "N <= size");
        return reinterpret_cast<vec<T, N>&>(*this);
    }
    template <int N>
    const vec<T, N>& v() const
    {
        static_assert(N <= size, "N <= size");
        return reinterpret_cast<const vec<T, N>&>(*this);
    }
};

template <size_t Index, typename T, size_t N, typename Fn, typename... Args,
          typename Tout = result_of<Fn(subtype<decay<Args>>...)>>
constexpr CMT_INLINE Tout applyfn_helper(Fn&& fn, Args&&... args)
{
    return fn(args[Index]...);
}

template <typename T, size_t N, typename Fn, typename... Args,
          typename Tout = result_of<Fn(subtype<decay<Args>>...)>, size_t... Indices>
constexpr CMT_INLINE vec<Tout, N> apply_helper(Fn&& fn, csizes_t<Indices...>, Args&&... args)
{
    return make_vector(applyfn_helper<Indices, T, N>(std::forward<Fn>(fn), std::forward<Args>(args)...)...);
}
template <typename T, size_t N, typename Fn, size_t... Indices>
constexpr CMT_INLINE vec<T, N> apply0_helper(Fn&& fn, csizes_t<Indices...>)
{
    return make_vector(((void)Indices, void(), fn())...);
}
}

template <typename T, size_t N, typename Fn, typename... Args,
          typename Tout = result_of<Fn(T, subtype<decay<Args>>...)>>
constexpr CMT_INLINE vec<Tout, N> apply(Fn&& fn, const vec<T, N>& arg, Args&&... args)
{
    return internal::apply_helper<T, N>(std::forward<Fn>(fn), csizeseq_t<N>(), arg,
                                        std::forward<Args>(args)...);
}

template <size_t N, typename Fn, typename T = result_of<Fn()>>
constexpr CMT_INLINE vec<T, N> apply(Fn&& fn)
{
    return internal::apply0_helper<T, N>(std::forward<Fn>(fn), csizeseq_t<N>());
}

#if defined CMT_ARCH_SSE2 && defined KFR_NATIVE_SIMD
CMT_INLINE f32x4 tovec(__m128 x) { return f32x4(x); }
CMT_INLINE f64x2 tovec(__m128d x) { return f64x2(x); }
#endif

template <typename T, typename... Args, size_t Nout = (sizeof...(Args) + 1)>
constexpr CMT_INLINE mask<T, Nout> make_mask(bool arg, Args... args)
{
    return vec<T, Nout>(internal::maskbits<T>(arg), internal::maskbits<T>(static_cast<bool>(args))...);
}
KFR_FN(make_mask)

template <typename T, size_t N>
CMT_GNU_CONSTEXPR CMT_INLINE vec<T, N> zerovector()
{
    return vec<T, N>(czeros);
}

template <typename T, size_t N>
CMT_GNU_CONSTEXPR CMT_INLINE vec<T, N> zerovector(vec_t<T, N>)
{
    return vec<T, N>(czeros);
}
KFR_FN(zerovector)

template <typename T, size_t N>
CMT_GNU_CONSTEXPR CMT_INLINE vec<T, N> allonesvector()
{
    return vec<T, N>(cones);
}
template <typename T, size_t N>
CMT_GNU_CONSTEXPR CMT_INLINE vec<T, N> allonesvector(vec_t<T, N>)
{
    return vec<T, N>(cones);
}
KFR_FN(allonesvector)

template <typename T, size_t N>
constexpr CMT_INLINE vec<T, N> undefinedvector()
{
    return vec<T, N>{};
}
template <typename T, size_t N>
constexpr CMT_INLINE vec<T, N> undefinedvector(vec_t<T, N>)
{
    return undefinedvector<T, N>();
}
KFR_FN(undefinedvector)

template <typename T, size_t N, size_t Nout /*= prev_poweroftwo(N - 1)*/>
CMT_INLINE vec<T, Nout> low(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<Nout>());
}

template <typename T, size_t N, size_t Nout = prev_poweroftwo(N - 1)>
CMT_INLINE vec_t<T, Nout> low(vec_t<T, N>)
{
    return {};
}

template <typename T, size_t N, size_t Nout /*= N - prev_poweroftwo(N - 1)*/>
CMT_INLINE vec<T, Nout> high(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<Nout, prev_poweroftwo(N - 1)>());
}

template <typename T, size_t N, size_t Nout = N - prev_poweroftwo(N - 1)>
CMT_INLINE vec_t<T, Nout> high(vec_t<T, N>)
{
    return {};
}
KFR_FN(low)
KFR_FN(high)
}

namespace cometa
{

template <typename T, size_t N>
struct compound_type_traits<kfr::vec_t<T, N>>
{
    constexpr static size_t width      = N;
    constexpr static size_t deep_width = width * compound_type_traits<T>::width;
    using subtype                      = T;
    using deep_subtype                 = cometa::deep_subtype<T>;
    constexpr static bool is_scalar    = false;
    constexpr static size_t depth      = cometa::compound_type_traits<T>::depth + 1;

    template <typename U>
    using rebind = kfr::vec_t<U, N>;
    template <typename U>
    using deep_rebind = kfr::vec_t<typename compound_type_traits<subtype>::template deep_rebind<U>, N>;
};

template <typename T, size_t N>
struct compound_type_traits<kfr::vec<T, N>>
{
    using subtype                      = T;
    using deep_subtype                 = cometa::deep_subtype<T>;
    constexpr static size_t width      = N;
    constexpr static size_t deep_width = width * compound_type_traits<T>::width;
    constexpr static bool is_scalar    = false;
    constexpr static size_t depth      = cometa::compound_type_traits<T>::depth + 1;
    template <typename U>
    using rebind = kfr::vec<U, N>;
    template <typename U>
    using deep_rebind = kfr::vec<typename compound_type_traits<subtype>::template deep_rebind<U>, N>;

    CMT_INLINE static constexpr subtype at(const kfr::vec<T, N>& value, size_t index) { return value[index]; }
};

template <typename T, size_t N>
struct compound_type_traits<kfr::mask<T, N>>
{
    using subtype                      = T;
    using deep_subtype                 = cometa::deep_subtype<T>;
    constexpr static size_t width      = N;
    constexpr static size_t deep_width = width * compound_type_traits<T>::width;
    constexpr static bool is_scalar    = false;
    constexpr static size_t depth      = cometa::compound_type_traits<T>::depth + 1;
    template <typename U>
    using rebind = kfr::mask<U, N>;
    template <typename U>
    using deep_rebind = kfr::mask<typename compound_type_traits<subtype>::template deep_rebind<U>, N>;

    CMT_INLINE static constexpr subtype at(const kfr::mask<T, N>& value, size_t index)
    {
        return value[index];
    }
};
}

namespace std
{
template <typename T1, typename T2, size_t N>
struct common_type<kfr::vec<T1, N>, kfr::vec<T2, N>>
{
    using type = kfr::vec<typename common_type<T1, T2>::type, N>;
};
template <typename T1, typename T2, size_t N>
struct common_type<kfr::vec<T1, N>, T2>
{
    using type = kfr::vec<typename common_type<T1, T2>::type, N>;
};
template <typename T1, typename T2, size_t N>
struct common_type<T1, kfr::vec<T2, N>>
{
    using type = kfr::vec<typename common_type<T1, T2>::type, N>;
};
template <typename T1, typename T2, size_t N1, size_t N2>
struct common_type<kfr::vec<T1, N1>, kfr::vec<kfr::vec<T2, N1>, N2>>
{
    using type = kfr::vec<kfr::vec<typename common_type<T1, T2>::type, N1>, N2>;
};
template <typename T1, typename T2, size_t N1, size_t N2>
struct common_type<kfr::vec<kfr::vec<T1, N1>, N2>, kfr::vec<T2, N1>>
{
    using type = kfr::vec<kfr::vec<typename common_type<T1, T2>::type, N1>, N2>;
};

template <typename T1, typename T2, size_t N>
struct common_type<kfr::mask<T1, N>, kfr::mask<T2, N>>
{
    using type = kfr::mask<typename common_type<T1, T2>::type, N>;
};
}

CMT_PRAGMA_GNU(GCC diagnostic pop)
CMT_PRAGMA_MSVC(warning(pop))
