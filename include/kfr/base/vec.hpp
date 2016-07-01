/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
 */
#pragma once

#include "kfr.h"

#include "types.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wc++98-compat-local-type-template-args"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wpacked"

namespace kfr
{

template <typename T, size_t N>
struct vec;
template <typename T, size_t N>
struct mask;

using simdindex = int;

template <typename T, simdindex N>
using simd = T __attribute__((ext_vector_type(N)));

namespace internal
{
template <typename T>
struct is_vec_impl : std::false_type
{
};

template <typename T, size_t N>
struct is_vec_impl<vec<T, N>> : std::true_type
{
};

template <typename T, size_t N>
struct is_vec_impl<mask<T, N>> : std::true_type
{
};

template <typename T, bool A>
struct struct_with_alignment
{
    T value;
    KFR_INTRIN void operator=(T value) { this->value = value; }
};

template <typename T>
struct struct_with_alignment<T, false>
{
    T value;
    KFR_INTRIN void operator=(T value) { this->value = value; }
} __attribute__((__packed__, __may_alias__)); //
}

template <typename T>
using is_vec = internal::is_vec_impl<T>;

template <typename T, size_t N, bool A>
using vec_algn = internal::struct_with_alignment<simd<T, N>, A>;

template <typename T, size_t N, bool A>
struct vec_ptr
{
    constexpr KFR_INLINE vec_ptr(T* data) noexcept : data(data) {}
    constexpr KFR_INLINE vec_ptr(const T* data) noexcept : data(const_cast<T*>(data)) {}
    KFR_INLINE const vec_algn<T, N, A>& operator[](size_t i) const
    {
        return *static_cast<vec_algn<T, N, A>*>(data + i);
    }
    KFR_INLINE vec_algn<T, N, A>& operator[](size_t i) { return *static_cast<vec_algn<T, N, A>*>(data + i); }
    T* data;
};

template <typename To, typename From, size_t N,
          KFR_ENABLE_IF(std::is_same<subtype<From>, subtype<To>>::value),
          size_t Nout = N* compound_type_traits<From>::width / compound_type_traits<To>::width>
constexpr KFR_INLINE vec<To, Nout> subcast(vec<From, N> value) noexcept
{
    return *value;
}

namespace internal
{

template <typename Fn, size_t index>
constexpr enable_if<std::is_same<size_t, decltype(std::declval<Fn>().operator()(size_t()))>::value, size_t>
get_vec_index()
{
    constexpr Fn fn{};
    return fn(index);
}

template <typename Fn, size_t index>
constexpr enable_if<
    std::is_same<size_t, decltype(std::declval<Fn>().template operator() < index > ())>::value, size_t>
get_vec_index(int = 0)
{
    constexpr Fn fn{};
    return fn.template operator()<index>();
}

constexpr size_t index_undefined = static_cast<size_t>(-1);

template <typename T, size_t N, size_t... Indices, KFR_ENABLE_IF(!is_compound<T>::value)>
KFR_INLINE vec<T, sizeof...(Indices)> shufflevector(csizes_t<Indices...>, vec<T, N> x, vec<T, N> y)
{
    vec<T, sizeof...(Indices)> result = __builtin_shufflevector(
        *x, *y, static_cast<intptr_t>(Indices == index_undefined ? -1 : static_cast<intptr_t>(Indices))...);
    return result;
}

template <size_t... indices, size_t... counter, size_t groupsize = sizeof...(counter) / sizeof...(indices)>
constexpr auto inflate_impl(csizes_t<indices...> ind, csizes_t<counter...> cnt)
    -> csizes_t<(ind.get(csize<counter / groupsize>) == index_undefined
                     ? index_undefined
                     : (counter % groupsize + groupsize * ind.get(csize<counter / groupsize>)))...>
{
    return {};
}

template <size_t groupsize, size_t... indices>
constexpr auto inflate(csize_t<groupsize>, csizes_t<indices...>)
{
    return inflate_impl(csizes<indices...>, csizeseq<sizeof...(indices)*groupsize>);
}

template <typename T, size_t N, size_t... Indices, KFR_ENABLE_IF(is_compound<T>::value)>
KFR_INLINE vec<T, sizeof...(Indices)> shufflevector(csizes_t<Indices...> indices, vec<T, N> x, vec<T, N> y)
{
    return subcast<T>(
        shufflevector(inflate(csize<widthof<T>()>, indices), subcast<subtype<T>>(x), subcast<subtype<T>>(y)));
}

template <size_t... Indices, size_t Nout = sizeof...(Indices), typename T, size_t N>
KFR_INLINE vec<T, Nout> shufflevector(csizes_t<Indices...>, vec<T, N> x)
{
    return internal::shufflevector<T, N>(csizes<Indices...>, x, x);
}

template <typename Fn, size_t groupsize, typename T, size_t N, size_t... Indices,
          size_t Nout = sizeof...(Indices)>
KFR_INLINE vec<T, Nout> shufflevector(vec<T, N> x, vec<T, N> y, cvals_t<size_t, Indices...>)
{
    static_assert(N % groupsize == 0, "N % groupsize == 0");
    return internal::shufflevector<T, N>(
        csizes<(get_vec_index<Fn, Indices / groupsize>() * groupsize + Indices % groupsize)...>, x, y);
}
}

template <size_t Nout, typename Fn, size_t groupsize = 1, typename T, size_t N>
KFR_INLINE vec<T, Nout> shufflevector(vec<T, N> x, vec<T, N> y)
{
    return internal::shufflevector<Fn, groupsize>(x, y, csizeseq<Nout>);
}

template <size_t Nout, typename Fn, size_t groupsize = 1, typename T, size_t N>
KFR_INLINE vec<T, Nout> shufflevector(vec<T, N> x)
{
    return internal::shufflevector<Fn, groupsize>(x, x, csizeseq<Nout>);
}

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

template <typename To, typename From, KFR_ENABLE_IF(!is_compound<From>::value)>
constexpr KFR_INLINE To cast(From value) noexcept
{
    return static_cast<To>(value);
}
template <typename To, typename From, KFR_ENABLE_IF(!is_compound<From>::value)>
constexpr KFR_INLINE To bitcast(From value) noexcept
{
    union {
        From from;
        To to;
    } u{ value };
    return u.to;
}

template <typename From, typename To = utype<From>, KFR_ENABLE_IF(!is_compound<From>::value)>
constexpr KFR_INLINE To ubitcast(From value) noexcept
{
    return bitcast<To>(value);
}

template <typename From, typename To = itype<From>, KFR_ENABLE_IF(!is_compound<From>::value)>
constexpr KFR_INLINE To ibitcast(From value) noexcept
{
    return bitcast<To>(value);
}

template <typename From, typename To = ftype<From>, KFR_ENABLE_IF(!is_compound<From>::value)>
constexpr KFR_INLINE To fbitcast(From value) noexcept
{
    return bitcast<To>(value);
}

template <typename To, typename From, size_t N, KFR_ENABLE_IF(!is_compound<To>::value)>
constexpr KFR_INLINE vec<To, N> cast(vec<From, N> value) noexcept
{
    return __builtin_convertvector(*value, simd<To, N>);
}
template <typename To, typename From, simdindex N>
constexpr KFR_INLINE simd<To, N> cast(simd<From, N> value) noexcept
{
    return __builtin_convertvector(value, simd<To, N>);
}
template <typename To, typename From, size_t N, size_t Nout = sizeof(From) * N / sizeof(To)>
constexpr KFR_INLINE vec<To, Nout> bitcast(vec<From, N> value) noexcept
{
    return reinterpret_cast<simd<To, Nout>>(*value);
}
template <typename To, typename From, simdindex N, simdindex Nout = sizeof(From) * N / sizeof(To)>
constexpr KFR_INLINE simd<To, Nout> bitcast(simd<From, N> value) noexcept
{
    return reinterpret_cast<simd<To, Nout>>(value);
}

template <typename From, size_t N, typename To = utype<From>, size_t Nout = sizeof(From) * N / sizeof(To)>
constexpr KFR_INLINE vec<To, Nout> ubitcast(vec<From, N> value) noexcept
{
    return reinterpret_cast<simd<To, Nout>>(*value);
}

template <typename From, size_t N, typename To = itype<From>, size_t Nout = sizeof(From) * N / sizeof(To)>
constexpr KFR_INLINE vec<To, Nout> ibitcast(vec<From, N> value) noexcept
{
    return reinterpret_cast<simd<To, Nout>>(*value);
}

template <typename From, size_t N, typename To = ftype<From>, size_t Nout = sizeof(From) * N / sizeof(To)>
constexpr KFR_INLINE vec<To, Nout> fbitcast(vec<From, N> value) noexcept
{
    return reinterpret_cast<simd<To, Nout>>(*value);
}

template <typename From, simdindex N, typename To = utype<From>,
          simdindex Nout = sizeof(From) * N / sizeof(To)>
constexpr KFR_INLINE simd<To, Nout> ubitcast(simd<From, N> value) noexcept
{
    return reinterpret_cast<simd<To, Nout>>(value);
}

template <typename From, simdindex N, typename To = itype<From>,
          simdindex Nout = sizeof(From) * N / sizeof(To)>
constexpr KFR_INLINE simd<To, Nout> ibitcast(simd<From, N> value) noexcept
{
    return reinterpret_cast<simd<To, Nout>>(value);
}

template <typename From, simdindex N, typename To = ftype<From>,
          simdindex Nout = sizeof(From) * N / sizeof(To)>
constexpr KFR_INLINE simd<To, Nout> fbitcast(simd<From, N> value) noexcept
{
    return reinterpret_cast<simd<To, Nout>>(value);
}

constexpr KFR_INLINE size_t vector_alignment(size_t size) { return next_poweroftwo(size); }

template <typename T, size_t N, size_t... Sizes, size_t Nout = N + csum(csizes<Sizes...>)>
KFR_INLINE vec<T, Nout> concat(vec<T, N> x, vec<T, Sizes>... rest);

namespace internal
{
template <size_t start = 0, size_t stride = 1>
struct shuffle_index
{
    constexpr KFR_INLINE size_t operator()(size_t index) const { return start + index * stride; }
};

template <size_t count, size_t start = 0, size_t stride = 1>
struct shuffle_index_wrap
{
    constexpr inline size_t operator()(size_t index) const { return (start + index * stride) % count; }
};
}

template <size_t count, typename T, size_t N, size_t Nout = N* count>
KFR_INLINE vec<T, Nout> repeat(vec<T, N> x)
{
    return shufflevector<Nout, internal::shuffle_index_wrap<N, 0, 1>>(x);
}
KFR_FN(repeat)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"

template <size_t N, typename T>
constexpr KFR_INLINE vec<T, N> broadcast(T x)
{
    return (simd<T, N>)(x);
}

#pragma clang diagnostic pop

template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(Nout != N)>
KFR_INLINE vec<T, Nout> resize(vec<T, N> x)
{
    return shufflevector<Nout, internal::shuffle_index_wrap<N, 0, 1>>(x);
}
template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(Nout == N)>
constexpr KFR_INLINE vec<T, Nout> resize(vec<T, N> x)
{
    return x;
}
KFR_FN(resize)

namespace internal_read_write
{

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
KFR_INLINE vec<T, N> read(const T* src)
{
    return ptr_cast<vec_algn<subtype<T>, vec<T, N>::scalar_size(), A>>(src)->value;
}

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(!is_poweroftwo(N))>
KFR_INLINE vec<T, N> read(const T* src)
{
    constexpr size_t first = prev_poweroftwo(N);
    constexpr size_t rest  = N - first;
    return concat(internal_read_write::read<first, A>(src),
                  internal_read_write::read<rest, false>(src + first));
}

template <bool A = false, size_t N, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
KFR_INLINE void write(T* dest, vec<T, N> value)
{
    ptr_cast<vec_algn<subtype<T>, value.scalar_size(), A>>(dest)->value = *value;
}

template <bool A = false, size_t N, typename T, KFR_ENABLE_IF(!is_poweroftwo(N))>
KFR_INLINE void write(T* dest, vec<T, N> value)
{
    constexpr size_t first = prev_poweroftwo(N);
    constexpr size_t rest  = N - first;
    internal_read_write::write<A, first>(dest, shufflevector<first, internal::shuffle_index<0>>(value));
    internal_read_write::write<false, rest>(dest + first,
                                            shufflevector<rest, internal::shuffle_index<first>>(value));
}
}

template <typename T, size_t N>
struct pkd_vec
{
    constexpr pkd_vec() noexcept {}
    pkd_vec(const vec<T, N>& value) noexcept { internal_read_write::write(v, value); }
    template <typename... Ts>
    constexpr pkd_vec(Ts... init) noexcept : v{ static_cast<T>(init)... }
    {
        static_assert(N <= sizeof...(Ts), "Too few initializers for pkd_vec");
    }

private:
    T v[N];
    friend struct vec<T, N>;
} __attribute__((packed));

template <typename T>
struct vec_op
{
    using scalar_type = subtype<T>;

    template <simdindex N>
    constexpr static simd<scalar_type, N> add(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return x + y;
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> sub(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return x - y;
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> mul(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return x * y;
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> div(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return x / y;
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> rem(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return x % y;
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> shl(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return x << y;
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> shr(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return x >> y;
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> neg(simd<scalar_type, N> x) noexcept
    {
        return -x;
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> band(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return bitcast<scalar_type>(ubitcast(x) & ubitcast(y));
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> bor(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return bitcast<scalar_type>(ubitcast(x) | ubitcast(y));
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> bxor(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return bitcast<scalar_type>(ubitcast(x) ^ ubitcast(y));
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> bnot(simd<scalar_type, N> x) noexcept
    {
        return bitcast<scalar_type>(~ubitcast(x));
    }

    template <simdindex N>
    constexpr static simd<scalar_type, N> eq(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return bitcast<scalar_type>(x == y);
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> ne(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return bitcast<scalar_type>(x != y);
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> lt(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return bitcast<scalar_type>(x < y);
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> gt(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return bitcast<scalar_type>(x > y);
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> le(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return bitcast<scalar_type>(x <= y);
    }
    template <simdindex N>
    constexpr static simd<scalar_type, N> ge(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        return bitcast<scalar_type>(x >= y);
    }
};

namespace internal
{
template <typename T, typename... Args, size_t... indices, size_t N = 1 + sizeof...(Args)>
constexpr KFR_INLINE vec<T, N> make_vector_impl(csizes_t<indices...>, const T& x, const Args&... rest)
{
    constexpr size_t width = compound_type_traits<T>::width;
    const std::tuple<const T&, const Args&...> list(x, rest...);
    typename vec<T, N>::simd_t result{ compound_type_traits<T>::at(std::get<indices / width>(list),
                                                                   indices % width)... };
    return result;
}
}

/// Create vector from scalar values
/// @code
/// CHECK( make_vector( 1, 2, 3, 4 ) == i32x4{1, 2, 3, 4} );
/// @encode
template <typename Type = void, typename Arg, typename... Args, size_t N = (sizeof...(Args) + 1),
          typename SubType = conditional<is_void<Type>::value, common_type<Arg, Args...>, Type>>
constexpr KFR_INLINE vec<SubType, N> make_vector(const Arg& x, const Args&... rest)
{
    return internal::make_vector_impl<SubType>(csizeseq<N * widthof<SubType>()>, static_cast<SubType>(x),
                                               static_cast<SubType>(rest)...);
}
template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N> make_vector(vec<T, N> x)
{
    return x;
}
template <typename T, T... Values, size_t N = sizeof...(Values)>
constexpr KFR_INLINE vec<T, N> make_vector(cvals_t<T, Values...>)
{
    return make_vector<T>(Values...);
}
KFR_FN(make_vector)

template <typename T, size_t N>
struct vec : vec_t<T, N>
{
    static_assert(N > 0 && N <= 256, "Invalid vector size");

    using value_type  = T;
    using scalar_type = subtype<T>;
    constexpr static size_t scalar_size() noexcept { return N * compound_type_traits<T>::width; }
    using simd_t = simd<scalar_type, scalar_size()>;
    using ref    = vec&;
    using cref   = const vec&;

    constexpr static bool is_pod = true;

    constexpr KFR_INLINE vec() noexcept {}
    constexpr KFR_INLINE vec(simd_t value) noexcept : v(value) {}
    constexpr KFR_INLINE vec(const array_ref<T>& value) noexcept
        : v(*internal_read_write::read<N, false>(value.data()))
    {
    }
    template <typename U,
              KFR_ENABLE_IF(std::is_convertible<U, T>::value&& compound_type_traits<T>::width > 1)>
    constexpr KFR_INLINE vec(const U& value) noexcept
        : v(*resize<scalar_size()>(bitcast<scalar_type>(make_vector(static_cast<T>(value)))))
    {
    }
    template <typename U,
              KFR_ENABLE_IF(std::is_convertible<U, T>::value&& compound_type_traits<T>::width == 1)>
    constexpr KFR_INLINE vec(const U& value) noexcept : v(static_cast<T>(value))
    {
    }
    template <typename... Ts>
    constexpr KFR_INLINE vec(const T& x, const T& y, const Ts&... rest) noexcept
        : v(*make_vector<T>(x, y, rest...))
    {
        static_assert(N <= 2 + sizeof...(Ts), "Too few initializers for vec");
    }
    template <size_t N1, size_t N2, size_t... Ns>
    constexpr KFR_INLINE vec(const vec<T, N1>& v1, const vec<T, N2>& v2,
                             const vec<T, Ns>&... vectors) noexcept : v(*concat(v1, v2, vectors...))
    {
        static_assert(csum(csizes<N1, N2, Ns...>) == N, "Can't concat vectors: invalid csizes");
    }
    constexpr KFR_INLINE vec(const vec&) noexcept = default;
    constexpr KFR_INLINE vec(vec&&) noexcept      = default;
    constexpr KFR_INLINE vec& operator=(const vec&) noexcept = default;
    constexpr KFR_INLINE vec& operator=(vec&&) noexcept = default;

    friend constexpr KFR_INLINE vec operator+(vec x, vec y) { return vec_op<T>::add(x.v, y.v); }
    friend constexpr KFR_INLINE vec operator-(vec x, vec y) { return vec_op<T>::sub(x.v, y.v); }
    friend constexpr KFR_INLINE vec operator*(vec x, vec y) { return vec_op<T>::mul(x.v, y.v); }
    friend constexpr KFR_INLINE vec operator/(vec x, vec y) { return vec_op<T>::div(x.v, y.v); }
    friend constexpr KFR_INLINE vec operator%(vec x, vec y) { return vec_op<T>::rem(x.v, y.v); }
    friend constexpr KFR_INLINE vec operator-(vec x) { return vec_op<T>::neg(x.v); }

    friend constexpr KFR_INLINE vec operator&(vec x, vec y) { return vec_op<T>::band(x.v, y.v); }
    friend constexpr KFR_INLINE vec operator|(vec x, vec y) { return vec_op<T>::bor(x.v, y.v); }
    friend constexpr KFR_INLINE vec operator^(vec x, vec y) { return vec_op<T>::bxor(x.v, y.v); }
    friend constexpr KFR_INLINE vec operator~(vec x) { return vec_op<T>::bnot(x.v); }

    friend constexpr KFR_INLINE vec operator<<(vec x, vec y) { return vec_op<T>::shl(x.v, y.v); }
    friend constexpr KFR_INLINE vec operator>>(vec x, vec y) { return vec_op<T>::shr(x.v, y.v); }

    friend constexpr KFR_INLINE mask<T, N> operator==(vec x, vec y) { return vec_op<T>::eq(x.v, y.v); }
    friend constexpr KFR_INLINE mask<T, N> operator!=(vec x, vec y) { return vec_op<T>::ne(x.v, y.v); }
    friend constexpr KFR_INLINE mask<T, N> operator<(vec x, vec y) { return vec_op<T>::lt(x.v, y.v); }
    friend constexpr KFR_INLINE mask<T, N> operator>(vec x, vec y) { return vec_op<T>::gt(x.v, y.v); }
    friend constexpr KFR_INLINE mask<T, N> operator<=(vec x, vec y) { return vec_op<T>::le(x.v, y.v); }
    friend constexpr KFR_INLINE mask<T, N> operator>=(vec x, vec y) { return vec_op<T>::ge(x.v, y.v); }

#define KFR_ASGN_OP(aop, op)                                                                                 \
    friend KFR_INLINE vec& operator aop(vec& x, vec y)                                                       \
    {                                                                                                        \
        x = x op y;                                                                                          \
        return x;                                                                                            \
    }
    KFR_ASGN_OP(+=, +)
    KFR_ASGN_OP(-=, -)
    KFR_ASGN_OP(*=, *)
    KFR_ASGN_OP(/=, /)
    KFR_ASGN_OP(%=, %)
    KFR_ASGN_OP(&=, &)
    KFR_ASGN_OP(|=, |)
    KFR_ASGN_OP(^=, ^)
    KFR_ASGN_OP(<<=, <<)
    KFR_ASGN_OP(>>=, >>)

    constexpr KFR_INLINE simd_t operator*() const { return v; }
    constexpr KFR_INLINE simd_t& operator*() { return v; }
    KFR_INLINE mask<T, N>& asmask() { return ref_cast<mask<T, N>>(*this); }
    KFR_INLINE const mask<T, N>& asmask() const { return ref_cast<mask<T, N>>(*this); }
    KFR_INLINE value_type operator[](size_t index) const { return data()[index]; }

    KFR_INLINE value_type* data() { return ptr_cast<T>(&v); }
    KFR_INLINE const T* data() const { return ptr_cast<T>(&v); }
    using array_t = T (&)[N];
    KFR_INLINE array_t arr() { return ref_cast<array_t>(v); }

    template <typename U, KFR_ENABLE_IF(std::is_convertible<T, U>::value)>
    constexpr operator vec<U, N>() noexcept
    {
        return cast<U>(*this);
    }

private:
    struct getter_setter;

public:
    getter_setter operator()(size_t index) { return { v, index }; }
    scalar_type operator()(size_t index) const { return v[index]; }

protected:
    template <typename U, size_t M>
    friend struct vec;
    template <typename U, size_t M>
    friend struct mask;
    simd_t v;

private:
    struct getter_setter
    {
        constexpr getter_setter(simd_t& v, size_t index) noexcept : v(v), index(index) {}
        KFR_INLINE getter_setter& operator=(scalar_type value) noexcept
        {
            v[index] = value;
            return *this;
        }
        KFR_INLINE operator scalar_type() const { return v[index]; }
    private:
        friend struct vec;
        simd_t& v;
        const size_t index;
    };
};

template <typename T, size_t N>
struct mask : public vec<T, N>
{
    using type                    = T;
    constexpr static size_t width = N;

    using base = vec<T, N>;

    constexpr KFR_INLINE mask() noexcept : base() {}

    constexpr KFR_INLINE mask(simd<T, N> value) noexcept : base(value) {}
    template <size_t N1, size_t... Ns>
    constexpr KFR_INLINE mask(const mask<T, N1>& mask1, const mask<T, Ns>&... masks) noexcept
        : base(*concat(mask1, masks...))
    {
    }
    template <typename... Ts, typename = enable_if<sizeof...(Ts) + 2 == N>>
    constexpr KFR_INLINE mask(bool x, bool y, Ts... rest) noexcept
        : base{ internal::maskbits<T>(x), internal::maskbits<T>(y), internal::maskbits<T>(rest)... }
    {
    }
    constexpr KFR_INLINE mask(const mask&) noexcept = default;
    constexpr KFR_INLINE mask(mask&&) noexcept      = default;
    KFR_INLINE mask& operator=(const mask&) noexcept = default;
    KFR_INLINE mask& operator=(mask&&) noexcept = default;

    template <typename M, typename = u8[sizeof(T) == sizeof(M)]>
    constexpr KFR_INLINE mask(vec<M, N> value) : base(reinterpret_cast<const vec<T, N>&>(value))
    {
    }

    template <typename M, typename = u8[sizeof(T) == sizeof(M)]>
    constexpr KFR_INLINE mask(mask<M, N> value) : base(reinterpret_cast<const vec<T, N>&>(value))
    {
    }
    constexpr KFR_INLINE mask operator~() const { return bitcast<T>(~ubitcast(this->v)); }
    constexpr KFR_INLINE mask operator&(vec<T, N> x) const
    {
        return bitcast<T>(ubitcast(this->v) & ubitcast(x.v));
    }
    constexpr KFR_INLINE mask operator|(vec<T, N> x) const
    {
        return bitcast<T>(ubitcast(this->v) | ubitcast(x.v));
    }
    constexpr KFR_INLINE mask operator^(vec<T, N> x) const
    {
        return bitcast<T>(ubitcast(this->v) ^ ubitcast(x.v));
    }

    constexpr KFR_INLINE mask operator&&(mask x) const { return *this & x; }
    constexpr KFR_INLINE mask operator||(mask x) const { return *this | x; }
    constexpr KFR_INLINE mask operator!() const { return ~*this; }

    constexpr KFR_INLINE simd<T, N> operator*() const { return this->v; }

    KFR_INLINE vec<T, N>& asvec() { return ref_cast<mask>(*this); }
    KFR_INLINE const vec<T, N>& asvec() const { return ref_cast<mask>(*this); }

    KFR_INLINE bool operator[](size_t index) const { return ibitcast(this->v[index]) < 0; }
};

template <typename T, size_t N>
using cvec = vec<T, N * 2>;

namespace internal
{

template <size_t start, size_t count>
struct shuffle_index_extend
{
    constexpr KFR_INLINE size_t operator()(size_t index) const
    {
        return index >= start && index < start + count ? index - start : index_undefined;
    }
};

template <size_t start, size_t count, typename T, size_t N>
KFR_INLINE vec<T, count> concatexact(vec<T, N> x, vec<T, N> y)
{
    return kfr::shufflevector<count, internal::shuffle_index<start>>(x, y);
}

template <size_t start, size_t count, typename T, size_t N1, size_t N2>
KFR_INLINE enable_if<(N1 == N2), vec<T, count>> concattwo(vec<T, N1> x, vec<T, N2> y)
{
    return concatexact<start, count>(x, y);
}

template <size_t start, size_t count, typename T, size_t N1, size_t N2>
KFR_INLINE enable_if<(N1 > N2), vec<T, count>> concattwo(vec<T, N1> x, vec<T, N2> y)
{
    return concatexact<start, count>(x, shufflevector<N1, internal::shuffle_index_extend<0, N2>>(y));
}
template <size_t start, size_t count, typename T, size_t N1, size_t N2>
KFR_INLINE enable_if<(N1 < N2), vec<T, count>> concattwo(vec<T, N1> x, vec<T, N2> y)
{
    return concatexact<N2 - N1 + start, count>(
        shufflevector<N2, internal::shuffle_index_extend<N2 - N1, N1>>(x), y);
}

template <typename T, size_t Nout, size_t N1, size_t... indices>
constexpr mask<T, Nout> partial_mask_helper(csizes_t<indices...>)
{
    return make_vector(maskbits<T>(indices < N1)...);
}
template <typename T, size_t Nout, size_t N1>
constexpr mask<T, Nout> partial_mask()
{
    return internal::partial_mask_helper<T, Nout, N1>(csizeseq<Nout>);
}

template <typename T, size_t N>
KFR_INLINE vec<T, N> concat(vec<T, N> x)
{
    return x;
}

template <typename T, size_t N1, size_t N2>
KFR_INLINE vec<T, N1 + N2> concat(vec<T, N1> x, vec<T, N2> y)
{
    return concattwo<0, N1 + N2>(x, y);
}

template <typename T, size_t N1, size_t N2, size_t... Sizes>
KFR_INLINE auto concat(vec<T, N1> x, vec<T, N2> y, vec<T, Sizes>... args)
{
    return concat(x, concat(y, args...));
}
}

template <typename T, size_t N, size_t... Sizes, size_t Nout>
KFR_INLINE vec<T, Nout> concat(vec<T, N> x, vec<T, Sizes>... rest)
{
    return internal::concat(x, rest...);
}
KFR_FN(concat)

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

using mf32x1  = mask<f32, 1>;
using mf32x2  = mask<f32, 2>;
using mf32x3  = mask<f32, 3>;
using mf32x4  = mask<f32, 4>;
using mf32x8  = mask<f32, 8>;
using mf32x16 = mask<f32, 16>;
using mf32x32 = mask<f32, 32>;
using mf64x1  = mask<f64, 1>;
using mf64x2  = mask<f64, 2>;
using mf64x3  = mask<f64, 3>;
using mf64x4  = mask<f64, 4>;
using mf64x8  = mask<f64, 8>;
using mf64x16 = mask<f64, 16>;
using mf64x32 = mask<f64, 32>;
using mi8x1   = mask<i8, 1>;
using mi8x2   = mask<i8, 2>;
using mi8x3   = mask<i8, 3>;
using mi8x4   = mask<i8, 4>;
using mi8x8   = mask<i8, 8>;
using mi8x16  = mask<i8, 16>;
using mi8x32  = mask<i8, 32>;
using mi16x1  = mask<i16, 1>;
using mi16x2  = mask<i16, 2>;
using mi16x3  = mask<i16, 3>;
using mi16x4  = mask<i16, 4>;
using mi16x8  = mask<i16, 8>;
using mi16x16 = mask<i16, 16>;
using mi16x32 = mask<i16, 32>;
using mi32x1  = mask<i32, 1>;
using mi32x2  = mask<i32, 2>;
using mi32x4  = mask<i32, 3>;
using mi32x3  = mask<i32, 4>;
using mi32x8  = mask<i32, 8>;
using mi32x16 = mask<i32, 16>;
using mi32x32 = mask<i32, 32>;
using mi64x1  = mask<i64, 1>;
using mi64x2  = mask<i64, 2>;
using mi64x3  = mask<i64, 3>;
using mi64x4  = mask<i64, 4>;
using mi64x8  = mask<i64, 8>;
using mi64x16 = mask<i64, 16>;
using mi64x32 = mask<i64, 32>;
using mu8x1   = mask<u8, 1>;
using mu8x2   = mask<u8, 2>;
using mu8x3   = mask<u8, 3>;
using mu8x4   = mask<u8, 4>;
using mu8x8   = mask<u8, 8>;
using mu8x16  = mask<u8, 16>;
using mu8x32  = mask<u8, 32>;
using mu16x1  = mask<u16, 1>;
using mu16x2  = mask<u16, 2>;
using mu16x3  = mask<u16, 3>;
using mu16x4  = mask<u16, 4>;
using mu16x8  = mask<u16, 8>;
using mu16x16 = mask<u16, 16>;
using mu16x32 = mask<u16, 32>;
using mu32x1  = mask<u32, 1>;
using mu32x2  = mask<u32, 2>;
using mu32x3  = mask<u32, 3>;
using mu32x4  = mask<u32, 4>;
using mu32x8  = mask<u32, 8>;
using mu32x16 = mask<u32, 16>;
using mu32x32 = mask<u32, 32>;
using mu64x1  = mask<u64, 1>;
using mu64x2  = mask<u64, 2>;
using mu64x3  = mask<u64, 3>;
using mu64x4  = mask<u64, 4>;
using mu64x8  = mask<u64, 8>;
using mu64x16 = mask<u64, 16>;
using mu64x32 = mask<u64, 32>;

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
using f32sse = vec<f32, vector_width<f32, cpu_t::sse2>>;
using f64sse = vec<f64, vector_width<f64, cpu_t::sse2>>;
using i8sse  = vec<i8, vector_width<i8, cpu_t::sse2>>;
using i16sse = vec<i16, vector_width<i16, cpu_t::sse2>>;
using i32sse = vec<i32, vector_width<i32, cpu_t::sse2>>;
using i64sse = vec<i64, vector_width<i64, cpu_t::sse2>>;
using u8sse  = vec<u8, vector_width<u8, cpu_t::sse2>>;
using u16sse = vec<u16, vector_width<u16, cpu_t::sse2>>;
using u32sse = vec<u32, vector_width<u32, cpu_t::sse2>>;
using u64sse = vec<u64, vector_width<u64, cpu_t::sse2>>;

using mf32sse = mask<f32, vector_width<f32, cpu_t::sse2>>;
using mf64sse = mask<f64, vector_width<f64, cpu_t::sse2>>;
using mi8sse  = mask<i8, vector_width<i8, cpu_t::sse2>>;
using mi16sse = mask<i16, vector_width<i16, cpu_t::sse2>>;
using mi32sse = mask<i32, vector_width<i32, cpu_t::sse2>>;
using mi64sse = mask<i64, vector_width<i64, cpu_t::sse2>>;
using mu8sse  = mask<u8, vector_width<u8, cpu_t::sse2>>;
using mu16sse = mask<u16, vector_width<u16, cpu_t::sse2>>;
using mu32sse = mask<u32, vector_width<u32, cpu_t::sse2>>;
using mu64sse = mask<u64, vector_width<u64, cpu_t::sse2>>;

using f32avx = vec<f32, vector_width<f32, cpu_t::avx1>>;
using f64avx = vec<f64, vector_width<f64, cpu_t::avx1>>;
using i8avx  = vec<i8, vector_width<i8, cpu_t::avx2>>;
using i16avx = vec<i16, vector_width<i16, cpu_t::avx2>>;
using i32avx = vec<i32, vector_width<i32, cpu_t::avx2>>;
using i64avx = vec<i64, vector_width<i64, cpu_t::avx2>>;
using u8avx  = vec<u8, vector_width<u8, cpu_t::avx2>>;
using u16avx = vec<u16, vector_width<u16, cpu_t::avx2>>;
using u32avx = vec<u32, vector_width<u32, cpu_t::avx2>>;
using u64avx = vec<u64, vector_width<u64, cpu_t::avx2>>;

using mf32avx = mask<f32, vector_width<f32, cpu_t::avx1>>;
using mf64avx = mask<f64, vector_width<f64, cpu_t::avx1>>;
using mi8avx  = mask<i8, vector_width<i8, cpu_t::avx2>>;
using mi16avx = mask<i16, vector_width<i16, cpu_t::avx2>>;
using mi32avx = mask<i32, vector_width<i32, cpu_t::avx2>>;
using mi64avx = mask<i64, vector_width<i64, cpu_t::avx2>>;
using mu8avx  = mask<u8, vector_width<u8, cpu_t::avx2>>;
using mu16avx = mask<u16, vector_width<u16, cpu_t::avx2>>;
using mu32avx = mask<u32, vector_width<u32, cpu_t::avx2>>;
using mu64avx = mask<u64, vector_width<u64, cpu_t::avx2>>;

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
          typename Tout = result_of<Fn(subtype<remove_reference<Args>>...)>>
constexpr KFR_INLINE Tout applyfn_helper(Fn&& fn, Args&&... args)
{
    return fn(args[Index]...);
}

template <typename T, size_t N, typename Fn, typename... Args,
          typename Tout = result_of<Fn(subtype<remove_reference<Args>>...)>, size_t... Indices>
constexpr KFR_INLINE vec<Tout, N> apply_helper(Fn&& fn, csizes_t<Indices...>, Args&&... args)
{
    return make_vector(applyfn_helper<Indices, T, N>(std::forward<Fn>(fn), std::forward<Args>(args)...)...);
}
template <typename T, size_t N, typename Fn, size_t... Indices>
constexpr KFR_INLINE vec<T, N> apply0_helper(Fn&& fn, csizes_t<Indices...>)
{
    return make_vector(((void)Indices, void(), fn())...);
}
}

template <typename T, size_t N, typename Fn, typename... Args,
          typename Tout = result_of<Fn(T, subtype<remove_reference<Args>>...)>>
constexpr KFR_INLINE vec<Tout, N> apply(Fn&& fn, vec<T, N> arg, Args&&... args)
{
    return internal::apply_helper<T, N>(std::forward<Fn>(fn), csizeseq<N>, arg, std::forward<Args>(args)...);
}

template <size_t N, typename Fn, typename T = result_of<Fn()>>
constexpr KFR_INLINE vec<T, N> apply(Fn&& fn)
{
    return internal::apply0_helper<T, N>(std::forward<Fn>(fn), csizeseq<N>);
}

template <typename T, int N>
KFR_INLINE vec<T, N> tovec(simd<T, N> x)
{
    return x;
}
KFR_INLINE f32x4 tovec(__m128 x) { return f32x4(x); }
KFR_INLINE f64x2 tovec(__m128d x) { return f64x2(x); }
KFR_INLINE f32x8 tovec(__m256 x) { return f32x8(x); }
KFR_INLINE f64x4 tovec(__m256d x) { return f64x4(x); }

template <typename T, typename... Args, size_t Nout = (sizeof...(Args) + 1)>
constexpr KFR_INLINE mask<T, Nout> make_mask(bool arg, Args... args)
{
    simd<T, Nout> temp{ internal::maskbits<T>(arg), internal::maskbits<T>(static_cast<bool>(args))... };
    return temp;
}
KFR_FN(make_mask)

template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N> zerovector()
{
    constexpr size_t width = N * compound_type_traits<T>::width;
    return subcast<T>(vec<subtype<T>, width>(simd<subtype<T>, width>()));
}

template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N> zerovector(vec_t<T, N>)
{
    return zerovector<T, N>();
}
KFR_FN(zerovector)

template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N> allonesvector()
{
    return zerovector<T, N>() == zerovector<T, N>();
}
template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N> allonesvector(vec_t<T, N>)
{
    return allonesvector<T, N>();
}
KFR_FN(allonesvector)

template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N> undefinedvector()
{
    return vec<T, N>{};
}
template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N> undefinedvector(vec_t<T, N>)
{
    return undefinedvector<T, N>();
}
KFR_FN(undefinedvector)

template <typename T, size_t N, size_t Nout = prev_poweroftwo(N - 1)>
KFR_INLINE vec<T, Nout> low(vec<T, N> x)
{
    return shufflevector<Nout, internal::shuffle_index<>>(x);
}

template <typename T, size_t N, size_t Nout = prev_poweroftwo(N - 1)>
KFR_INLINE vec_t<T, Nout> low(vec_t<T, N>)
{
    return {};
}

template <typename T, size_t N, size_t Nout = N - prev_poweroftwo(N - 1)>
KFR_INLINE vec<T, Nout> high(vec<T, N> x)
{
    return shufflevector<Nout, internal::shuffle_index<prev_poweroftwo(N - 1)>>(x);
}

template <typename T, size_t N, size_t Nout = N - prev_poweroftwo(N - 1)>
KFR_INLINE vec_t<T, Nout> high(vec_t<T, N>)
{
    return {};
}
KFR_FN(low)
KFR_FN(high)

namespace internal
{

template <typename Fn>
struct expression_lambda : input_expression
{
    KFR_INLINE expression_lambda(Fn&& fn) : fn(std::move(fn)) {}

    template <typename T, size_t N, KFR_ENABLE_IF(N&& is_callable<Fn, cinput_t, size_t, vec_t<T, N>>::value)>
    KFR_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N> y) const
    {
        return fn(cinput, index, y);
    }

    template <typename T, size_t N, KFR_ENABLE_IF(N&& is_callable<Fn, size_t>::value)>
    KFR_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        vec<T, N> result;
        for (size_t i = 0; i < N; i++)
        {
            result(i) = fn(index + i);
        }
        return result;
    }
    template <typename T, size_t N, KFR_ENABLE_IF(N&& is_callable<Fn>::value)>
    KFR_INLINE vec<T, N> operator()(cinput_t, size_t, vec_t<T, N>) const
    {
        vec<T, N> result;
        for (size_t i = 0; i < N; i++)
        {
            result(i) = fn();
        }
        return result;
    }

    Fn fn;
};
}

template <typename Fn>
internal::expression_lambda<decay<Fn>> lambda(Fn&& fn)
{
    return internal::expression_lambda<Fn>(std::move(fn));
}
}

#pragma clang diagnostic pop

namespace cometa
{

template <typename T, size_t N>
struct compound_type_traits<kfr::simd<T, N>>
{
    using subtype                   = T;
    using deep_subtype              = cometa::deep_subtype<T>;
    constexpr static size_t width   = N;
    constexpr static bool is_scalar = false;
    template <typename U>
    using rebind = kfr::simd<U, N>;
    template <typename U>
    using deep_rebind = kfr::simd<cometa::deep_rebind<subtype, U>, N>;

    static constexpr const subtype& at(const kfr::simd<T, N>& value, size_t index) { return value[index]; }
};

template <typename T, size_t N>
struct compound_type_traits<kfr::vec<T, N>>
{
    using subtype                   = T;
    using deep_subtype              = cometa::deep_subtype<T>;
    constexpr static size_t width   = N;
    constexpr static bool is_scalar = false;
    template <typename U>
    using rebind = kfr::vec<U, N>;
    template <typename U>
    using deep_rebind = kfr::vec<cometa::deep_rebind<subtype, U>, N>;

    static constexpr subtype at(const kfr::vec<T, N>& value, size_t index) { return value[index]; }
};

template <typename T, size_t N>
struct compound_type_traits<kfr::mask<T, N>>
{
    using subtype                   = T;
    using deep_subtype              = cometa::deep_subtype<T>;
    constexpr static size_t width   = N;
    constexpr static bool is_scalar = false;
    template <typename U>
    using rebind = kfr::mask<U, N>;
    template <typename U>
    using deep_rebind = kfr::mask<cometa::deep_rebind<subtype, U>, N>;

    static constexpr subtype at(const kfr::mask<T, N>& value, size_t index) { return value[index]; }
};
}
