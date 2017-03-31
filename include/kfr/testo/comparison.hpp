#pragma once

#include "../cometa/tuple.hpp"

#include "../cometa.hpp"
#include "../cometa/range.hpp"
#include "../cometa/string.hpp"
#include <cmath>

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wexit-time-destructors")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpadded")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")

namespace testo
{

using namespace cometa;

template <typename Fn, typename L, typename R>
struct comparison
{
    L left;
    R right;
    Fn cmp;

    comparison(L&& left, R&& right) : left(std::forward<L>(left)), right(std::forward<R>(right)) {}

    bool operator()() const { return cmp(left, right); }
};

template <typename Left, typename Right>
struct static_assert_type_eq
{
    static_assert(std::is_same<Left, Right>::value, "std::is_same<Left, Right>::value");
};

template <typename T, T left, T right>
struct static_assert_eq
{
    static_assert(left == right, "left == right");
};

template <typename L, typename R, typename = void>
struct equality_comparer
{
    bool operator()(const L& l, const R& r) const { return l == r; }
};

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wfloat-equal")

template <typename T>
inline T& epsilon()
{
    static T value = std::numeric_limits<T>::epsilon();
    return value;
}

template <>
struct equality_comparer<float, float>
{
    bool operator()(const float& l, const float& r) const { return !(std::abs(l - r) > epsilon<float>()); }
};
template <>
struct equality_comparer<double, double>
{
    bool operator()(const double& l, const double& r) const { return !(std::abs(l - r) > epsilon<double>()); }
};
template <>
struct equality_comparer<long double, long double>
{
    bool operator()(const long double& l, const long double& r) const
    {
        return !(std::abs(l - r) > epsilon<long double>());
    }
};

CMT_PRAGMA_GNU(GCC diagnostic pop)

template <typename L, typename R>
struct equality_comparer<L, R, void_t<enable_if<!compound_type_traits<L>::is_scalar>>>
{
    using Tsubtype = subtype<L>;
    constexpr static static_assert_type_eq<subtype<L>, subtype<R>> assert{};

    bool operator()(const L& l, const R& r) const
    {
        if (compound_type_traits<L>::width != compound_type_traits<R>::width)
            return false;

        compound_type_traits<L> itl;
        compound_type_traits<R> itr;
        for (size_t i = 0; i < compound_type_traits<L>::width; i++)
        {
            equality_comparer<Tsubtype, Tsubtype> cmp;
            if (!cmp(itl.at(l, i), itr.at(r, i)))
                return false;
        }
        return true;
    }
};

struct cmp_eq
{
    static const char* op() { return "=="; }

    template <typename L, typename R>
    bool operator()(L&& left, R&& right) const
    {
        equality_comparer<decay<L>, decay<R>> eq;
        return eq(left, right);
    }
};

struct cmp_ne
{
    static const char* op() { return "!="; }

    template <typename L, typename R>
    bool operator()(L&& left, R&& right) const
    {
        return !cmp_eq()(left, right);
    }
};

struct cmp_lt
{
    static const char* op() { return "<"; }

    template <typename L, typename R>
    bool operator()(L&& left, R&& right) const
    {
        return left < right;
    }
};

struct cmp_gt
{
    static const char* op() { return ">"; }

    template <typename L, typename R>
    bool operator()(L&& left, R&& right) const
    {
        return left > right;
    }
};

struct cmp_le
{
    static const char* op() { return "<="; }

    template <typename L, typename R>
    bool operator()(L&& left, R&& right) const
    {
        return left <= right;
    }
};

struct cmp_ge
{
    static const char* op() { return ">="; }

    template <typename L, typename R>
    bool operator()(L&& left, R&& right) const
    {
        return left >= right;
    }
};

template <typename L>
struct half_comparison
{
    half_comparison(L&& left) : left(std::forward<L>(left)) {}

    template <typename R>
    comparison<cmp_eq, L, R> operator==(R&& right)
    {
        return comparison<cmp_eq, L, R>(std::forward<L>(left), std::forward<R>(right));
    }

    template <typename R>
    comparison<cmp_ne, L, R> operator!=(R&& right)
    {
        return comparison<cmp_ne, L, R>(std::forward<L>(left), std::forward<R>(right));
    }

    template <typename R>
    comparison<cmp_lt, L, R> operator<(R&& right)
    {
        return comparison<cmp_lt, L, R>(std::forward<L>(left), std::forward<R>(right));
    }

    template <typename R>
    comparison<cmp_gt, L, R> operator>(R&& right)
    {
        return comparison<cmp_gt, L, R>(std::forward<L>(left), std::forward<R>(right));
    }

    template <typename R>
    comparison<cmp_le, L, R> operator<=(R&& right)
    {
        return comparison<cmp_le, L, R>(std::forward<L>(left), std::forward<R>(right));
    }

    template <typename R>
    comparison<cmp_ge, L, R> operator>=(R&& right)
    {
        return comparison<cmp_ge, L, R>(std::forward<L>(left), std::forward<R>(right));
    }

    L left;
};

struct make_comparison
{
    template <typename L>
    half_comparison<L> operator<=(L&& left)
    {
        return half_comparison<L>(std::forward<L>(left));
    }
};
}

CMT_PRAGMA_GNU(GCC diagnostic pop)
