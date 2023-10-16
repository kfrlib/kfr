/** @addtogroup testo
 *  @{
 */
#pragma once

#include "../cometa/tuple.hpp"

#include "../cometa.hpp"
#include "../cometa/range.hpp"
#include "../cometa/string.hpp"
#include <cmath>

CMT_PRAGMA_GNU(GCC diagnostic push)
#if CMT_HAS_WARNING("-Wexit-time-destructors")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wexit-time-destructors")
#endif
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpadded")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4018))

namespace testo
{

using namespace cometa;

template <typename Fn, typename L, typename R>
struct comparison
{
    L left;
    R right;
    Fn cmp;

    comparison(L&& left, R&& right) : left(std::forward<L>(left)), right(std::forward<R>(right)), cmp() {}

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
inline T& current_epsilon()
{
    static T value = std::numeric_limits<T>::epsilon();
    return value;
}

template <typename T = void>
struct epsilon_scope
{
    static_assert(std::is_floating_point_v<T>);
    epsilon_scope(T scale) { current_epsilon<T>() = std::numeric_limits<T>::epsilon() * scale; }
    ~epsilon_scope() { current_epsilon<T>() = saved; }
    T saved = current_epsilon<T>();
};

template <>
struct epsilon_scope<void>
{
    epsilon_scope(float scale) : f(scale), d(static_cast<double>(scale)), ld(static_cast<long double>(scale))
    {
    }
    epsilon_scope<float> f;
    epsilon_scope<double> d;
    epsilon_scope<long double> ld;
};

CMT_PRAGMA_GNU(GCC diagnostic pop)

template <typename T1, typename T2,
          CMT_ENABLE_IF(compound_type_traits<T1>::is_scalar&& compound_type_traits<T2>::is_scalar &&
                        (std::is_floating_point<T1>::value || std::is_floating_point<T2>::value))>
constexpr bool deep_is_equal(const T1& x, const T2& y)
{
    using C    = std::common_type_t<T1, T2>;
    const C xx = static_cast<C>(x);
    const C yy = static_cast<C>(y);
    if (std::isnan(xx) && std::isnan(yy))
        return true;
    if (std::isnan(xx) || std::isnan(yy))
        return false;

    return !(std::abs(xx - yy) > current_epsilon<C>());
}

template <typename T1, typename T2,
          CMT_ENABLE_IF(compound_type_traits<T1>::is_scalar&& compound_type_traits<T2>::is_scalar &&
                        !std::is_floating_point<T1>::value && !std::is_floating_point<T2>::value)>
constexpr bool deep_is_equal(const T1& x, const T2& y)
{
    return x == y;
}

template <typename T1, typename T2,
          CMT_ENABLE_IF(!compound_type_traits<T1>::is_scalar || !compound_type_traits<T2>::is_scalar)>
constexpr bool deep_is_equal(const T1& x, const T2& y)
{
    static_assert(compound_type_traits<T1>::width == compound_type_traits<T2>::width ||
                      compound_type_traits<T1>::is_scalar || compound_type_traits<T2>::is_scalar,
                  "");
    for (size_t i = 0; i < std::max(+compound_type_traits<T1>::width, +compound_type_traits<T2>::width); i++)
    {
        if (!deep_is_equal(compound_type_traits<T1>::at(x, i), compound_type_traits<T2>::at(y, i)))
            return false;
    }
    return true;
}

struct cmp_eq
{
    static const char* op() { return "=="; }

    template <typename L, typename R>
    bool operator()(L&& left, R&& right) const
    {
        return deep_is_equal(left, right);
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
} // namespace testo

CMT_PRAGMA_MSVC(warning(pop))
CMT_PRAGMA_GNU(GCC diagnostic pop)
