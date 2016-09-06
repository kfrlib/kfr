/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"
#include "cstring.hpp"
#include "ctti.hpp"
#include "named_arg.hpp"
#include <array>
#include <cstdio>
#include <string>
#include <utility>

#pragma GCC diagnostic push
#if CMT_HAS_WARNING("-Wformat-security")
#pragma GCC diagnostic ignored "-Wformat-security"
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif

namespace cometa
{

template <typename T>
struct representation
{
    static constexpr const T& get(const T& value) noexcept { return value; }
};

template <typename T>
using repr_type = decay<decltype(representation<T>::get(std::declval<T>()))>;

template <typename... Args>
CMT_INLINE std::string as_string(const Args&... args);

namespace details
{
template <typename T, char t = static_cast<char>(-1), int width = -1, int prec = -1>
struct fmt_t
{
    const T& value;
};

template <int number, CMT_ENABLE_IF(number >= 0 && number < 10)>
constexpr cstring<2> itoa()
{
    return cstring<2>{ { static_cast<char>(number + '0'), 0 } };
}
template <int number, CMT_ENABLE_IF(number >= 10)>
constexpr auto itoa()
{
    return concat_cstring(itoa<number / 10>(), itoa<number % 10>());
}
template <int number, CMT_ENABLE_IF(number < 0)>
constexpr auto itoa()
{
    return concat_cstring(make_cstring("-"), itoa<-number>());
}

template <typename T, char t, int width, int prec, CMT_ENABLE_IF(width < 0 && prec >= 0)>
CMT_INLINE constexpr auto value_fmt_arg(ctype_t<fmt_t<T, t, width, prec>>)
{
    return concat_cstring(make_cstring("."), itoa<prec>());
}
template <typename T, char t, int width, int prec, CMT_ENABLE_IF(width >= 0 && prec < 0)>
CMT_INLINE constexpr auto value_fmt_arg(ctype_t<fmt_t<T, t, width, prec>>)
{
    return itoa<width>();
}
template <typename T, char t, int width, int prec, CMT_ENABLE_IF(width < 0 && prec < 0)>
CMT_INLINE constexpr auto value_fmt_arg(ctype_t<fmt_t<T, t, width, prec>>)
{
    return make_cstring("");
}
template <typename T, char t, int width, int prec, CMT_ENABLE_IF(width >= 0 && prec >= 0)>
CMT_INLINE constexpr auto value_fmt_arg(ctype_t<fmt_t<T, t, width, prec>>)
{
    return concat_cstring(itoa<width>(), make_cstring("."), itoa<prec>());
}

CMT_INLINE constexpr auto value_fmt(ctype_t<bool>) { return make_cstring("%s"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<std::string>) { return make_cstring("%s"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<char>) { return make_cstring("%d"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<signed char>) { return make_cstring("%d"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<unsigned char>) { return make_cstring("%d"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<short>) { return make_cstring("%d"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<unsigned short>) { return make_cstring("%d"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<int>) { return make_cstring("%d"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<long>) { return make_cstring("%ld"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<long long>) { return make_cstring("%lld"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<unsigned int>) { return make_cstring("%u"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<unsigned long>) { return make_cstring("%lu"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<unsigned long long>) { return make_cstring("%llu"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<float>) { return make_cstring("%g"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<double>) { return make_cstring("%g"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<long double>) { return make_cstring("%Lg"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<const char*>) { return make_cstring("%s"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<char*>) { return make_cstring("%s"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<void*>) { return make_cstring("%p"); }
CMT_INLINE constexpr auto value_fmt(ctype_t<const void*>) { return make_cstring("%p"); }

template <char... chars>
CMT_INLINE constexpr auto value_fmt(ctype_t<cchars_t<chars...>>)
{
    return concat_cstring(make_cstring("%s"), make_cstring(cchars<chars...>));
}

template <typename T>
CMT_INLINE constexpr auto value_fmt(ctype_t<ctype_t<T>>)
{
    return make_cstring("%s");
}

template <typename T, int width, int prec>
CMT_INLINE constexpr auto value_fmt(ctype_t<fmt_t<T, static_cast<char>(-1), width, prec>> fmt)
{
    return concat_cstring(make_cstring("%"), value_fmt_arg(fmt),
                          value_fmt(ctype<repr_type<T>>).slice(csize<1>));
}
template <typename T, char t, int width, int prec>
CMT_INLINE constexpr auto value_fmt(ctype_t<fmt_t<T, t, width, prec>> fmt)
{
    return concat_cstring(make_cstring("%"), value_fmt_arg(fmt), cstring<2>{ { t, 0 } });
}

template <char... chars>
CMT_INLINE const char* pack_value(const cchars_t<chars...>&)
{
    return "";
}

template <typename Arg>
CMT_INLINE const Arg& pack_value(const Arg& value)
{
    return value;
}
CMT_INLINE double pack_value(float value) { return static_cast<double>(value); }
CMT_INLINE auto pack_value(bool value) { return value ? "true" : "false"; }
CMT_INLINE auto pack_value(const std::string& value) { return value.c_str(); }

template <typename T>
CMT_INLINE const char* pack_value(ctype_t<T>)
{
    return type_name<T>();
}

template <typename T, char t, int width, int prec>
CMT_INLINE auto pack_value(const fmt_t<T, t, width, prec>& value)
{
    return pack_value(representation<T>::get(value.value));
}

template <size_t N1, size_t Nnew, size_t... indices>
CMT_INLINE constexpr cstring<N1 - 3 + Nnew> fmt_replace_impl(const cstring<N1>& str,
                                                             const cstring<Nnew>& newfmt,
                                                             csizes_t<indices...>)
{
    size_t start = 0;
    size_t end   = 0;
    cstring<N1 - 3 + Nnew> result;
    for (size_t i = 0; i < N1; i++)
    {
        if (str[i] == '{')
            start = i;
        else if (str[i] == '}')
            end = i;
    }

    if (end - start == 1) // {}
    {
        for (size_t i = 0; i < N1; i++)
        {
            if (i < start)
                result[i] = str[i];
            else if (i == start)
                result[i] = '%';
            else if (i > start && i - start - 1 < Nnew - 1)
                result[i] = newfmt[i - start - 1];
            else if (i - Nnew + 3 < N1 - 1)
                result[i] = str[i - Nnew + 3];
            else
                result[i] = 0;
        }
    }
    return result;
}

template <size_t N1, size_t Nto>
CMT_INLINE constexpr cstring<N1 - 3 + Nto> fmt_replace(const cstring<N1>& str, const cstring<Nto>& newfmt)
{
    return fmt_replace_impl(str, newfmt, csizeseq<N1 - 3 + Nto - 1>);
}

inline std::string replace_one(const std::string& str, const std::string& from, const std::string& to)
{
    std::string r    = str;
    size_t start_pos = 0;
    if ((start_pos = r.find(from, start_pos)) != std::string::npos)
    {
        r.replace(start_pos, from.size(), to);
    }
    return r;
}

CMT_INLINE const std::string& build_fmt(const std::string& str, ctypes_t<>) { return str; }

template <typename Arg, typename... Args>
CMT_INLINE auto build_fmt(const std::string& str, ctypes_t<Arg, Args...>)
{
    constexpr auto fmt = value_fmt(ctype<decay<Arg>>);
    return build_fmt(replace_one(str, "{}", std::string(fmt.data())), ctypes<Args...>);
}
}

template <char t, int width = -1, int prec = -1, typename T>
CMT_INLINE details::fmt_t<T, t, width, prec> fmt(const T& value)
{
    return { value };
}

template <int width = -1, int prec = -1, typename T>
CMT_INLINE details::fmt_t<T, static_cast<char>(-1), width, prec> fmtwidth(const T& value)
{
    return { value };
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-string-literal-operator-template"

constexpr auto build_fmt_str(cchars_t<>, ctypes_t<>) { return make_cstring(""); }

template <char... chars, typename Arg, typename... Args>
constexpr auto build_fmt_str(cchars_t<'@', chars...>, ctypes_t<Arg, Args...>)
{
    return concat_cstring(details::value_fmt(ctype<decay<Arg>>),
                          build_fmt_str(cchars<chars...>, ctypes<Args...>));
}

template <char ch, char... chars, typename... Args>
constexpr auto build_fmt_str(cchars_t<ch, chars...>, ctypes_t<Args...>)
{
    return concat_cstring(make_cstring(cchars<ch>), build_fmt_str(cchars<chars...>, ctypes<Args...>));
}

template <char... chars>
struct format_t
{
    template <typename... Args>
    inline std::string operator()(const Args&... args)
    {
        constexpr auto format_str = build_fmt_str(cchars<chars...>, ctypes<repr_type<Args>...>);

        std::string result;
        const int size = std::snprintf(nullptr, 0, format_str.data(), details::pack_value(args)...);
        if (size <= 0)
            return result;
        result.resize(size_t(size + 1));
        result.resize(size_t(std::snprintf(&result[0], size_t(size + 1), format_str.data(),
                                           details::pack_value(representation<Args>::get(args))...)));
        return result;
    }
};

template <char... chars>
struct print_t
{
    template <typename... Args>
    CMT_INLINE void operator()(const Args&... args)
    {
        constexpr auto format_str = build_fmt_str(cchars<chars...>, ctypes<repr_type<Args>...>);

        std::printf(format_str.data(), details::pack_value(args)...);
    }
};

template <typename Char, Char... chars>
constexpr format_t<chars...> operator""_format()
{
    return {};
}

template <typename Char, Char... chars>
constexpr CMT_INLINE print_t<chars...> operator""_print()
{
    return {};
}

#pragma GCC diagnostic pop

template <typename... Args>
CMT_INLINE void printfmt(const std::string& fmt, const Args&... args)
{
    const auto format_str = details::build_fmt(fmt, ctypes<repr_type<Args>...>);
    std::printf(format_str.data(), details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE void fprintfmt(FILE* f, const std::string& fmt, const Args&... args)
{
    const auto format_str = details::build_fmt(fmt, ctypes<repr_type<Args>...>);
    std::fprintf(f, format_str.data(), details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE int snprintfmt(char* str, size_t size, const std::string& fmt, const Args&... args)
{
    const auto format_str = details::build_fmt(fmt, ctypes<repr_type<Args>...>);
    return std::snprintf(str, size, format_str.data(),
                         details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE std::string format(const std::string& fmt, const Args&... args)
{
    std::string result;
    const auto format_str = details::build_fmt(fmt, ctypes<repr_type<Args>...>);
    const int size =
        std::snprintf(nullptr, 0, format_str.data(), details::pack_value(representation<Args>::get(args))...);
    if (size <= 0)
        return result;
    result.resize(size_t(size + 1));
    result.resize(size_t(std::snprintf(&result[0], size_t(size + 1), format_str.data(),
                                       details::pack_value(representation<Args>::get(args))...)));
    return result;
}

namespace details
{
template <typename T>
constexpr auto get_value_fmt()
{
    return details::value_fmt(ctype<decay<repr_type<T>>>);
}
}

template <typename... Args>
CMT_INLINE void print(const Args&... args)
{
    constexpr auto format_str = concat_cstring(details::get_value_fmt<Args>()...);
    std::printf(format_str.data(), details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE void println(const Args&... args)
{
    constexpr auto format_str = concat_cstring(details::get_value_fmt<Args>()..., make_cstring("\n"));
    std::printf(format_str.data(), details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE std::string as_string(const Args&... args)
{
    std::string result;
    constexpr auto format_str = concat_cstring(details::get_value_fmt<Args>()...);

    const int size =
        std::snprintf(nullptr, 0, format_str.data(), details::pack_value(representation<Args>::get(args))...);
    if (size <= 0)
        return result;
    result.resize(size_t(size + 1));
    result.resize(size_t(std::snprintf(&result[0], size_t(size + 1), format_str.data(),
                                       details::pack_value(representation<Args>::get(args))...)));
    return result;
}

inline std::string padright(size_t size, const std::string& text, char character = ' ')
{
    const size_t pad = size >= text.size() ? size - text.size() : 0;
    return std::string(pad, character) + text;
}

inline std::string padleft(size_t size, const std::string& text, char character = ' ')
{
    const size_t pad = size >= text.size() ? size - text.size() : 0;
    return text + std::string(pad, character);
}

inline std::string padcenter(size_t size, const std::string& text, char character = ' ')
{
    const size_t pad = size >= text.size() ? size - text.size() : 0;
    return std::string(pad / 2, character) + text + std::string(pad - pad / 2, character);
}

template <typename T>
inline std::string q(T x)
{
    return "\"" + as_string(std::forward<T>(x)) + "\"";
}

template <typename T>
inline std::string join(T x)
{
    return as_string(std::forward<T>(x));
}

template <typename T, typename U, typename... Ts>
inline std::string join(T x, U y, Ts... rest)
{
    return format("{}, {}", x, join(std::forward<U>(y), std::forward<Ts>(rest)...));
}

template <typename T>
struct representation<named_arg<T>>
{
    static std::string get(const named_arg<T>& value)
    {
        return std::string(value.name) + " = " + as_string(value.value);
    }
};

template <typename T1, typename T2>
struct representation<std::pair<T1, T2>>
{
    static std::string get(const std::pair<T1, T2>& value)
    {
        return "(" + as_string(value.first) + "; " + as_string(value.second) + ")";
    }
};
}

#pragma GCC diagnostic pop
