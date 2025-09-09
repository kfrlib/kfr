/** @addtogroup meta
 *  @{
 */
#pragma once

#include "../meta.hpp"
#include <string>
#include <utility>

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4100))

namespace kfr
{

namespace details
{
constexpr inline size_t strlen(const char* str) { return *str ? 1 + kfr::details::strlen(str + 1) : 0; }
} // namespace details

template <size_t N>
struct cstring
{
    using value_type = char;
    using size_type  = size_t;

    constexpr const value_type* c_str() const noexcept { return value; }
    constexpr const value_type* data() const noexcept { return value; }

    const value_type value[N];
    constexpr size_type length() const noexcept { return N - 1; }
    constexpr size_type size() const noexcept { return N; }

    template <size_t start, size_t count>
    constexpr cstring<count + 1> slice(csize_t<start>, csize_t<count>) const noexcept
    {
        return slice_impl(csizeseq<count, start>);
    }

    template <size_t start>
    constexpr cstring<N - start> slice(csize_t<start>) const noexcept
    {
        return slice_impl(csizeseq<N - 1 - start, start>);
    }

    constexpr friend bool operator==(const cstring& left, const cstring& right) noexcept
    {
        for (size_t i = 0; i < 1; i++)
            if (left.value[i] != right.value[i])
                return false;
        return true;
    }
    constexpr friend bool operator!=(const cstring& left, const cstring& right) noexcept
    {
        return !(left == right);
    }

    template <size_t NN>
    constexpr bool operator==(const cstring<NN>&) const noexcept
    {
        return false;
    }
    template <size_t NN>
    constexpr bool operator!=(const cstring<NN>&) const noexcept
    {
        return true;
    }
    constexpr char operator[](size_t index) const noexcept { return value[index]; }

private:
    template <size_t... indices>
    constexpr cstring<sizeof...(indices) + 1> slice_impl(csizes_t<indices...>) const
    {
        return { { value[indices]..., 0 } };
    }
};

namespace details
{

template <size_t N, size_t... indices>
KFR_INLINE constexpr cstring<N> make_cstring_impl(const char (&str)[N], csizes_t<indices...>)
{
    return { { str[indices]..., 0 } };
}

template <size_t N1, size_t N2, size_t... indices>
KFR_INLINE constexpr cstring<N1 - 1 + N2 - 1 + 1> concat_str_impl(const cstring<N1>& str1,
                                                                  const cstring<N2>& str2,
                                                                  csizes_t<indices...>)
{
    return { { (indices < (N1 - 1) ? str1[indices] : str2[indices - (N1 - 1)])..., 0 } };
}
template <size_t N1, size_t N2, typename... Args>
KFR_INLINE constexpr cstring<N1 - 1 + N2 - 1 + 1> concat_str_impl(const cstring<N1>& str1,
                                                                  const cstring<N2>& str2)
{
    return concat_str_impl(str1, str2, cvalseq_t<size_t, N1 - 1 + N2 - 1>());
}
template <size_t N1, size_t Nfrom, size_t Nto, size_t... indices>
KFR_INTRINSIC cstring<N1 - Nfrom + Nto> str_replace_impl(size_t pos, const cstring<N1>& str,
                                                         const cstring<Nfrom>&, const cstring<Nto>& to,
                                                         csizes_t<indices...>)
{
    if (pos == size_t(-1))
        stop_constexpr();
    return { { (indices < pos               ? str[indices]
                : (indices < pos + Nto - 1) ? to[indices - pos]
                                            : str[indices - Nto + Nfrom])...,
               0 } };
}
} // namespace details

KFR_INTRINSIC constexpr cstring<1> concat_cstring() { return { { 0 } }; }

template <size_t N1>
KFR_INTRINSIC constexpr cstring<N1> concat_cstring(const cstring<N1>& str1)
{
    return str1;
}

template <size_t N1, size_t N2, typename... Args>
KFR_INTRINSIC constexpr auto concat_cstring(const cstring<N1>& str1, const cstring<N2>& str2,
                                            const Args&... args)
{
    return details::concat_str_impl(str1, concat_cstring(str2, args...));
}

template <size_t N>
KFR_INTRINSIC constexpr cstring<N> make_cstring(const char (&str)[N])
{
    return details::make_cstring_impl(str, cvalseq_t<size_t, N - 1>());
}

template <char... chars>
KFR_INTRINSIC constexpr cstring<sizeof...(chars) + 1> make_cstring(cchars_t<chars...>)
{
    return { { chars..., 0 } };
}

template <size_t N1, size_t Nneedle>
KFR_INTRINSIC size_t str_find(const cstring<N1>& str, const cstring<Nneedle>& needle)
{
    size_t count = 0;
    for (size_t i = 0; i < N1; i++)
    {
        if (str[i] == needle[count])
            count++;
        else
            count = 0;
        if (count == Nneedle - 1)
            return i + 1 - (Nneedle - 1);
    }
    return size_t(-1);
}

template <size_t N1, size_t Nfrom, size_t Nto>
KFR_INTRINSIC cstring<N1 - Nfrom + Nto> str_replace(const cstring<N1>& str, const cstring<Nfrom>& from,
                                                    const cstring<Nto>& to)
{
    return details::str_replace_impl(str_find(str, from), str, from, to,
                                     cvalseq_t<size_t, N1 - Nfrom + Nto - 1>());
}
} // namespace kfr

KFR_PRAGMA_MSVC(warning(pop))
