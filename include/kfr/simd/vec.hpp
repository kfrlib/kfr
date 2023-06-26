/** @addtogroup types
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

#include "../cometa/string.hpp"
#include "../version.hpp"
#include "constants.hpp"
#include "impl/backend.hpp"

/**
 *  @brief Internal macro for functions
 */
#define KFR_FN(FN)                                                                                           \
    namespace fn                                                                                             \
    {                                                                                                        \
    struct FN                                                                                                \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        CMT_INLINE_MEMBER decltype(::kfr::FN(std::declval<Args>()...)) operator()(Args&&... args) const      \
        {                                                                                                    \
            return ::kfr::FN(std::forward<Args>(args)...);                                                   \
        }                                                                                                    \
    };                                                                                                       \
    }

/**
 *  @brief Internal macro for functions
 */
#define KFR_I_FN(FN)                                                                                         \
    namespace fn                                                                                             \
    {                                                                                                        \
    struct FN                                                                                                \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        CMT_INLINE_MEMBER decltype(::kfr::intrinsics::FN(std::declval<Args>()...)) operator()(               \
            Args&&... args) const                                                                            \
        {                                                                                                    \
            return ::kfr::intrinsics::FN(std::forward<Args>(args)...);                                       \
        }                                                                                                    \
    };                                                                                                       \
    }

#define KFR_I_FN_FULL(FN, FULLFN)                                                                            \
    namespace fn                                                                                             \
    {                                                                                                        \
    struct FN                                                                                                \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        CMT_INLINE_MEMBER decltype(FULLFN(std::declval<Args>()...)) operator()(Args&&... args) const         \
        {                                                                                                    \
            return FULLFN(std::forward<Args>(args)...);                                                      \
        }                                                                                                    \
    };                                                                                                       \
    }

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

template <typename T, size_t N>
struct alignas(next_poweroftwo(sizeof(T)) * next_poweroftwo(N)) portable_vec
{
    static constexpr vec_shape<T, N> shape() CMT_NOEXCEPT { return {}; }

    constexpr portable_vec() = default;

    constexpr portable_vec(T value) : portable_vec(csizeseq<N>, value) {}

    template <typename... Ts, size_t NN = N, KFR_ENABLE_IF(NN >= 2)>
    constexpr portable_vec(T v1, T v2, Ts... args) : elem{ v1, v2, static_cast<T>(args)... }
    {
    }

    static_assert(N > 0 && N <= 1024, "Invalid vector size");

    static_assert(is_simd_type<T> || !compound_type_traits<T>::is_scalar, "Invalid vector type");

    // type and size
    using value_type = T;

    constexpr static size_t size() CMT_NOEXCEPT { return N; }

    T elem[N];

    constexpr T operator[](size_t index) const { return elem[index]; }
    constexpr T& operator[](size_t index) { return elem[index]; }
    constexpr T front() const { return elem[0]; }
    constexpr T& front() { return elem[0]; }
    constexpr T back() const { return elem[N - 1]; }
    constexpr T& back() { return elem[N - 1]; }

private:
    template <size_t... indices>
    constexpr portable_vec(csizes_t<indices...>, T value) : elem{ (static_cast<void>(indices), value)... }
    {
    }
};

inline namespace CMT_ARCH_NAME
{

template <typename T, size_t N>
struct vec;

template <typename T>
constexpr inline size_t vec_rank = 0;

template <typename T, size_t N>
struct vec_halves
{
    vec<T, prev_poweroftwo(N - 1)> low;
    vec<T, N - prev_poweroftwo(N - 1)> high;
};

template <typename T>
struct vec_halves<T, 1>
{
    T val;
};

namespace internal
{
enum class conv_t
{
    promote,
    broadcast,
};

// scalar to scalar
template <size_t ToRank, size_t FromRank, typename To, typename From, conv_t conv>
struct conversion
{
};

template <typename To, typename From, conv_t conv>
struct conversion<0, 0, To, From, conv>
{
    static_assert(std::is_convertible_v<From, To>);

    static To cast(const From& value) { return value; }
};

template <typename T>
struct compoundcast
{
    static vec<T, 1> to_flat(const T& x) { return vec<T, 1>(x); }

    static T from_flat(const vec<T, 1>& x) { return x.front(); }
};

template <typename T, size_t N>
struct compoundcast<vec<T, N>>
{
    static const vec<T, N>& to_flat(const vec<T, N>& x) { return x; }

    static const vec<T, N>& from_flat(const vec<T, N>& x) { return x; }
};

template <typename T, size_t N1, size_t N2>
struct compoundcast<vec<vec<T, N1>, N2>>
{
    static vec<T, N1 * N2> to_flat(const vec<vec<T, N1>, N2>& x) { return x.v; }

    static vec<vec<T, N1>, N2> from_flat(const vec<T, N1 * N2>& x) { return x.v; }
};

template <typename T, size_t N_>
inline constexpr size_t vec_alignment =
    const_max(alignof(intrinsics::simd<typename compound_type_traits<T>::deep_subtype,
                                       const_max(size_t(1), N_) * compound_type_traits<T>::deep_width>),
              const_min(size_t(platform<>::native_vector_alignment),
                        next_poweroftwo(sizeof(typename compound_type_traits<T>::deep_subtype) *
                                        const_max(size_t(1), N_) * compound_type_traits<T>::deep_width)));

} // namespace internal

template <typename T, size_t N_>
struct alignas(internal::vec_alignment<T, N_>) vec
{
    static_assert(N_ > 0, "vec<T, N>: vector width cannot be zero");

    constexpr static inline size_t N = const_max(size_t(1), N_);
    static constexpr vec_shape<T, N> shape() CMT_NOEXCEPT { return {}; }

    // type and size
    using value_type = T;

    constexpr static size_t size() CMT_NOEXCEPT { return N; }

    using ST          = typename compound_type_traits<T>::deep_subtype;
    using scalar_type = ST;

    constexpr static inline size_t SW = compound_type_traits<T>::deep_width;
    constexpr static inline size_t SN = N * SW;

    constexpr static size_t scalar_size() CMT_NOEXCEPT { return SN; }

    static_assert(is_simd_type<scalar_type>, "Invalid vector type");

    static_assert(scalar_size() > 0 && scalar_size() <= 1024, "Invalid vector size");

    using mask_t = mask<T, N>;

    using simd_type    = intrinsics::simd<ST, SN>;
    using uvalue_type  = utype<T>;
    using iuvalue_type = std::conditional_t<is_i_class<T>, T, uvalue_type>;

    using uscalar_type  = utype<ST>;
    using iuscalar_type = std::conditional_t<is_i_class<ST>, ST, uscalar_type>;

    using usimd_type  = intrinsics::simd<uscalar_type, SN>;
    using iusimd_type = intrinsics::simd<iuscalar_type, SN>;

    // constructors and assignment
    // from SIMD
    KFR_MEM_INTRINSIC vec(const simd_type& simd) CMT_NOEXCEPT : v(simd) {}
    // default
    KFR_MEM_INTRINSIC constexpr vec() CMT_NOEXCEPT {}

#if defined(_MSC_VER) && !defined(__clang__)
    // MSVC Internal Compiler Error workaround
    // copy
    KFR_MEM_INTRINSIC constexpr vec(const vec& value) CMT_NOEXCEPT : v(value.v) {}
    // move
    KFR_MEM_INTRINSIC constexpr vec(vec&& value) CMT_NOEXCEPT : v(value.v) {}
    // assignment
    KFR_MEM_INTRINSIC constexpr vec& operator=(const vec& value) CMT_NOEXCEPT
    {
        v = value.v;
        return *this;
    }
    // assignment
    KFR_MEM_INTRINSIC constexpr vec& operator=(vec&& value) CMT_NOEXCEPT
    {
        v = value.v;
        return *this;
    }
#else
    // copy
    KFR_MEM_INTRINSIC constexpr vec(const vec&) CMT_NOEXCEPT = default;
    // move
    KFR_MEM_INTRINSIC constexpr vec(vec&&) CMT_NOEXCEPT = default;
    // assignment
    KFR_MEM_INTRINSIC constexpr vec& operator=(const vec&) CMT_NOEXCEPT = default;
    // assignment
    KFR_MEM_INTRINSIC constexpr vec& operator=(vec&&) CMT_NOEXCEPT = default;
#endif

    // from scalar
    template <typename U,
              KFR_ENABLE_IF(std::is_convertible_v<U, value_type>&& compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC vec(const U& s) CMT_NOEXCEPT
        : v(intrinsics::simd_broadcast(intrinsics::simd_t<unwrap_bit<ST>, SN>{},
                                       static_cast<unwrap_bit<ST>>(static_cast<ST>(s))))
    {
    }

    template <typename U,
              KFR_ENABLE_IF(std::is_convertible_v<U, value_type> && !compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC vec(const U& s) CMT_NOEXCEPT
        : v(intrinsics::simd_shuffle(intrinsics::simd_t<unwrap_bit<ST>, SW>{},
                                     internal::compoundcast<T>::to_flat(static_cast<T>(s)).v,
                                     csizeseq<SN> % csize<SW>, overload_auto))
    {
    }

    // from list
    template <typename... Us, KFR_ENABLE_IF(sizeof...(Us) <= 1022 && compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC vec(const value_type& s0, const value_type& s1, const Us&... rest) CMT_NOEXCEPT
        : v(intrinsics::simd_make(cometa::ctype<T>, s0, s1, static_cast<value_type>(rest)...))
    {
    }

    template <typename... Us, KFR_ENABLE_IF(sizeof...(Us) <= 1022 && !compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC vec(const value_type& s0, const value_type& s1, const Us&... rest) CMT_NOEXCEPT
        : v(intrinsics::simd_concat<ST, size_t(SW), size_t(SW), just_value<Us, size_t>(SW)...>(
              internal::compoundcast<T>::to_flat(s0).v, internal::compoundcast<T>::to_flat(s1).v,
              internal::compoundcast<T>::to_flat(static_cast<T>(rest)).v...))
    {
    }

    // from vector of another type
    template <typename U, KFR_ENABLE_IF(std::is_convertible_v<U, value_type> &&
                                        (compound_type_traits<T>::is_scalar && !is_bit<U>))>
    KFR_MEM_INTRINSIC vec(const vec<U, N>& x) CMT_NOEXCEPT
        : v(intrinsics::simd_convert(
              intrinsics::simd_cvt_t<unwrap_bit<ST>, unwrap_bit<deep_subtype<U>>, SN>{}, x.v))
    {
    }

    // from lambda
    template <typename Fn, KFR_ENABLE_IF(std::is_invocable_r_v<T, Fn, size_t>)>
    KFR_MEM_INTRINSIC vec(Fn&& fn) CMT_NOEXCEPT
    {
        for (size_t i = 0; i < N; ++i)
        {
            auto v = fn(i);
            set(i, v);
        }
    }

    template <typename U, KFR_ENABLE_IF(std::is_convertible_v<U, value_type> &&
                                        !(compound_type_traits<T>::is_scalar && !is_bit<U>))>
    KFR_MEM_INTRINSIC vec(const vec<U, N>& x) CMT_NOEXCEPT
        : v(internal::conversion<vec_rank<T> + 1, vec_rank<U> + 1, vec<T, N>, vec<U, N>,
                                 internal::conv_t::promote>::cast(x)
                .v)
    {
    }

    // from list of vectors
    template <size_t... Ns, typename = std::enable_if_t<csum<size_t, Ns...>() == N>>
    KFR_MEM_INTRINSIC vec(const vec<T, Ns>&... vs) CMT_NOEXCEPT
        : v(intrinsics::simd_concat<ST, (SW * Ns)...>(vs.v...))
    {
    }

    KFR_MEM_INTRINSIC vec(const portable_vec<T, N>& p) CMT_NOEXCEPT : vec(bitcast_anything<vec>(p)) {}

    KFR_MEM_INTRINSIC operator portable_vec<T, N>() const CMT_NOEXCEPT
    {
        return bitcast_anything<portable_vec<T, N>>(*this);
    }

    KFR_MEM_INTRINSIC vec(czeros_t) CMT_NOEXCEPT : v(intrinsics::simd_zeros<ST, SN>()) {}

    KFR_MEM_INTRINSIC vec(cones_t) CMT_NOEXCEPT : v(intrinsics::simd_allones<ST, SN>()) {}

    template <typename U, size_t M, KFR_ENABLE_IF(sizeof(U) * M == sizeof(T) * N)>
    KFR_MEM_INTRINSIC static vec frombits(const vec<U, M>& v) CMT_NOEXCEPT
    {
        return intrinsics::simd_bitcast(
            intrinsics::simd_cvt_t<ST, typename vec<U, M>::scalar_type, vec<U, M>::scalar_size()>{}, v.v);
    }

    // shuffle
    template <size_t... indices>
    KFR_MEM_INTRINSIC vec<value_type, sizeof...(indices)> shuffle(csizes_t<indices...> i) const CMT_NOEXCEPT
    {
        return vec<value_type, sizeof...(indices)>(intrinsics::simd_shuffle(
            intrinsics::simd_t<unwrap_bit<ST>, SN>{}, v, scale<SW>(i), overload_auto));
    }

    template <size_t... indices>
    KFR_MEM_INTRINSIC vec<value_type, sizeof...(indices)> shuffle(const vec& y,
                                                                  csizes_t<indices...> i) const CMT_NOEXCEPT
    {
        return vec<value_type, sizeof...(indices)>(
            intrinsics::simd_shuffle(intrinsics::simd2_t<ST, SN, SN>{}, v, y.v, scale<SW>(i), overload_auto));
    }

    // element access
    struct element;

    KFR_MEM_INTRINSIC constexpr value_type operator[](size_t index) const& CMT_NOEXCEPT { return get(index); }

    KFR_MEM_INTRINSIC constexpr value_type operator[](size_t index) && CMT_NOEXCEPT { return get(index); }

    KFR_MEM_INTRINSIC constexpr element operator[](size_t index) & CMT_NOEXCEPT { return { *this, index }; }

    KFR_MEM_INTRINSIC value_type front() const& CMT_NOEXCEPT { return get(csize<0>); }

    KFR_MEM_INTRINSIC value_type back() const& CMT_NOEXCEPT { return get(csize<N - 1>); }

    KFR_MEM_INTRINSIC value_type front() && CMT_NOEXCEPT { return get(csize<0>); }

    KFR_MEM_INTRINSIC value_type back() && CMT_NOEXCEPT { return get(csize<N - 1>); }

    KFR_MEM_INTRINSIC element front() & CMT_NOEXCEPT { return { *this, 0 }; }

    KFR_MEM_INTRINSIC element back() & CMT_NOEXCEPT { return { *this, N - 1 }; }

    template <int dummy = 0, KFR_ENABLE_IF(dummy == 0 && compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC constexpr value_type get(size_t index) const CMT_NOEXCEPT
    {
        return intrinsics::simd_get_element<T, N>(v, index);
    }

    template <int dummy = 0, typename = void,
              KFR_ENABLE_IF(dummy == 0 && !compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC constexpr value_type get(size_t index) const CMT_NOEXCEPT
    {
        union
        {
            simd_type v;
            T s[N];
        } u{ this->v };
        return u.s[index];
    }

    template <size_t index, KFR_ENABLE_IF(index < 1024 && compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC constexpr value_type get(csize_t<index>) const CMT_NOEXCEPT
    {
        return intrinsics::simd_get_element<T, N>(v, csize<index>);
    }

    template <size_t index, typename = void,
              KFR_ENABLE_IF(index < 1024 && !compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC constexpr value_type get(csize_t<index>) const CMT_NOEXCEPT
    {
        return internal::compoundcast<T>::from_flat(intrinsics::simd_shuffle(
            intrinsics::simd_t<unwrap_bit<ST>, SN>{}, v, csizeseq<SW, SW * index>, overload_auto));
    }

    template <size_t index>
    KFR_MEM_INTRINSIC constexpr value_type get() const CMT_NOEXCEPT
    {
        return this->get(csize_t<index>{});
    }

    template <int dummy = 0, KFR_ENABLE_IF(dummy == 0 && compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC constexpr void set(size_t index, const value_type& s) CMT_NOEXCEPT
    {
        v = intrinsics::simd_set_element<T, N>(v, index, s);
    }

    template <int dummy = 0, KFR_ENABLE_IF(dummy == 0 && !compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC constexpr void set(size_t index, const value_type& s) CMT_NOEXCEPT
    {
        union
        {
            simd_type v;
            T s[N];
        } u{ this->v };
        u.s[index] = s;
        this->v    = u.v;
    }

    template <size_t index, KFR_ENABLE_IF(index < 1024 && compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC constexpr void set(csize_t<index>, const value_type& s) CMT_NOEXCEPT
    {
        v = intrinsics::simd_set_element<T, N>(v, csize<index>, s);
    }

    template <size_t index, typename = void,
              KFR_ENABLE_IF(index < 1024 && !compound_type_traits<T>::is_scalar)>
    KFR_MEM_INTRINSIC constexpr void set(csize_t<index>, const value_type& s) CMT_NOEXCEPT
    {
        this->s[index] = s;
    }

    struct element
    {
        constexpr operator value_type() const CMT_NOEXCEPT { return v.get(index); }

        KFR_MEM_INTRINSIC element& operator=(const value_type& s) CMT_NOEXCEPT
        {
            v.set(index, s);
            return *this;
        }

        KFR_MEM_INTRINSIC element& operator+=(const value_type& s) CMT_NOEXCEPT
        {
            v.set(index, v.get(index) + s);
            return *this;
        }
        KFR_MEM_INTRINSIC element& operator-=(const value_type& s) CMT_NOEXCEPT
        {
            v.set(index, v.get(index) - s);
            return *this;
        }
        KFR_MEM_INTRINSIC element& operator*=(const value_type& s) CMT_NOEXCEPT
        {
            v.set(index, v.get(index) * s);
            return *this;
        }
        KFR_MEM_INTRINSIC element& operator/=(const value_type& s) CMT_NOEXCEPT
        {
            v.set(index, v.get(index) / s);
            return *this;
        }
        KFR_MEM_INTRINSIC element& operator++() CMT_NOEXCEPT
        {
            v.set(index, v.get(index) + 1);
            return *this;
        }
        KFR_MEM_INTRINSIC element& operator--() CMT_NOEXCEPT
        {
            v.set(index, v.get(index) - 1);
            return *this;
        }
        KFR_MEM_INTRINSIC value_type operator++(int) CMT_NOEXCEPT
        {
            value_type val = v.get(index) + 1;
            v.set(index, val);
            return val;
        }
        KFR_MEM_INTRINSIC value_type operator--(int) CMT_NOEXCEPT
        {
            value_type val = v.get(index) - 1;
            v.set(index, val);
            return val;
        }

        KFR_MEM_INTRINSIC element& operator=(const element& s) CMT_NOEXCEPT
        {
            v.set(index, static_cast<value_type>(s));
            return *this;
        }

        template <typename U, size_t M>
        KFR_MEM_INTRINSIC element& operator=(const typename vec<U, M>::element& s) CMT_NOEXCEPT
        {
            v.set(index, static_cast<value_type>(static_cast<U>(s)));
            return *this;
        }

        vec& v;
        size_t index;
    };

    // read/write
    template <bool aligned = false>
    KFR_MEM_INTRINSIC explicit constexpr vec(const value_type* src,
                                             cbool_t<aligned> = cbool_t<aligned>()) CMT_NOEXCEPT;

    template <bool aligned = false>
    KFR_MEM_INTRINSIC const vec& write(value_type* dest,
                                       cbool_t<aligned> = cbool_t<aligned>()) const CMT_NOEXCEPT;

    KFR_MEM_INTRINSIC vec<ST, SN> flatten() const CMT_NOEXCEPT { return v; }

    KFR_MEM_INTRINSIC static vec from_flatten(const vec<ST, SN>& x) { return vec(x.v); }

    KFR_MEM_INTRINSIC constexpr mask_t asmask() const CMT_NOEXCEPT { return mask_t(v); }

    KFR_MEM_INTRINSIC constexpr vec<unwrap_bit<T>, N> asvec() const CMT_NOEXCEPT
    {
        return vec<unwrap_bit<T>, N>(v);
    }

    constexpr static size_t simd_element_size  = const_min(vector_width<T>, N);
    constexpr static size_t simd_element_count = N / simd_element_size;
    using simd_element_type                    = simd<ST, simd_element_size>;

public:
    union
    {
        simd_type v;
        vec_halves<T, N> h;
        // simd_element_type w[simd_element_count];
        // T s[N];
    };
};

template <typename... T>
vec(T&&...) -> vec<std::common_type_t<T...>, sizeof...(T)>;

template <typename T>
constexpr inline bool is_vec_element = is_simd_type<deep_subtype<std::remove_const_t<T>>>;

template <typename T, size_t N, size_t... indices>
KFR_INTRINSIC vec<T, sizeof...(indices)> shufflevector(const vec<T, N>& x,
                                                       csizes_t<indices...> i) CMT_NOEXCEPT
{
    return intrinsics::simd_shuffle(intrinsics::simd_t<unwrap_bit<T>, N>{}, x.v, i, overload_auto);
}

template <typename T, size_t N, size_t... indices>
KFR_INTRINSIC vec<T, sizeof...(indices)> shufflevectors(const vec<T, N>& x, const vec<T, N>& y,
                                                        csizes_t<indices...> i) CMT_NOEXCEPT
{
    return intrinsics::simd_shuffle(intrinsics::simd2_t<T, N, N>{}, x.v, y.v, i, overload_auto);
}

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
} // namespace internal

template <typename T>
constexpr inline bool is_vec = internal::is_vec_impl<T>::value;

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wold-style-cast")

template <size_t N, typename T>
constexpr KFR_INTRINSIC vec<T, N> broadcast(T x)
{
    return x;
}

CMT_PRAGMA_GNU(GCC diagnostic pop)

namespace internal
{

template <typename To, typename From, size_t N, typename Tsub = deep_subtype<To>,
          size_t Nout = (N * compound_type_traits<To>::deep_width)>
constexpr KFR_INTRINSIC vec<To, N> builtin_convertvector(const vec<From, N>& value) CMT_NOEXCEPT
{
    return vec<To, N>(value);
}

// vector to vector
template <typename To, typename From, size_t N, size_t N2, conv_t conv>
struct conversion<1, 1, vec<To, N>, vec<From, N2>, conv>
{
    static_assert(N == N2, "");
    static_assert(!is_compound_type<To>, "");
    static_assert(!is_compound_type<From>, "");

    static vec<To, N> cast(const vec<From, N>& value) { return vec<To, N>(value); }
};

// scalar to vector
template <typename To, typename From, size_t N, conv_t conv>
struct conversion<1, 0, vec<To, N>, From, conv>
{
    static_assert(std::is_convertible_v<From, To>, "");

    static vec<To, N> cast(const From& value) { return broadcast<N>(static_cast<To>(value)); }
};
} // namespace internal

template <typename T>
constexpr size_t size_of() CMT_NOEXCEPT
{
    return sizeof(deep_subtype<T>) * compound_type_traits<T>::deep_width;
}

template <typename From, size_t N, typename Tsub = deep_subtype<From>,
          size_t Nout = N* size_of<From>() / size_of<Tsub>()>
constexpr KFR_INTRINSIC vec<Tsub, Nout> flatten(const vec<From, N>& x) CMT_NOEXCEPT
{
    return x.flatten();
}

template <typename To, typename From,
          typename Tout = typename compound_type_traits<From>::template deep_rebind<To>>
constexpr KFR_INTRINSIC Tout cast(const From& value) CMT_NOEXCEPT
{
    return static_cast<Tout>(value);
}

template <typename Tout, typename Tin, size_t N, KFR_ENABLE_IF(!std::is_same_v<Tin, Tout>)>
constexpr KFR_INTRINSIC vec<Tout, N> cast(const vec<Tin, N>& value) CMT_NOEXCEPT
{
    return vec<Tout, N>(value);
}

template <typename Tout, typename Tin, size_t N1, size_t N2, KFR_ENABLE_IF(!std::is_same_v<Tin, Tout>)>
constexpr KFR_INTRINSIC vec<vec<Tout, N1>, N2> cast(const vec<vec<Tin, N1>, N2>& value) CMT_NOEXCEPT
{
    return vec<vec<Tout, N1>, N2>(value);
}

template <typename Tout, typename Tin, size_t N, KFR_ENABLE_IF(std::is_same_v<Tin, Tout>)>
constexpr KFR_INTRINSIC const vec<Tin, N>& cast(const vec<Tin, N>& value) CMT_NOEXCEPT
{
    return value;
}

template <typename Tout, typename Tin, size_t N1, size_t N2, KFR_ENABLE_IF(std::is_same_v<Tin, Tout>)>
constexpr KFR_INTRINSIC const vec<vec<Tin, N1>, N2>& cast(const vec<vec<Tin, N1>, N2>& value) CMT_NOEXCEPT
{
    return value;
}

//

template <typename To, typename From,
          typename Tout = typename compound_type_traits<From>::template deep_rebind<To>>
constexpr KFR_INTRINSIC Tout broadcastto(const From& value) CMT_NOEXCEPT
{
    return static_cast<Tout>(value);
}

template <typename Tout, typename Tin, size_t N, KFR_ENABLE_IF(!std::is_same_v<Tin, Tout>)>
constexpr KFR_INTRINSIC vec<Tout, N> broadcastto(const vec<Tin, N>& value) CMT_NOEXCEPT
{
    return internal::conversion<vec_rank<Tout> + 1, 1, vec<Tout, N>, vec<Tin, N>,
                                internal::conv_t::broadcast>::cast(value);
}

template <typename Tout, typename Tin, size_t N1, size_t N2, KFR_ENABLE_IF(!std::is_same_v<Tin, Tout>)>
constexpr KFR_INTRINSIC vec<vec<Tout, N1>, N2> broadcastto(const vec<vec<Tin, N1>, N2>& value) CMT_NOEXCEPT
{
    return internal::conversion<vec_rank<Tout> + 2, 2, vec<vec<Tout, N1>, N2>, vec<vec<Tin, N1>, N2>,
                                internal::conv_t::broadcast>::cast(value);
}

template <typename Tout, typename Tin, size_t N, KFR_ENABLE_IF(std::is_same_v<Tin, Tout>)>
constexpr KFR_INTRINSIC const vec<Tin, N>& broadcastto(const vec<Tin, N>& value) CMT_NOEXCEPT
{
    return value;
}

template <typename Tout, typename Tin, size_t N1, size_t N2, KFR_ENABLE_IF(std::is_same_v<Tin, Tout>)>
constexpr KFR_INTRINSIC const vec<vec<Tin, N1>, N2>& broadcastto(const vec<vec<Tin, N1>, N2>& value)
    CMT_NOEXCEPT
{
    return value;
}

//
template <typename Tout, typename Tin>
constexpr KFR_INTRINSIC Tout promoteto(const Tin& value) CMT_NOEXCEPT
{
    return static_cast<Tout>(value);
}

template <typename Tout, typename Tin, size_t N>
constexpr KFR_INTRINSIC vec<Tout, N> promoteto(const vec<Tin, N>& value) CMT_NOEXCEPT
{
    if constexpr (std::is_same_v<Tin, Tout>)
        return value;
    else
        return internal::conversion<vec_rank<Tout> + 1, 1, vec<Tout, N>, vec<Tin, N>,
                                    internal::conv_t::promote>::cast(value);
}

template <typename Tout, typename Tin, size_t N1, size_t N2>
constexpr KFR_INTRINSIC vec<Tout, N2> promoteto(const vec<vec<Tin, N1>, N2>& value) CMT_NOEXCEPT
{
    if constexpr (std::is_same_v<Tin, Tout>)
        return value;
    else
        return internal::conversion<vec_rank<Tout> + 1, 2, vec<Tout, N2>, vec<vec<Tin, N1>, N2>,
                                    internal::conv_t::promote>::cast(value);
}

template <typename To, typename From>
CMT_GNU_CONSTEXPR KFR_INTRINSIC To bitcast(const From& value) CMT_NOEXCEPT
{
    static_assert(sizeof(From) == sizeof(To), "bitcast: Incompatible types");
    union
    {
        From from;
        To to;
    } u{ value };
    return u.to;
}

template <typename To, typename From, size_t N, size_t Nout = (N * size_of<From>() / size_of<To>())>
CMT_GNU_CONSTEXPR KFR_INTRINSIC vec<To, Nout> bitcast(const vec<From, N>& value) CMT_NOEXCEPT
{
    return vec<To, Nout>::frombits(value);
}

template <typename From, typename To = utype<From>, KFR_ENABLE_IF(!is_compound_type<From>)>
constexpr KFR_INTRINSIC To ubitcast(const From& value) CMT_NOEXCEPT
{
    return bitcast<To>(value);
}

template <typename From, typename To = itype<From>, KFR_ENABLE_IF(!is_compound_type<From>)>
constexpr KFR_INTRINSIC To ibitcast(const From& value) CMT_NOEXCEPT
{
    return bitcast<To>(value);
}

template <typename From, typename To = ftype<From>, KFR_ENABLE_IF(!is_compound_type<From>)>
constexpr KFR_INTRINSIC To fbitcast(const From& value) CMT_NOEXCEPT
{
    return bitcast<To>(value);
}

template <typename From, typename To = uitype<From>, KFR_ENABLE_IF(!is_compound_type<From>)>
constexpr KFR_INTRINSIC To uibitcast(const From& value) CMT_NOEXCEPT
{
    return bitcast<To>(value);
}

template <typename From, size_t N, typename To = utype<From>,
          size_t Nout = size_of<From>() * N / size_of<To>()>
constexpr KFR_INTRINSIC vec<To, Nout> ubitcast(const vec<From, N>& value) CMT_NOEXCEPT
{
    return vec<To, Nout>::frombits(value);
}

template <typename From, size_t N, typename To = itype<From>,
          size_t Nout = size_of<From>() * N / size_of<To>()>
constexpr KFR_INTRINSIC vec<To, Nout> ibitcast(const vec<From, N>& value) CMT_NOEXCEPT
{
    return vec<To, Nout>::frombits(value);
}

template <typename From, size_t N, typename To = ftype<From>,
          size_t Nout = size_of<From>() * N / size_of<To>()>
constexpr KFR_INTRINSIC vec<To, Nout> fbitcast(const vec<From, N>& value) CMT_NOEXCEPT
{
    return vec<To, Nout>::frombits(value);
}

template <typename From, size_t N, typename To = uitype<From>,
          size_t Nout = size_of<From>() * N / size_of<To>()>
constexpr KFR_INTRINSIC vec<To, Nout> uibitcast(const vec<From, N>& value) CMT_NOEXCEPT
{
    return vec<To, Nout>::frombits(value);
}

constexpr KFR_INTRINSIC size_t vector_alignment(size_t size) { return next_poweroftwo(size); }

template <typename T, size_t N>
struct pkd_vec
{
    constexpr pkd_vec() CMT_NOEXCEPT {}

    pkd_vec(const vec<T, N>& value) CMT_NOEXCEPT { value.write(v); }

    template <typename... Ts>
    constexpr pkd_vec(Ts... init) CMT_NOEXCEPT : v{ static_cast<T>(init)... }
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
constexpr KFR_INTRINSIC T make_vector_get_n()
{
    return T();
}

template <size_t index, typename T, typename... Args>
constexpr KFR_INTRINSIC T make_vector_get_n(const T& arg, const Args&... args)
{
    return index == 0 ? arg : make_vector_get_n<index - 1, T>(args...);
}

template <typename T, typename... Args, size_t... indices, size_t N = sizeof...(Args)>
CMT_GNU_CONSTEXPR KFR_INTRINSIC vec<T, N> make_vector_impl(csizes_t<indices...>, const Args&... args)
{
    static_assert(sizeof...(indices) == sizeof...(Args), "");
    const T list[] = { static_cast<T>(args)... };
    return vec<T, N>(list[indices]...);
}

template <bool, typename Tfallback, typename... Args>
struct conditional_common;

template <typename Tfallback, typename... Args>
struct conditional_common<true, Tfallback, Args...>
{
    using type = std::common_type_t<Args...>;
};

template <typename Tfallback, typename... Args>
struct conditional_common<false, Tfallback, Args...>
{
    using type = Tfallback;
};

} // namespace internal

/// Create vector from scalar values
/// @code
/// CHECK( make_vector( 1, 2, 3, 4 ) == i32x4{1, 2, 3, 4} );
/// @endcode
template <typename Type = void, typename Arg, typename... Args, size_t N = (sizeof...(Args) + 1),
          typename SubType =
              fix_type<typename internal::conditional_common<std::is_void_v<Type>, Type, Arg, Args...>::type>>
constexpr KFR_INTRINSIC vec<SubType, N> make_vector(const Arg& x, const Args&... rest)
{
    return internal::make_vector_impl<SubType>(cvalseq_t<size_t, N>(), static_cast<SubType>(x),
                                               static_cast<SubType>(rest)...);
}

template <typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> make_vector(const vec<T, N>& x)
{
    return x;
}

template <typename T, T... Values, size_t N = sizeof...(Values)>
constexpr KFR_INTRINSIC vec<T, N> make_vector(cvals_t<T, Values...>)
{
    return make_vector<T>(Values...);
}

template <typename Type = void, typename Arg, typename... Args, size_t N = (sizeof...(Args) + 1),
          typename SubType =
              fix_type<std::conditional_t<std::is_void_v<Type>, std::common_type_t<Arg, Args...>, Type>>,
          KFR_ENABLE_IF(is_number<subtype<SubType>>)>
constexpr KFR_INTRINSIC vec<SubType, N> pack(const Arg& x, const Args&... rest)
{
    return internal::make_vector_impl<SubType>(csizeseq<N>, static_cast<SubType>(x),
                                               static_cast<SubType>(rest)...);
}

using f32x1  = vec<f32, 1>;
using f32x2  = vec<f32, 2>;
using f32x3  = vec<f32, 3>;
using f32x4  = vec<f32, 4>;
using f32x8  = vec<f32, 8>;
using f32x16 = vec<f32, 16>;
using f32x32 = vec<f32, 32>;
using f32x64 = vec<f32, 64>;
using f64x1  = vec<f64, 1>;
using f64x2  = vec<f64, 2>;
using f64x3  = vec<f64, 3>;
using f64x4  = vec<f64, 4>;
using f64x8  = vec<f64, 8>;
using f64x16 = vec<f64, 16>;
using f64x32 = vec<f64, 32>;
using f64x64 = vec<f64, 64>;
using i8x1   = vec<i8, 1>;
using i8x2   = vec<i8, 2>;
using i8x3   = vec<i8, 3>;
using i8x4   = vec<i8, 4>;
using i8x8   = vec<i8, 8>;
using i8x16  = vec<i8, 16>;
using i8x32  = vec<i8, 32>;
using i8x64  = vec<i8, 64>;
using i16x1  = vec<i16, 1>;
using i16x2  = vec<i16, 2>;
using i16x3  = vec<i16, 3>;
using i16x4  = vec<i16, 4>;
using i16x8  = vec<i16, 8>;
using i16x16 = vec<i16, 16>;
using i16x32 = vec<i16, 32>;
using i16x64 = vec<i16, 64>;
using i32x1  = vec<i32, 1>;
using i32x2  = vec<i32, 2>;
using i32x3  = vec<i32, 3>;
using i32x4  = vec<i32, 4>;
using i32x8  = vec<i32, 8>;
using i32x16 = vec<i32, 16>;
using i32x32 = vec<i32, 32>;
using i32x64 = vec<i32, 64>;
using i64x1  = vec<i64, 1>;
using i64x2  = vec<i64, 2>;
using i64x3  = vec<i64, 3>;
using i64x4  = vec<i64, 4>;
using i64x8  = vec<i64, 8>;
using i64x16 = vec<i64, 16>;
using i64x32 = vec<i64, 32>;
using i64x64 = vec<i64, 64>;
using u8x1   = vec<u8, 1>;
using u8x2   = vec<u8, 2>;
using u8x3   = vec<u8, 3>;
using u8x4   = vec<u8, 4>;
using u8x8   = vec<u8, 8>;
using u8x16  = vec<u8, 16>;
using u8x32  = vec<u8, 32>;
using u8x64  = vec<u8, 64>;
using u16x1  = vec<u16, 1>;
using u16x2  = vec<u16, 2>;
using u16x3  = vec<u16, 3>;
using u16x4  = vec<u16, 4>;
using u16x8  = vec<u16, 8>;
using u16x16 = vec<u16, 16>;
using u16x32 = vec<u16, 32>;
using u16x64 = vec<u16, 64>;
using u32x1  = vec<u32, 1>;
using u32x2  = vec<u32, 2>;
using u32x3  = vec<u32, 3>;
using u32x4  = vec<u32, 4>;
using u32x8  = vec<u32, 8>;
using u32x16 = vec<u32, 16>;
using u32x32 = vec<u32, 32>;
using u32x64 = vec<u32, 64>;
using u64x1  = vec<u64, 1>;
using u64x2  = vec<u64, 2>;
using u64x3  = vec<u64, 3>;
using u64x4  = vec<u64, 4>;
using u64x8  = vec<u64, 8>;
using u64x16 = vec<u64, 16>;
using u64x32 = vec<u64, 32>;
using u64x64 = vec<u64, 64>;

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
} // namespace glsl_names
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
} // namespace opencl_names

namespace internal
{

template <size_t Index, typename T, size_t N, typename Fn, typename... Args,
          typename Tout = std::invoke_result_t<Fn, subtype<std::decay_t<Args>>...>>
constexpr KFR_INTRINSIC Tout applyfn_helper(Fn&& fn, Args&&... args)
{
    return fn(args[Index]...);
}

template <typename T, size_t N, typename Fn, typename... Args,
          typename Tout = std::invoke_result_t<Fn, subtype<std::decay_t<Args>>...>, size_t... Indices>
constexpr KFR_INTRINSIC vec<Tout, N> apply_helper(Fn&& fn, csizes_t<Indices...>, Args&&... args)
{
    return make_vector(applyfn_helper<Indices, T, N>(std::forward<Fn>(fn), std::forward<Args>(args)...)...);
}

template <typename T, size_t N, typename Fn, size_t... Indices>
constexpr KFR_INTRINSIC vec<T, N> apply0_helper(Fn&& fn, csizes_t<Indices...>)
{
    return make_vector(((void)Indices, void(), fn())...);
}
} // namespace internal

template <typename T, size_t N, typename Fn, typename... Args,
          typename Tout = std::invoke_result_t<Fn, T, subtype<std::decay_t<Args>>...>>
constexpr KFR_INTRINSIC vec<Tout, N> apply(Fn&& fn, const vec<T, N>& arg, Args&&... args)
{
    return internal::apply_helper<T, N>(std::forward<Fn>(fn), csizeseq<N>, arg, std::forward<Args>(args)...);
}

template <typename T, typename Fn, typename... Args,
          typename Tout = std::invoke_result_t<Fn, T, std::decay_t<Args>...>,
          KFR_ENABLE_IF(std::is_same_v<T, subtype<T>>)>
constexpr KFR_INTRINSIC Tout apply(Fn&& fn, const T& arg, Args&&... args)
{
    return fn(arg, args...);
}

template <size_t N, typename Fn, typename T = std::invoke_result_t<Fn>>
constexpr KFR_INTRINSIC vec<T, N> apply(Fn&& fn)
{
    return internal::apply0_helper<T, N>(std::forward<Fn>(fn), csizeseq<N>);
}

template <typename T, size_t N>
CMT_GNU_CONSTEXPR KFR_INTRINSIC vec<T, N> zerovector()
{
    return vec<T, N>(czeros);
}

template <typename T, size_t N>
CMT_GNU_CONSTEXPR KFR_INTRINSIC vec<T, N> zerovector(vec_shape<T, N>)
{
    return vec<T, N>(czeros);
}

template <typename T, size_t N>
CMT_GNU_CONSTEXPR KFR_INTRINSIC vec<T, N> zerovector(vec<T, N>)
{
    return vec<T, N>(czeros);
}

template <typename T, size_t N>
CMT_GNU_CONSTEXPR KFR_INTRINSIC vec<T, N> allonesvector()
{
    return vec<T, N>(cones);
}

template <typename T, size_t N>
CMT_GNU_CONSTEXPR KFR_INTRINSIC vec<T, N> allonesvector(vec_shape<T, N>)
{
    return vec<T, N>(cones);
}

template <typename T, size_t N>
CMT_GNU_CONSTEXPR KFR_INTRINSIC vec<T, N> allonesvector(vec<T, N>)
{
    return vec<T, N>(cones);
}

template <typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> undefinedvector()
{
    return vec<T, N>{};
}

template <typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> undefinedvector(vec_shape<T, N>)
{
    return undefinedvector<T, N>();
}

template <size_t N>
struct vec_template
{
    template <typename T>
    using type = vec<T, N>;
};

template <size_t N1, size_t N2>
struct vecvec_template
{
    template <typename T>
    using type = vec<vec<T, N1>, N2>;
};

#ifdef KFR_TESTING

inline const std::vector<special_value>& special_values()
{
    static const std::vector<special_value> values{ special_constant::infinity,
                                                    special_constant::neg_infinity,
                                                    special_constant::min,
                                                    special_constant::lowest,
                                                    special_constant::max,
                                                    3.1415926535897932384626433832795,
                                                    4.499999,
                                                    4.500001,
                                                    -4.499999,
                                                    -4.500001,
                                                    0.1111111111111111111111111111111,
                                                    -0.4444444444444444444444444444444,
                                                    -1,
                                                    0,
                                                    +1 };
    return values;
}

namespace test_catogories
{
constexpr cint_t<1> scalars{};
constexpr cint_t<2> vectors{};
constexpr cint_t<3> all{};

constexpr inline auto types(cint_t<0>) { return ctypes_t<>{}; }

constexpr inline auto types(cint_t<1>) { return cconcat(numeric_types); }

constexpr inline auto types(cint_t<2>) { return cconcat(numeric_vector_types<vec>); }

constexpr inline auto types(cint_t<3>) { return cconcat(numeric_types, numeric_vector_types<vec>); }

} // namespace test_catogories

template <typename T, size_t N, size_t... indices>
vec<T, N> test_enumerate(vec_shape<T, N>, csizes_t<indices...>, double start = 0, double step = 1)
{
    return make_vector<T>(static_cast<T>(start + step * indices)...);
}

template <int Cat, typename Fn, typename RefFn, typename IsApplicable = fn_return_constant<bool, true>>
void test_function1(cint_t<Cat> cat, Fn&& fn, RefFn&& reffn, IsApplicable&& isapplicable = IsApplicable{})
{
    testo::matrix(named("value") = special_values(), named("type") = test_catogories::types(cat),
                  [&](special_value value, auto type)
                  {
                      using T = typename decltype(type)::type;
                      if (isapplicable(cometa::ctype<T>, value))
                      {
                          const T x(value);
#if !defined(_MSC_VER) || defined(__clang__)
                          // Supress ICE in MSVC
                          using RefFnTy = decltype(std::declval<RefFn>()(std::declval<subtype<T>>()));
                          CHECK(std::is_same_v<decltype(fn(x)),
                                               typename compound_type_traits<T>::template rebind<RefFnTy>>);
#endif
                          const auto fn_x  = fn(x);
                          const auto ref_x = apply(reffn, x);
                          ::testo::active_test()->check(testo::deep_is_equal(ref_x, fn_x),
                                                        as_string(fn_x, " == ", ref_x),
                                                        "fn(x) == apply(reffn, x)", __FILE__, __LINE__);
                          //   CHECK(fn(x) == apply(reffn, x));
                      }
                  });

    testo::matrix(named("type") = test_catogories::types(cint<Cat & ~1>),
                  [&](auto type)
                  {
                      using T   = typename decltype(type)::type;
                      const T x = test_enumerate(T::shape(), csizeseq<T::size()>, 0);
                      CHECK(fn(x) == apply(reffn, x));
                  });
}

template <int Cat, typename Fn, typename RefFn, typename IsApplicable = fn_return_constant<bool, true>,
          typename IsDefined = fn_return_constant<bool, true>>
void test_function2(cint_t<Cat> cat, Fn&& fn, RefFn&& reffn, IsApplicable&& isapplicable = IsApplicable{},
                    IsDefined&& = IsDefined{})
{

    testo::matrix(
        named("value1") = special_values(), //
        named("value2") = special_values(), named("type") = test_catogories::types(cat),
        [&](special_value value1, special_value value2, auto type)
        {
            using T = typename decltype(type)::type;
            if constexpr (IsDefined{}(cometa::ctype<T>))
            {
                const T x1(value1);
                const T x2(value2);
                if (isapplicable(cometa::ctype<T>, value1, value2))
                {
                    CHECK(std::is_same_v<decltype(fn(x1, x2)),
                                         typename compound_type_traits<T>::template rebind<decltype(reffn(
                                             std::declval<subtype<T>>(), std::declval<subtype<T>>()))>>);
                    CHECK(fn(x1, x2) == apply(reffn, x1, x2));
                }
            }
        });

    testo::matrix(named("type") = test_catogories::types(cint<Cat & ~1>),
                  [&](auto type)
                  {
                      using T    = typename decltype(type)::type;
                      const T x1 = test_enumerate(T::shape(), csizeseq<T::size()>, 0, 1);
                      const T x2 = test_enumerate(T::shape(), csizeseq<T::size()>, 100, -1);
                      if constexpr (IsDefined{}(cometa::ctype<T>))
                      {
                          CHECK(fn(x1, x2) == apply(reffn, x1, x2));
                      }
                  });
}

#endif

namespace internal
{
// mask to mask
template <typename To, typename From, size_t N, conv_t conv>
struct conversion<1, 1, vec<bit<To>, N>, vec<bit<From>, N>, conv>
{
    static vec<bit<To>, N> cast(const vec<bit<From>, N>& value)
    {
        return vec<To, N>::frombits(broadcastto<itype<To>>(vec<itype<From>, N>::frombits(value.asvec())))
            .asmask();
    }
};

// vector to vector<vector>
template <typename To, typename From, size_t N1, size_t N2, size_t Ns1>
struct conversion<2, 1, vec<vec<To, N1>, N2>, vec<From, Ns1>, conv_t::broadcast>
{
    static_assert(N1 == Ns1, "");
    static_assert(!is_compound_type<To>, "");
    static_assert(!is_compound_type<From>, "");

    static vec<vec<To, N1>, N2> cast(const vec<From, N1>& value)
    {
        return vec<vec<To, N1>, N2>::from_flatten(
            kfr::broadcastto<To>(value.flatten())
                .shuffle(csizeseq<N2 * vec<From, N1>::scalar_size()> % csize<N2>));
    }
};

// vector to vector<vector>
template <typename To, typename From, size_t N1, size_t N2, size_t Ns1>
struct conversion<2, 1, vec<vec<To, N1>, N2>, vec<From, Ns1>, conv_t::promote>
{
    static_assert(N2 == Ns1, "");
    static_assert(!is_compound_type<To>, "");
    static_assert(!is_compound_type<From>, "");

    static vec<vec<To, N1>, N2> cast(const vec<From, N2>& value)
    {
        return vec<vec<To, N1>, N2>::from_flatten(
            kfr::broadcastto<To>(value.flatten())
                .shuffle(csizeseq<N2 * vec<From, N1>::scalar_size()> / csize<vec<From, N1>::scalar_size()> %
                         csize<N2>));
    }
};

// vector<vector> to vector<vector>
template <typename To, typename From, size_t N1, size_t N2, size_t NN1, size_t NN2, conv_t conv>
struct conversion<2, 2, vec<vec<To, N1>, N2>, vec<vec<From, NN1>, NN2>, conv>
{
    static_assert(N1 == NN1, "");
    static_assert(N2 == NN2, "");
    static_assert(!is_compound_type<To>, "");
    static_assert(!is_compound_type<From>, "");

    static vec<vec<To, N1>, N2> cast(const vec<vec<From, N1>, N2>& value)
    {
        return vec<vec<To, N1>, N2>::from_flatten(kfr::broadcastto<To>(value.flatten()));
    }
};

} // namespace internal

template <typename T, size_t N1, size_t N2 = N1>
using mat = vec<vec<T, N1>, N2>;

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

template <size_t N1, size_t N2>
struct vec_vec_template
{
    template <typename T>
    using type = vec<vec<T, N1>, N2>;
};

namespace internal
{

template <typename T, size_t... Ns>
struct vecx_t;

template <typename T>
struct vecx_t<T>
{
    using type = T;
};

template <typename T, size_t N1>
struct vecx_t<T, N1>
{
    using type = vec<T, N1>;
};

template <typename T, size_t N1, size_t N2>
struct vecx_t<T, N1, N2>
{
    using type = vec<vec<T, N1>, N2>;
};

} // namespace internal

template <typename T, size_t... Ns>
using vecx = typename internal::vecx_t<T, Ns...>::type;

template <typename T, size_t N>
constexpr inline size_t vec_rank<vec<T, N>> = 1;

template <typename T, size_t N1, size_t N2>
constexpr inline size_t vec_rank<vec<vec<T, N1>, N2>> = 2;

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> to_vec(const portable_vec<T, N>& pv)
{
    return pv;
}

} // namespace CMT_ARCH_NAME

template <typename T1, typename T2, size_t N1, size_t N2, typename = void>
struct common_type_helper
{
};
template <typename T1, typename T2, size_t N>
struct common_type_helper<T1, T2, N, N>
    : construct_common_type<std::common_type<T1, T2>, vec_template<N>::template type>
{
};
template <typename T1, typename T2, size_t N1, size_t N2>
struct common_type_helper<vec<T1, N2>, T2, N1, N2, std::enable_if_t<N1 != N2>>
    : construct_common_type<std::common_type<T1, T2>, vecvec_template<N2, N1>::template type>
{
};
template <typename T1, typename T2, size_t N1, size_t N2>
struct common_type_helper<T1, vec<T2, N2>, N1, N2, std::enable_if_t<N1 != N2>>
    : construct_common_type<std::common_type<T1, T2>, vecvec_template<N2, N1>::template type>
{
};

} // namespace kfr

namespace std
{

// V x V
template <typename T1, typename T2, size_t N1, size_t N2>
struct common_type<kfr::vec<T1, N1>, kfr::vec<T2, N2>> : kfr::common_type_helper<T1, T2, N1, N2>
{
};
// V x S
template <typename T1, typename T2, size_t N>
struct common_type<kfr::vec<T1, N>, T2>
    : kfr::construct_common_type<std::common_type<T1, T2>, kfr::vec_template<N>::template type>
{
};
// S x V
template <typename T1, typename T2, size_t N>
struct common_type<T1, kfr::vec<T2, N>>
    : kfr::construct_common_type<std::common_type<T1, T2>, kfr::vec_template<N>::template type>
{
};
#if 0
// VV x V
template <typename T1, typename T2, size_t N1, size_t N2>
struct common_type<kfr::vec<kfr::vec<T1, N1>, N2>, kfr::vec<T2, N1>>
    : kfr::construct_common_type<std::common_type<T1, T2>, kfr::vecvec_template<N1, N2>::template type>
{
};
// V x VV
template <typename T1, typename T2, size_t N1, size_t N2>
struct common_type<kfr::vec<T1, N1>, kfr::vec<kfr::vec<T2, N1>, N2>>
    : kfr::construct_common_type<std::common_type<T1, T2>, kfr::vecvec_template<N1, N2>::template type>
{
};
// VV x VV
template <typename T1, typename T2, size_t N1, size_t N2>
struct common_type<kfr::vec<kfr::vec<T1, N1>, N2>, kfr::vec<kfr::vec<T2, N1>, N2>>
    : kfr::construct_common_type<std::common_type<T1, T2>, kfr::vecvec_template<N1, N2>::template type>
{
};
#endif

} // namespace std

namespace cometa
{

template <typename T, size_t N>
struct compound_type_traits<kfr::vec_shape<T, N>>
{
    constexpr static size_t width      = N;
    constexpr static size_t deep_width = width * compound_type_traits<T>::width;
    using subtype                      = T;
    using deep_subtype                 = cometa::deep_subtype<T>;
    constexpr static bool is_scalar    = false;
    constexpr static size_t depth      = cometa::compound_type_traits<T>::depth + 1;

    template <typename U>
    using rebind = kfr::vec_shape<U, N>;
    template <typename U>
    using deep_rebind = kfr::vec_shape<typename compound_type_traits<subtype>::template deep_rebind<U>, N>;
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

    KFR_MEM_INTRINSIC static constexpr subtype at(const kfr::vec<T, N>& value, size_t index)
    {
        return value[index];
    }
};

namespace details
{
template <typename T, size_t N>
struct flt_type_impl<kfr::CMT_ARCH_NAME::vec<T, N>>
{
    using type = kfr::CMT_ARCH_NAME::vec<typename flt_type_impl<T>::type, N>;
};
} // namespace details

template <typename T, size_t N>
struct representation<kfr::CMT_ARCH_NAME::vec<T, N>>
{
    using type = std::string;
    static std::string get(const kfr::CMT_ARCH_NAME::vec<T, N>& value)
    {
        kfr::portable_vec<T, N> p = value;
        return array_to_string(N, ptr_cast<T>(&p.front()));
    }
};

template <char t, int width, int prec, typename T, size_t N>
struct representation<fmt_t<kfr::CMT_ARCH_NAME::vec<T, N>, t, width, prec>>
{
    using type = std::string;
    static std::string get(const fmt_t<kfr::CMT_ARCH_NAME::vec<T, N>, t, width, prec>& value)
    {
        kfr::portable_vec<T, N> p = value.value;
        return array_to_string<fmt_t<T, t, width, prec>>(N, ptr_cast<T>(&p.front()));
    }
};

template <typename T, size_t N>
struct representation<kfr::CMT_ARCH_NAME::mask<T, N>>
{
    using type = std::string;
    static std::string get(const kfr::CMT_ARCH_NAME::mask<T, N>& value)
    {
        bool values[N];
        for (size_t i = 0; i < N; i++)
            values[i] = value[i];
        return array_to_string(N, values);
    }
};

} // namespace cometa

CMT_PRAGMA_GNU(GCC diagnostic pop)
CMT_PRAGMA_MSVC(warning(pop))

namespace std
{

template <typename T, size_t N>
struct tuple_size<kfr::vec<T, N>> : public integral_constant<size_t, N>
{
};

template <size_t I, class T, size_t N>
struct tuple_element<I, kfr::vec<T, N>>
{
    using type = T;
};

} // namespace std
