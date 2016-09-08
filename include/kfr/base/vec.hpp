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

#include "types.hpp"

#include "simd.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wc++98-compat-local-type-template-args"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wpacked"

namespace kfr
{

/// @brief Base class for all vector classes
template <typename T, size_t N>
struct vec_t
{
    using value_type = T;
    constexpr static size_t size() noexcept { return N; }
    constexpr vec_t() noexcept = default;

    using scalar_type = subtype<T>;
    constexpr static size_t scalar_size() noexcept { return N * compound_type_traits<T>::width; }
};

template <typename T, size_t N>
struct vec;
template <typename T, size_t N>
struct mask;

namespace internal
{

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

template <typename T, size_t N>
struct is_vec_impl<mask<T, N>> : std::true_type
{
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

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

#pragma GCC diagnostic pop

template <typename T, size_t N, bool A>
struct vec_ptr
{
    constexpr CMT_INLINE vec_ptr(T* data) noexcept : data(data) {}
    constexpr CMT_INLINE vec_ptr(const T* data) noexcept : data(const_cast<T*>(data)) {}
    CMT_INLINE const vec_algn<T, N, A>& operator[](size_t i) const
    {
        return *static_cast<vec_algn<T, N, A>*>(data + i);
    }
    CMT_INLINE vec_algn<T, N, A>& operator[](size_t i) { return *static_cast<vec_algn<T, N, A>*>(data + i); }
    T* data;
};

template <typename To, typename From, size_t N,
          KFR_ENABLE_IF(std::is_same<subtype<From>, subtype<To>>::value),
          size_t Nout = N* compound_type_traits<From>::width / compound_type_traits<To>::width>
constexpr CMT_INLINE vec<To, Nout> compcast(const vec<From, N>& value) noexcept
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
CMT_INLINE vec<T, sizeof...(Indices)> shufflevector(csizes_t<Indices...>, const vec<T, N>& x,
                                                    const vec<T, N>& y)
{
    vec<T, sizeof...(Indices)> result = KFR_BUILTIN_SHUFFLEVECTOR(
        T, N, *x, *y,
        static_cast<intptr_t>(Indices == index_undefined ? -1 : static_cast<intptr_t>(Indices))...);
    return result;
}

template <size_t counter, size_t groupsize, size_t... indices>
constexpr size_t inflate_get_index()
{
    constexpr csizes_t<indices...> ind{};
    return (ind.get(csize<counter / groupsize>) == index_undefined
                ? index_undefined
                : (counter % groupsize + groupsize * ind.get(csize<counter / groupsize>)));
}

template <size_t... indices, size_t... counter, size_t groupsize = sizeof...(counter) / sizeof...(indices)>
constexpr auto inflate_impl(csizes_t<indices...> ind, csizes_t<counter...> cnt)
    -> csizes_t<inflate_get_index<counter, groupsize, indices...>()...>
{
    return {};
}
}

namespace internal
{

template <size_t groupsize, size_t... indices>
constexpr auto inflate(csize_t<groupsize>, csizes_t<indices...>)
{
    return inflate_impl(csizes<indices...>, csizeseq<sizeof...(indices)*groupsize>);
}

template <typename T, size_t N, size_t... Indices, KFR_ENABLE_IF(is_compound<T>::value)>
CMT_INLINE vec<T, sizeof...(Indices)> shufflevector(csizes_t<Indices...> indices, const vec<T, N>& x,
                                                    const vec<T, N>& y)
{
    return compcast<T>(shufflevector(inflate(csize<widthof<T>()>, indices), compcast<subtype<T>>(x),
                                     compcast<subtype<T>>(y)));
}

template <size_t... Indices, size_t Nout = sizeof...(Indices), typename T, size_t N>
CMT_INLINE vec<T, Nout> shufflevector(csizes_t<Indices...>, const vec<T, N>& x)
{
    return internal::shufflevector<T, N>(csizes<Indices...>, x, x);
}

template <typename Fn, size_t groupsize, typename T, size_t N, size_t... Indices,
          size_t Nout = sizeof...(Indices)>
CMT_INLINE vec<T, Nout> shufflevector(const vec<T, N>& x, const vec<T, N>& y, cvals_t<size_t, Indices...>)
{
    static_assert(N % groupsize == 0, "N % groupsize == 0");
    return internal::shufflevector<T, N>(
        csizes<(get_vec_index<Fn, Indices / groupsize>() * groupsize + Indices % groupsize)...>, x, y);
}
}

template <size_t Nout, typename Fn, size_t groupsize = 1, typename T, size_t N>
CMT_INLINE vec<T, Nout> shufflevector(const vec<T, N>& x, const vec<T, N>& y)
{
    return internal::shufflevector<Fn, groupsize>(x, y, csizeseq<Nout>);
}

template <size_t Nout, typename Fn, size_t groupsize = 1, typename T, size_t N>
CMT_INLINE vec<T, Nout> shufflevector(const vec<T, N>& x)
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"

template <size_t N, typename T>
constexpr CMT_INLINE vec<T, N> broadcast(T x)
{
    return x;
}

#pragma clang diagnostic pop

namespace internal
{

template <typename To, typename From, size_t N, typename Tsub = deep_subtype<To>,
          size_t Nout = N* compound_type_traits<To>::deep_width>
constexpr CMT_INLINE vec<To, N> builtin_convertvector(const vec<From, N>& value) noexcept
{
    return KFT_CONVERT_VECTOR(*value, Tsub, Nout);
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

// mask to mask
template <typename To, typename From, size_t N>
struct conversion<mask<To, N>, mask<From, N>>
{
    static_assert(sizeof(To) == sizeof(From), "");
    static mask<To, N> cast(const mask<From, N>& value) { return reinterpret_cast<simd<To, N>>(*value); }
};
}

template <typename T>
constexpr size_t size_of() noexcept
{
    return sizeof(deep_subtype<T>) * compound_type_traits<T>::deep_width;
}

template <typename From, size_t N, typename Tsub = deep_subtype<From>,
          size_t Nout = N* size_of<From>() / size_of<Tsub>()>
constexpr CMT_INLINE vec<Tsub, Nout> flatten(const vec<From, N>& value) noexcept
{
    return *value;
}

template <typename To, typename From, typename Tout = deep_rebind<From, To>>
constexpr CMT_INLINE Tout cast(const From& value) noexcept
{
    return static_cast<Tout>(value);
}

template <typename To, typename From>
constexpr CMT_INLINE To bitcast(const From& value) noexcept
{
    static_assert(sizeof(From) == sizeof(To), "bitcast: Incompatible types");
    union {
        From from;
        To to;
    } u{ value };
    return u.to;
}

template <typename To, typename From, size_t N, size_t Nout = N* size_of<From>() / size_of<To>()>
constexpr CMT_INLINE vec<To, Nout> bitcast(const vec<From, N>& value) noexcept
{
    return reinterpret_cast<typename vec<To, Nout>::simd_t>(*value);
}

template <typename To, typename From, size_t N, size_t Nout = N* size_of<From>() / size_of<To>()>
constexpr CMT_INLINE mask<To, Nout> bitcast(const mask<From, N>& value) noexcept
{
    return reinterpret_cast<typename mask<To, Nout>::simd_t>(*value);
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
    return reinterpret_cast<simd<To, Nout>>(*value);
}

template <typename From, size_t N, typename To = itype<From>,
          size_t Nout = size_of<From>() * N / size_of<To>()>
constexpr CMT_INLINE vec<To, Nout> ibitcast(const vec<From, N>& value) noexcept
{
    return reinterpret_cast<simd<To, Nout>>(*value);
}

template <typename From, size_t N, typename To = ftype<From>,
          size_t Nout = size_of<From>() * N / size_of<To>()>
constexpr CMT_INLINE vec<To, Nout> fbitcast(const vec<From, N>& value) noexcept
{
    return reinterpret_cast<simd<To, Nout>>(*value);
}

constexpr CMT_INLINE size_t vector_alignment(size_t size) { return next_poweroftwo(size); }

template <typename T, size_t N, size_t... Sizes>
CMT_INLINE vec<T, N + csum(csizes<Sizes...>)> concat(const vec<T, N>& x, const vec<T, Sizes>&... rest);

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
    return shufflevector<Nout, internal::shuffle_index_wrap<N, 0, 1>>(x);
}
KFR_FN(repeat)

template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(Nout != N)>
CMT_INLINE vec<T, Nout> resize(const vec<T, N>& x)
{
    return shufflevector<Nout, internal::shuffle_index_wrap<N, 0, 1>>(x);
}
template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(Nout == N)>
constexpr CMT_INLINE vec<T, Nout> resize(const vec<T, N>& x)
{
    return x;
}
KFR_FN(resize)

namespace internal_read_write
{

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
CMT_INLINE vec<T, N> read(const T* src)
{
    return ptr_cast<vec_algn<subtype<T>, vec<T, N>::scalar_size(), A>>(src)->value;
}

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(!is_poweroftwo(N))>
CMT_INLINE vec<T, N> read(const T* src)
{
    constexpr size_t first = prev_poweroftwo(N);
    constexpr size_t rest  = N - first;
    return concat(internal_read_write::read<first, A>(src),
                  internal_read_write::read<rest, false>(src + first));
}

template <bool A = false, size_t N, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
CMT_INLINE void write(T* dest, const vec<T, N>& value)
{
    ptr_cast<vec_algn<subtype<T>, vec<T, N>::scalar_size(), A>>(dest)->value = *value;
}

template <bool A = false, size_t N, typename T, KFR_ENABLE_IF(!is_poweroftwo(N))>
CMT_INLINE void write(T* dest, const vec<T, N>& value)
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
constexpr CMT_INLINE vec<T, N> make_vector_impl(csizes_t<indices...>, const Args&... args)
{
    constexpr size_t width = compound_type_traits<T>::width;
    const T list[]         = { args... };
    using simd_t           = typename vec<T, N>::simd_t;
    return simd_t{ compound_type_traits<T>::at(list[indices / width], indices % width)... };
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
    return internal::make_vector_impl<SubType>(csizeseq<N * widthof<SubType>()>, static_cast<SubType>(x),
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
    return internal::make_vector_impl<SubType>(csizeseq<N * widthof<SubType>()>, static_cast<SubType>(x),
                                               static_cast<SubType>(rest)...);
}
KFR_FN(pack)

namespace operators
{
struct empty
{
};
}

template <typename T, size_t N>
struct vec : vec_t<T, N>, operators::empty
{
    static_assert(N > 0 && N <= 256, "Invalid vector size");

    static_assert(!is_vec<T>::value || is_poweroftwo(size_of<T>()),
                  "Inner vector size must be a power of two");

    using UT          = utype<T>;
    using value_type  = T;
    using scalar_type = subtype<T>;
    constexpr static size_t scalar_size() noexcept { return N * compound_type_traits<T>::width; }
    using simd_t = simd<scalar_type, scalar_size()>;
    using ref    = vec&;
    using cref   = const vec&;

    constexpr static bool is_pod = true;

    constexpr CMT_INLINE vec() noexcept {}
    constexpr CMT_INLINE vec(simd_t value) noexcept : v(value) {}
    template <typename U,
              KFR_ENABLE_IF(std::is_convertible<U, T>::value&& compound_type_traits<T>::width > 1)>
    constexpr CMT_INLINE vec(const U& value) noexcept
        : v(*resize<scalar_size()>(bitcast<scalar_type>(make_vector(static_cast<T>(value)))))
    {
    }
    template <typename U,
              KFR_ENABLE_IF(std::is_convertible<U, T>::value&& compound_type_traits<T>::width == 1)>
    constexpr CMT_INLINE vec(const U& value) noexcept : v(KFR_SIMD_FROM_SCALAR(static_cast<T>(value), T, N))
    {
    }
    template <typename... Ts>
    constexpr CMT_INLINE vec(const T& x, const T& y, const Ts&... rest) noexcept
        : v(*make_vector<T>(x, y, rest...))
    {
        static_assert(N <= 2 + sizeof...(Ts), "Too few initializers for vec");
    }
    template <size_t N1, size_t N2, size_t... Ns>
    constexpr CMT_INLINE vec(const vec<T, N1>& v1, const vec<T, N2>& v2,
                             const vec<T, Ns>&... vectors) noexcept : v(*concat(v1, v2, vectors...))
    {
        static_assert(csum(csizes<N1, N2, Ns...>) == N, "Can't concat vectors: invalid csizes");
    }
    constexpr CMT_INLINE vec(const vec&) noexcept = default;
    constexpr CMT_INLINE vec(vec&&) noexcept      = default;
    constexpr CMT_INLINE vec& operator=(const vec&) noexcept = default;
    constexpr CMT_INLINE vec& operator=(vec&&) noexcept = default;

    friend constexpr CMT_INLINE vec operator+(const vec& x, const vec& y)
    {
        return vec_op<T, N>::add(x.v, y.v);
    }
    friend constexpr CMT_INLINE vec operator-(const vec& x, const vec& y)
    {
        return vec_op<T, N>::sub(x.v, y.v);
    }
    friend constexpr CMT_INLINE vec operator*(const vec& x, const vec& y)
    {
        return vec_op<T, N>::mul(x.v, y.v);
    }
    friend constexpr CMT_INLINE vec operator/(const vec& x, const vec& y)
    {
        return vec_op<T, N>::div(x.v, y.v);
    }
    friend constexpr CMT_INLINE vec operator%(const vec& x, const vec& y)
    {
        return vec_op<T, N>::rem(x.v, y.v);
    }
    friend constexpr CMT_INLINE vec operator-(const vec& x) { return vec_op<T, N>::neg(x.v); }

    friend constexpr CMT_INLINE vec operator&(const vec& x, const vec& y)
    {
        return vec_op<T, N>::band(x.v, y.v);
    }
    friend constexpr CMT_INLINE vec operator|(const vec& x, const vec& y)
    {
        return vec_op<T, N>::bor(x.v, y.v);
    }
    friend constexpr CMT_INLINE vec operator^(const vec& x, const vec& y)
    {
        return vec_op<T, N>::bxor(x.v, y.v);
    }
    friend constexpr CMT_INLINE vec operator~(const vec& x) { return vec_op<T, N>::bnot(x.v); }

    friend constexpr CMT_INLINE vec operator<<(const vec& x, const vec& y)
    {
        return vec_op<T, N>::shl(x.v, y.v);
    }
    friend constexpr CMT_INLINE vec operator>>(const vec& x, const vec& y)
    {
        return vec_op<T, N>::shr(x.v, y.v);
    }

    friend constexpr CMT_INLINE mask<T, N> operator==(const vec& x, const vec& y)
    {
        return vec_op<T, N>::eq(x.v, y.v);
    }
    friend constexpr CMT_INLINE mask<T, N> operator!=(const vec& x, const vec& y)
    {
        return vec_op<T, N>::ne(x.v, y.v);
    }
    friend constexpr CMT_INLINE mask<T, N> operator<(const vec& x, const vec& y)
    {
        return vec_op<T, N>::lt(x.v, y.v);
    }
    friend constexpr CMT_INLINE mask<T, N> operator>(const vec& x, const vec& y)
    {
        return vec_op<T, N>::gt(x.v, y.v);
    }
    friend constexpr CMT_INLINE mask<T, N> operator<=(const vec& x, const vec& y)
    {
        return vec_op<T, N>::le(x.v, y.v);
    }
    friend constexpr CMT_INLINE mask<T, N> operator>=(const vec& x, const vec& y)
    {
        return vec_op<T, N>::ge(x.v, y.v);
    }

#define KFR_ASGN_OP(aop, op)                                                                                 \
    friend CMT_INLINE vec& operator aop(vec& x, const vec& y)                                                \
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
#undef KFR_ASGN_OP

    constexpr CMT_INLINE const simd_t& operator*() const { return v; }
    constexpr CMT_INLINE simd_t& operator*() { return v; }
    CMT_INLINE mask<T, N>& asmask() { return ref_cast<mask<T, N>>(*this); }
    CMT_INLINE const mask<T, N>& asmask() const { return ref_cast<mask<T, N>>(*this); }
    CMT_INLINE value_type operator[](size_t index) const { return data()[index]; }

    CMT_INLINE value_type* data() { return ptr_cast<T>(&v); }
    CMT_INLINE const T* data() const { return ptr_cast<T>(&v); }
    using array_t = T (&)[N];
    CMT_INLINE array_t arr() { return ref_cast<array_t>(v); }

    template <typename U, KFR_ENABLE_IF(std::is_convertible<T, U>::value && !std::is_same<U, vec>::value)>
    CMT_INLINE constexpr operator vec<U, N>() const noexcept
    {
        return internal::conversion<vec<U, N>, vec<T, N>>::cast(*this);
    }

private:
    struct getter_setter;

public:
    CMT_INLINE getter_setter operator()(size_t index) { return { v, index }; }
    CMT_INLINE scalar_type operator()(size_t index) const { return v[index]; }

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
        CMT_INLINE getter_setter& operator=(scalar_type value) noexcept
        {
            v[index] = value;
            return *this;
        }
        CMT_INLINE operator scalar_type() const { return v[index]; }
    private:
        friend struct vec;
        simd_t& v;
        const size_t index;
    };
} __attribute__((aligned(next_poweroftwo(sizeof(T) * N))));

namespace operators
{
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator+(const vec<T1, N>& x, const T2& y)
{
    return vec_op<C, N>::add(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator-(const vec<T1, N>& x, const T2& y)
{
    return vec_op<C, N>::sub(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator*(const vec<T1, N>& x, const T2& y)
{
    return vec_op<C, N>::mul(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator/(const vec<T1, N>& x, const T2& y)
{
    return vec_op<C, N>::div(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}

template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator+(const T1& x, const vec<T2, N>& y)
{
    return vec_op<C, N>::add(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator-(const T1& x, const vec<T2, N>& y)
{
    return vec_op<C, N>::sub(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator*(const T1& x, const vec<T2, N>& y)
{
    return vec_op<C, N>::mul(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator/(const T1& x, const vec<T2, N>& y)
{
    return vec_op<C, N>::div(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}

template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator+(const vec<T1, N>& x, const vec<T2, N>& y)
{
    return vec_op<C, N>::add(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator-(const vec<T1, N>& x, const vec<T2, N>& y)
{
    return vec_op<C, N>::sub(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator*(const vec<T1, N>& x, const vec<T2, N>& y)
{
    return vec_op<C, N>::mul(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}
template <typename T1, typename T2, size_t N, typename C = common_type<T1, T2>>
constexpr CMT_INLINE vec<C, N> operator/(const vec<T1, N>& x, const vec<T2, N>& y)
{
    return vec_op<C, N>::div(*static_cast<vec<C, N>>(x), *static_cast<vec<C, N>>(y));
}
}

template <typename T, size_t N>
struct mask : public vec<T, N>
{
    using UT                      = utype<T>;
    using type                    = T;
    constexpr static size_t width = N;

    using base = vec<T, N>;

    constexpr CMT_INLINE mask() noexcept : base() {}

    constexpr CMT_INLINE mask(simd<T, N> value) noexcept : base(value) {}
    template <size_t N1, size_t... Ns>
    constexpr CMT_INLINE mask(const mask<T, N1>& mask1, const mask<T, Ns>&... masks) noexcept
        : base(*concat(mask1, masks...))
    {
    }
    template <typename... Ts, typename = enable_if<sizeof...(Ts) + 2 == N>>
    constexpr CMT_INLINE mask(bool x, bool y, Ts... rest) noexcept
        : base{ internal::maskbits<T>(x), internal::maskbits<T>(y), internal::maskbits<T>(rest)... }
    {
    }
    constexpr CMT_INLINE mask(const mask&) noexcept = default;
    constexpr CMT_INLINE mask(mask&&) noexcept      = default;
    CMT_INLINE mask& operator=(const mask&) noexcept = default;
    CMT_INLINE mask& operator=(mask&&) noexcept = default;

    template <typename M, KFR_ENABLE_IF(sizeof(T) == sizeof(M))>
    constexpr CMT_INLINE mask(const vec<M, N>& value) : base(bitcast<T>(value))
    {
    }

    friend constexpr CMT_INLINE mask operator&(const mask& x, const mask& y)
    {
        return vec_op<T, N>::band(x.v, y.v);
    }
    friend constexpr CMT_INLINE mask operator|(const mask& x, const mask& y)
    {
        return vec_op<T, N>::bor(x.v, y.v);
    }
    friend constexpr CMT_INLINE mask operator^(const mask& x, const mask& y)
    {
        return vec_op<T, N>::bxor(x.v, y.v);
    }
    friend constexpr CMT_INLINE mask operator~(const mask& x) { return vec_op<T, N>::bnot(x.v); }

    constexpr CMT_INLINE mask operator&&(const mask& x) const { return *this & x; }
    constexpr CMT_INLINE mask operator||(const mask& x) const { return *this | x; }
    constexpr CMT_INLINE mask operator!() const { return ~*this; }

    constexpr CMT_INLINE simd<T, N> operator*() const { return this->v; }

    CMT_INLINE vec<T, N>& asvec() { return ref_cast<mask>(*this); }
    CMT_INLINE const vec<T, N>& asvec() const { return ref_cast<mask>(*this); }

    template <typename U, KFR_ENABLE_IF(sizeof(T) == sizeof(U))>
    CMT_INLINE operator mask<U, N>() const
    {
        return bitcast<U>(*this);
    }

    CMT_INLINE bool operator[](size_t index) const { return ibitcast(this->v[index]) < 0; }
};

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

template <size_t start, size_t count, typename T, size_t N>
CMT_INLINE vec<T, count> concatexact(const vec<T, N>& x, const vec<T, N>& y)
{
    return kfr::shufflevector<count, internal::shuffle_index<start>>(x, y);
}

template <size_t start, size_t count, typename T, size_t N1, size_t N2>
CMT_INLINE enable_if<(N1 == N2), vec<T, count>> concattwo(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return concatexact<start, count>(x, y);
}

template <size_t start, size_t count, typename T, size_t N1, size_t N2>
CMT_INLINE enable_if<(N1 > N2), vec<T, count>> concattwo(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return concatexact<start, count>(x, shufflevector<N1, internal::shuffle_index_extend<0, N2>>(y));
}
template <size_t start, size_t count, typename T, size_t N1, size_t N2>
CMT_INLINE enable_if<(N1 < N2), vec<T, count>> concattwo(const vec<T, N1>& x, const vec<T, N2>& y)
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
CMT_INLINE vec<T, N> concat(const vec<T, N>& x)
{
    return x;
}

template <typename T, size_t N1, size_t N2>
CMT_INLINE vec<T, N1 + N2> concat(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return concattwo<0, N1 + N2>(x, y);
}

template <typename T, size_t N1, size_t N2, size_t... Sizes>
CMT_INLINE auto concat(const vec<T, N1>& x, const vec<T, N2>& y, const vec<T, Sizes>&... args)
{
    return concat(x, concat(y, args...));
}
}

template <typename T, size_t N, size_t... Sizes>
CMT_INLINE vec<T, N + csum(csizes<Sizes...>)> concat(const vec<T, N>& x, const vec<T, Sizes>&... rest)
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
    return internal::apply_helper<T, N>(std::forward<Fn>(fn), csizeseq<N>, arg, std::forward<Args>(args)...);
}

template <size_t N, typename Fn, typename T = result_of<Fn()>>
constexpr CMT_INLINE vec<T, N> apply(Fn&& fn)
{
    return internal::apply0_helper<T, N>(std::forward<Fn>(fn), csizeseq<N>);
}

template <typename T, int N>
CMT_INLINE vec<T, N> tovec(const simd<T, N>& x)
{
    return x;
}
template <typename T, size_t N>
CMT_INLINE vec<T, N> tovec(const mask<T, N>& x)
{
    return *x;
}

#ifdef CMT_ARCH_SSE2
CMT_INLINE f32x4 tovec(__m128 x) { return f32x4(x); }
CMT_INLINE f64x2 tovec(__m128d x) { return f64x2(x); }
#endif

template <typename T, typename... Args, size_t Nout = (sizeof...(Args) + 1)>
constexpr CMT_INLINE mask<T, Nout> make_mask(bool arg, Args... args)
{
    simd<T, Nout> temp{ internal::maskbits<T>(arg), internal::maskbits<T>(static_cast<bool>(args))... };
    return temp;
}
KFR_FN(make_mask)

template <typename T, size_t N>
constexpr CMT_INLINE vec<T, N> zerovector()
{
    constexpr size_t width = N * compound_type_traits<T>::width;
    return compcast<T>(vec<subtype<T>, width>(simd<subtype<T>, width>()));
}

template <typename T, size_t N>
constexpr CMT_INLINE vec<T, N> zerovector(vec_t<T, N>)
{
    return zerovector<T, N>();
}
KFR_FN(zerovector)

template <typename T, size_t N>
constexpr CMT_INLINE vec<T, N> allonesvector()
{
    return zerovector<T, N>() == zerovector<T, N>();
}
template <typename T, size_t N>
constexpr CMT_INLINE vec<T, N> allonesvector(vec_t<T, N>)
{
    return allonesvector<T, N>();
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

template <typename T, size_t N, size_t Nout = prev_poweroftwo(N - 1)>
CMT_INLINE vec<T, Nout> low(const vec<T, N>& x)
{
    return shufflevector<Nout, internal::shuffle_index<>>(x);
}

template <typename T, size_t N, size_t Nout = prev_poweroftwo(N - 1)>
CMT_INLINE vec_t<T, Nout> low(vec_t<T, N>)
{
    return {};
}

template <typename T, size_t N, size_t Nout = N - prev_poweroftwo(N - 1)>
CMT_INLINE vec<T, Nout> high(const vec<T, N>& x)
{
    return shufflevector<Nout, internal::shuffle_index<prev_poweroftwo(N - 1)>>(x);
}

template <typename T, size_t N, size_t Nout = N - prev_poweroftwo(N - 1)>
CMT_INLINE vec_t<T, Nout> high(vec_t<T, N>)
{
    return {};
}
KFR_FN(low)
KFR_FN(high)
}

#pragma clang diagnostic pop

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
    using deep_rebind = kfr::vec_t<cometa::deep_rebind<subtype, U>, N>;
};

#ifdef KFR_SIMD_PARAM_ARE_DEDUCIBLE
template <typename T, size_t N>
struct compound_type_traits<kfr::simd<T, N>>
{
    using subtype                      = T;
    using deep_subtype                 = cometa::deep_subtype<T>;
    constexpr static size_t width      = N;
    constexpr static size_t deep_width = width * compound_type_traits<T>::width;
    constexpr static bool is_scalar    = false;
    constexpr static size_t depth      = cometa::compound_type_traits<T>::depth + 1;
    template <typename U>
    using rebind = kfr::simd<U, N>;
    template <typename U>
    using deep_rebind = kfr::simd<cometa::deep_rebind<subtype, U>, N>;

    CMT_INLINE static constexpr const subtype& at(const kfr::simd<T, N>& value, size_t index)
    {
        return value[index];
    }
};
#endif

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
    using deep_rebind = kfr::vec<cometa::deep_rebind<subtype, U>, N>;

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
    using deep_rebind = kfr::mask<cometa::deep_rebind<subtype, U>, N>;

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

template <typename T1, typename T2, size_t N>
struct common_type<kfr::mask<T1, N>, kfr::mask<T2, N>>
{
    using type = kfr::mask<typename common_type<T1, T2>::type, N>;
};
}
