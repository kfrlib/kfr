/** @addtogroup meta
 *  @{
 */
#pragma once

#include "../meta.hpp"
#include "cstring.hpp"
#include "ctti.hpp"
#include "named_arg.hpp"
#include <array>
#include <climits>
#include <cstdio>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

KFR_PRAGMA_GNU(GCC diagnostic push)
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wpragmas")
#if KFR_HAS_WARNING("-Wformat-security") || defined KFR_COMPILER_GCC
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wformat-security")
#endif
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wused-but-marked-unused")

namespace kfr
{

/**
 * @brief Primary template for converting a value of type @c T to its string representation.
 *
 * Specializations provide a @c type alias (the type actually passed to printf-style
 * formatting) and a @c get function returning the packable value.
 *
 * @tparam T Type to represent.
 */
template <typename T>
struct representation;

/**
 * @brief Alias for the type used by @ref representation<T> when formatting values of type @c T.
 */
template <typename T>
using repr_type = typename representation<T>::type;

/**
 * @brief Convert arbitrary values to a @c std::string using KFR's built-in formatting.
 *
 * @note These helpers have limited capabilities and exist only to make string
 *       processing/printing inside KFR easier. For anything non-trivial prefer a
 *       more capable formatting library.
 */
template <typename... Args>
KFR_INLINE std::string as_string(const Args&... args);

/**
 * @brief Wrapper that carries a value together with printf-style format options.
 *
 * @tparam t     Conversion specifier character (e.g. @c 'f', @c 'e', @c 'x').
 *              A value of @c -1 means "use the default specifier for the type".
 * @tparam width Minimum field width (@c -1 for none).
 * @tparam prec  Precision (@c -1 for none).
 * @tparam T     Type of the wrapped value.
 */
template <typename T, char t = static_cast<char>(-1), int width = -1, int prec = -1>
struct fmt_t
{
    const T& value;
};

namespace details
{

template <int number>
    requires(number >= 0 && number < 10)
constexpr cstring<2> itoa()
{
    return cstring<2>{ { static_cast<char>(number + '0'), 0 } };
}
template <int number>
    requires(number >= 10)
constexpr auto itoa()
{
    return concat_cstring(itoa<number / 10>(), itoa<number % 10>());
}
template <int number>
    requires(number < 0)
constexpr auto itoa()
{
    return concat_cstring(make_cstring("-"), itoa<-number>());
}

template <typename T, char t, int width, int prec>
    requires(width < 0 && prec >= 0)
KFR_INLINE constexpr auto value_fmt_arg(ctype_t<fmt_t<T, t, width, prec>>)
{
    return concat_cstring(make_cstring("."), itoa<prec>());
}
template <typename T, char t, int width, int prec>
    requires(width >= 0 && prec < 0)
KFR_INLINE constexpr auto value_fmt_arg(ctype_t<fmt_t<T, t, width, prec>>)
{
    return itoa<width>();
}
template <typename T, char t, int width, int prec>
    requires(width < 0 && prec < 0)
KFR_INLINE constexpr auto value_fmt_arg(ctype_t<fmt_t<T, t, width, prec>>)
{
    return make_cstring("");
}
template <typename T, char t, int width, int prec>
    requires(width >= 0 && prec >= 0)
KFR_INLINE constexpr auto value_fmt_arg(ctype_t<fmt_t<T, t, width, prec>>)
{
    return concat_cstring(itoa<width>(), make_cstring("."), itoa<prec>());
}

KFR_INLINE constexpr auto value_fmt(ctype_t<bool>) { return make_cstring("%s"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<std::string>) { return make_cstring("%s"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<char>) { return make_cstring("%d"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<signed char>) { return make_cstring("%d"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<unsigned char>) { return make_cstring("%d"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<short>) { return make_cstring("%d"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<unsigned short>) { return make_cstring("%d"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<int>) { return make_cstring("%d"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<long>) { return make_cstring("%ld"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<long long>) { return make_cstring("%lld"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<unsigned int>) { return make_cstring("%u"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<unsigned long>) { return make_cstring("%lu"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<unsigned long long>) { return make_cstring("%llu"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<float>) { return make_cstring("%g"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<double>) { return make_cstring("%g"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<long double>) { return make_cstring("%Lg"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<const char*>) { return make_cstring("%s"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<char*>) { return make_cstring("%s"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<void*>) { return make_cstring("%p"); }
KFR_INLINE constexpr auto value_fmt(ctype_t<const void*>) { return make_cstring("%p"); }

template <char... chars>
KFR_INLINE constexpr auto value_fmt(ctype_t<cchars_t<chars...>>)
{
    return concat_cstring(make_cstring("%s"), make_cstring(cchars_t<chars...>()));
}

template <typename T>
KFR_INLINE constexpr auto value_fmt(ctype_t<ctype_t<T>>)
{
    return make_cstring("%s");
}

template <typename T, int width, int prec>
KFR_INLINE constexpr auto value_fmt(ctype_t<fmt_t<T, static_cast<char>(-1), width, prec>> fmt)
{
    return concat_cstring(make_cstring("%"), value_fmt_arg(fmt),
                          value_fmt(ctype_t<repr_type<T>>()).slice(csize_t<1>()));
}
template <typename T, char t, int width, int prec>
KFR_INLINE constexpr auto value_fmt(ctype_t<fmt_t<T, t, width, prec>> fmt)
{
    return concat_cstring(make_cstring("%"), value_fmt_arg(fmt), cstring<2>{ { t, 0 } });
}

template <typename T>
KFR_INLINE constexpr auto value_fmt(ctype_t<T>)
{
    return make_cstring("%s");
}

template <char... chars>
KFR_INLINE const char* pack_value(const cchars_t<chars...>&)
{
    return "";
}

#define KFR_STANDARD_PACK(type)                                                                              \
    KFR_INLINE type pack_value(type value) { return value; }

KFR_STANDARD_PACK(char)
KFR_STANDARD_PACK(signed char)
KFR_STANDARD_PACK(unsigned char)
KFR_STANDARD_PACK(signed short)
KFR_STANDARD_PACK(unsigned short)
KFR_STANDARD_PACK(signed int)
KFR_STANDARD_PACK(unsigned int)
KFR_STANDARD_PACK(signed long)
KFR_STANDARD_PACK(unsigned long)
KFR_STANDARD_PACK(signed long long)
KFR_STANDARD_PACK(unsigned long long)
KFR_STANDARD_PACK(double)
KFR_STANDARD_PACK(char*)
KFR_STANDARD_PACK(const char*)
KFR_STANDARD_PACK(void*)
KFR_STANDARD_PACK(const void*)

KFR_INLINE double pack_value(float value) { return static_cast<double>(value); }
KFR_INLINE auto pack_value(bool value) { return value ? "true" : "false"; }
KFR_INLINE auto pack_value(const std::string& value) { return value.c_str(); }

template <typename T>
KFR_INLINE const char* pack_value(ctype_t<T>)
{
    return type_name<T>();
}

template <typename T, char t, int width, int prec>
KFR_INLINE auto pack_value(const fmt_t<T, t, width, prec>& value)
{
    return pack_value(representation<T>::get(value.value));
}

template <typename T>
KFR_INLINE auto pack_value(const T&)
{
    return pack_value(type_name<T>());
}

template <size_t N1, size_t Nnew, size_t... indices>
KFR_INLINE constexpr cstring<N1 - 3 + Nnew> fmt_replace_impl(const cstring<N1>& str,
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
KFR_INLINE constexpr cstring<N1 - 3 + Nto> fmt_replace(const cstring<N1>& str, const cstring<Nto>& newfmt)
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

KFR_INLINE const std::string& build_fmt(const std::string& str, ctypes_t<>) { return str; }

template <typename Arg, typename... Args>
KFR_INLINE auto build_fmt(const std::string& str, ctypes_t<Arg, Args...>)
{
    constexpr auto fmt = value_fmt(ctype_t<std::decay_t<Arg>>());
    return build_fmt(replace_one(str, "{}", std::string(fmt.data())), ctypes_t<Args...>());
}
} // namespace details

/**
 * @brief Default representation: the value is forwarded as-is to the formatting layer.
 */
template <typename T>
struct representation
{
    using type = T;
    static constexpr auto get(const T& value) noexcept { return details::pack_value(value); }
};

/**
 * @brief Wrap @p value with a custom conversion specifier.
 *
 * @tparam t     Conversion specifier character (e.g. @c 'f', @c 'e', @c 'x').
 * @tparam width Minimum field width (@c -1 for none).
 * @tparam prec  Precision (@c -1 for none).
 * @tparam T     Type of the value.
 * @param  value Value to format.
 * @return @ref fmt_t bound to @p value.
 */
template <char t, int width = -1, int prec = -1, typename T>
KFR_INLINE fmt_t<T, t, width, prec> fmt(const T& value)
{
    return { value };
}

/**
 * @brief Wrap @p value with only width/precision, keeping the default conversion specifier.
 *
 * @tparam width Minimum field width (@c -1 for none).
 * @tparam prec  Precision (@c -1 for none).
 * @tparam T     Type of the value.
 * @param  value Value to format.
 * @return @ref fmt_t bound to @p value.
 */
template <int width = -1, int prec = -1, typename T>
KFR_INLINE fmt_t<T, static_cast<char>(-1), width, prec> fmtwidth(const T& value)
{
    return { value };
}

KFR_PRAGMA_GNU(GCC diagnostic push)
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wpragmas")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wgnu-string-literal-operator-template")

/**
 * @brief Build a compile-time printf format string from a literal template and argument types.
 *
 * Each @c '@' character in the literal is replaced by the format specifier
 * corresponding to the next argument type; all other characters are copied verbatim.
 */
constexpr auto build_fmt_str(cchars_t<>, ctypes_t<>) { return make_cstring(""); }

/**
 * @brief Build a compile-time format string: replace the leading @c '@' with the
 *        specifier for @c Arg and recurse on the remaining arguments.
 */
template <char... chars, typename Arg, typename... Args>
constexpr auto build_fmt_str(cchars_t<'@', chars...>, ctypes_t<Arg, Args...>)
{
    return concat_cstring(details::value_fmt(ctype_t<std::decay_t<Arg>>()),
                          build_fmt_str(cchars_t<chars...>(), ctypes_t<Args...>()));
}

/**
 * @brief Build a compile-time format string: copy a literal character and recurse.
 */
template <char ch, char... chars, typename... Args>
constexpr auto build_fmt_str(cchars_t<ch, chars...>, ctypes_t<Args...>)
{
    return concat_cstring(make_cstring(cchars_t<ch>()),
                          build_fmt_str(cchars_t<chars...>(), ctypes_t<Args...>()));
}

/**
 * @brief Callable object holding a compile-time format literal (built by @c operator""_format).
 *
 * Calling the object formats the supplied arguments according to the literal, where
 * each @c '@' is replaced by the appropriate conversion specifier for the corresponding
 * argument type, and returns the result as a @c std::string.
 */
template <char... chars>
struct format_t
{
    /**
     * @brief Format the given arguments using the stored literal.
     * @param args Values to format.
     * @return Formatted string.
     */
    template <typename... Args>
    inline std::string operator()(const Args&... args)
    {
        constexpr auto format_str = build_fmt_str(cchars_t<chars...>(), ctypes_t<repr_type<Args>...>());

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

/**
 * @brief Callable object holding a compile-time format literal (built by @c operator""_print).
 *
 * Calling the object prints the supplied arguments to standard output according to
 * the literal, where each @c '@' is replaced by the appropriate conversion specifier
 * for the corresponding argument type.
 */
template <char... chars>
struct print_t
{
    /**
     * @brief Print the given arguments to stdout using the stored literal.
     * @param args Values to print.
     */
    template <typename... Args>
    KFR_INLINE void operator()(const Args&... args)
    {
        constexpr auto format_str = build_fmt_str(cchars_t<chars...>(), ctypes_t<repr_type<Args>...>());

        std::printf(format_str.data(), details::pack_value(args)...);
    }
};

#if defined KFR_COMPILER_GNU && !defined(KFR_COMPILER_INTEL)

/**
 * @brief User-defined literal returning a @ref format_t for the given string literal.
 *
 * Use @c '@' as a placeholder for each argument, e.g. @c "x=@, y=@\n"_format(x, y).
 */
template <typename Char, Char... chars>
constexpr format_t<chars...> operator""_format()
{
    return {};
}

/**
 * @brief User-defined literal returning a @ref print_t for the given string literal.
 *
 * Use @c '@' as a placeholder for each argument, e.g. @c "x=@, y=@\n"_print(x, y).
 */
template <typename Char, Char... chars>
constexpr KFR_INLINE print_t<chars...> operator""_print()
{
    return {};
}

#endif

KFR_PRAGMA_GNU(GCC diagnostic pop)

/**
 * @brief Print to stdout using a @c "{}"-based format string.
 *
 * Each @c "{}" placeholder is replaced at runtime by the conversion specifier of
 * the corresponding argument type.
 *
 * @param fmt  Format string with @c "{}" placeholders.
 * @param args Values to print.
 */
template <typename... Args>
KFR_INLINE void printfmt(const std::string& fmt, const Args&... args)
{
    const auto format_str = details::build_fmt(fmt, ctypes_t<repr_type<Args>...>());
    std::printf(format_str.data(), details::pack_value(representation<Args>::get(args))...);
}

/**
 * @brief Print to a file @p f using a @c "{}"-based format string.
 *
 * @param f    Output stream.
 * @param fmt  Format string with @c "{}" placeholders.
 * @param args Values to print.
 */
template <typename... Args>
KFR_INLINE void fprintfmt(FILE* f, const std::string& fmt, const Args&... args)
{
    const auto format_str = details::build_fmt(fmt, ctypes_t<repr_type<Args>...>());
    std::fprintf(f, format_str.data(), details::pack_value(representation<Args>::get(args))...);
}

/**
 * @brief Write to a buffer using a @c "{}"-based format string.
 *
 * @param str  Destination buffer.
 * @param size Size of @p str in bytes.
 * @param fmt  Format string with @c "{}" placeholders.
 * @param args Values to format.
 * @return Number of characters that would have been written (excluding the null terminator).
 */
template <typename... Args>
KFR_INLINE int snprintfmt(char* str, size_t size, const std::string& fmt, const Args&... args)
{
    const auto format_str = details::build_fmt(fmt, ctypes_t<repr_type<Args>...>());
    return std::snprintf(str, size, format_str.data(),
                         details::pack_value(representation<Args>::get(args))...);
}

/**
 * @brief Format values into a @c std::string using a @c "{}"-based format string.
 *
 * @param fmt  Format string with @c "{}" placeholders.
 * @param args Values to format.
 * @return Formatted string.
 */
template <typename... Args>
KFR_INLINE std::string format(const std::string& fmt, const Args&... args)
{
    std::string result;
    const auto format_str = details::build_fmt(fmt, ctypes_t<repr_type<Args>...>());
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
    return details::value_fmt(ctype_t<std::decay_t<repr_type<T>>>());
}
} // namespace details

/**
 * @brief Print values to stdout, each formatted with its default conversion specifier.
 *
 * @note These helpers have limited capabilities and exist only to make string
 *       processing/printing inside KFR easier. For anything non-trivial prefer a
 *       more capable formatting library.
 *
 * @param args Values to print.
 */
template <typename... Args>
KFR_INLINE void print(const Args&... args)
{
    constexpr const auto format_str = concat_cstring(details::get_value_fmt<Args>()...);
    const char* str                 = format_str.data();
    std::printf(str, details::pack_value(representation<Args>::get(args))...);
}

/**
 * @brief Print values to stdout followed by a newline.
 *
 * @note These helpers have limited capabilities and exist only to make string
 *       processing/printing inside KFR easier. For anything non-trivial prefer a
 *       more capable formatting library.
 *
 * @param args Values to print.
 */
template <typename... Args>
KFR_INLINE void println(const Args&... args)
{
    constexpr const auto format_str = concat_cstring(details::get_value_fmt<Args>()..., make_cstring("\n"));
    const char* str                 = format_str.data();
    std::printf(str, details::pack_value(representation<Args>::get(args))...);
}

/**
 * @brief Print values to stderr, each formatted with its default conversion specifier.
 *
 * @param args Values to print.
 */
template <typename... Args>
KFR_INLINE void error(const Args&... args)
{
    constexpr const auto format_str = concat_cstring(details::get_value_fmt<Args>()...);
    const char* str                 = format_str.data();
    std::fprintf(stderr, str, details::pack_value(representation<Args>::get(args))...);
}

/**
 * @brief Print values to stderr followed by a newline, then flush stderr.
 *
 * @param args Values to print.
 */
template <typename... Args>
KFR_INLINE void errorln(const Args&... args)
{
    constexpr const auto format_str = concat_cstring(details::get_value_fmt<Args>()..., make_cstring("\n"));
    const char* str                 = format_str.data();
    std::fprintf(stderr, str, details::pack_value(representation<Args>::get(args))...);
    std::fflush(stderr);
}

/**
 * @brief Convert values to a @c std::string, each formatted with its default conversion specifier.
 *
 * @note These helpers have limited capabilities and exist only to make string
 *       processing/printing inside KFR easier. For anything non-trivial prefer a
 *       more capable formatting library.
 *
 * @param args Values to convert.
 * @return String representation of the concatenated values.
 */
template <typename... Args>
KFR_INLINE std::string as_string(const Args&... args)
{
    std::string result;
    constexpr auto format_str = concat_cstring(details::get_value_fmt<Args>()...);
    const char* str           = format_str.data();

    const int size = std::snprintf(nullptr, 0, str, details::pack_value(representation<Args>::get(args))...);
    if (size <= 0)
        return result;
    result.resize(size_t(size + 1));
    result.resize(size_t(std::snprintf(&result[0], size_t(size + 1), str,
                                       details::pack_value(representation<Args>::get(args))...)));
    return result;
}

/**
 * @brief Right-align @p text within a field of @p size characters by prepending @p character.
 *
 * @param size      Total field width.
 * @param text      Text to pad.
 * @param character Padding character (default space).
 * @return Padded string.
 */
inline std::string padright(size_t size, const std::string& text, char character = ' ')
{
    const size_t pad = size >= text.size() ? size - text.size() : 0;
    return std::string(pad, character) + text;
}

/**
 * @brief Left-align @p text within a field of @p size characters by appending @p character.
 *
 * @param size      Total field width.
 * @param text      Text to pad.
 * @param character Padding character (default space).
 * @return Padded string.
 */
inline std::string padleft(size_t size, const std::string& text, char character = ' ')
{
    const size_t pad = size >= text.size() ? size - text.size() : 0;
    return text + std::string(pad, character);
}

/**
 * @brief Center @p text within a field of @p size characters using @p character.
 *
 * When the padding cannot be split evenly the extra character is placed on the right.
 *
 * @param size      Total field width.
 * @param text      Text to pad.
 * @param character Padding character (default space).
 * @return Padded string.
 */
inline std::string padcenter(size_t size, const std::string& text, char character = ' ')
{
    const size_t pad = size >= text.size() ? size - text.size() : 0;
    return std::string(pad / 2, character) + text + std::string(pad - pad / 2, character);
}

/**
 * @brief Wrap the string representation of @p x in double quotes.
 *
 * @tparam T Type of the value.
 * @param  x  Value to quote.
 * @return @c "\"" + as_string(x) + "\"".
 */
template <typename T>
inline std::string q(T x)
{
    return "\"" + as_string(std::forward<T>(x)) + "\"";
}

/**
 * @brief Convert a single value to a string (terminator of @ref join).
 *
 * @tparam T Type of the value.
 * @param  x  Value to convert.
 * @return String representation of @p x.
 */
template <typename T>
inline std::string join(T x)
{
    return as_string(std::forward<T>(x));
}

/**
 * @brief Join values into a comma-separated string of the form @c "{x}, {y}, {rest...}".
 *
 * @tparam T  Type of the first value.
 * @tparam U  Type of the second value.
 * @tparam Ts Types of the remaining values.
 * @param  x    First value.
 * @param  y    Second value.
 * @param  rest Remaining values.
 * @return Comma-separated string.
 */
template <typename T, typename U, typename... Ts>
inline std::string join(T x, U y, Ts... rest)
{
    return format("{}, {}", x, join(std::forward<U>(y), std::forward<Ts>(rest)...));
}

/**
 * @brief Representation of @ref named_arg as @c "name = value".
 */
template <typename T>
struct representation<named_arg<T>>
{
    using type = std::string;
    static std::string get(const named_arg<T>& value)
    {
        return std::string(value.name) + " = " + as_string(value.value);
    }
};

/**
 * @brief Representation of @c std::pair as @c "(first; second)".
 */
template <typename T1, typename T2>
struct representation<std::pair<T1, T2>>
{
    using type = std::string;
    static std::string get(const std::pair<T1, T2>& value)
    {
        return "(" + as_string(value.first) + "; " + as_string(value.second) + ")";
    }
};

/**
 * @brief Representation of @c std::unique_ptr as @c "Type(value)" or @c "Type(nullptr)".
 */
template <typename T1>
struct representation<std::unique_ptr<T1>>
{
    using type = std::string;
    static std::string get(const std::unique_ptr<T1>& value)
    {
        if (value)
            return as_string(type_name<std::unique_ptr<T1>>(), "(", *value.get(), ")");
        else
            return as_string(type_name<std::unique_ptr<T1>>(), "(nullptr)");
    }
};

/**
 * @brief Representation of @c std::weak_ptr as @c "Type(value)" or @c "Type(nullptr)".
 */
template <typename T1>
struct representation<std::weak_ptr<T1>>
{
    using type = std::string;
    static std::string get(const std::weak_ptr<T1>& value)
    {
        std::shared_ptr<T1> sh = value.lock();
        if (sh)
            return as_string(type_name<std::weak_ptr<T1>>(), "(", *sh.get(), ")");
        else
            return as_string(type_name<std::weak_ptr<T1>>(), "(nullptr)");
    }
};

/**
 * @brief Representation of @c std::shared_ptr as @c "Type(value)" or @c "Type(nullptr)".
 */
template <typename T1>
struct representation<std::shared_ptr<T1>>
{
    using type = std::string;
    static std::string get(const std::shared_ptr<T1>& value)
    {
        if (value)
            return as_string(type_name<std::shared_ptr<T1>>(), "(", *value.get(), ")");
        else
            return as_string(type_name<std::shared_ptr<T1>>(), "(nullptr)");
    }
};

/**
 * @brief Representation of @c std::shared_ptr<void> as @c "Type(pointer)" or @c "Type(nullptr)".
 */
template <>
struct representation<std::shared_ptr<void>>
{
    using type = std::string;
    static std::string get(const std::shared_ptr<void>& value)
    {
        if (value)
            return as_string(type_name<std::shared_ptr<void>>(), "(", value.get(), ")");
        else
            return as_string(type_name<std::shared_ptr<void>>(), "(nullptr)");
    }
};

namespace details
{

template <size_t dims>
KFR_INTRINSIC size_t trailing_zeros(const std::array<size_t, dims>& indices)
{
    for (size_t i = 0; i < dims; ++i)
    {
        if (indices[dims - 1 - i] != 0)
            return i;
    }
    return dims;
}

template <size_t dims>
KFR_INTRINSIC bool increment_indices(std::array<size_t, dims>& indices, const std::array<size_t, dims>& stop)
{
    indices[dims - 1] += 1;
    KFR_PRAGMA_GNU(clang diagnostic push)
#if KFR_HAS_WARNING("-Wpass-failed")
    KFR_PRAGMA_GNU(clang diagnostic ignored "-Wpass-failed")
#endif
    KFR_LOOP_UNROLL
    for (int i = dims - 1; i >= 0;)
    {
        if (KFR_LIKELY(indices[i] < stop[i]))
            return true;
        // carry
        indices[i] = 0;
        --i;
        if (i < 0)
        {
            return false;
        }
        indices[i] += 1;
    }
    KFR_PRAGMA_GNU(clang diagnostic pop)
    return true;
}
} // namespace details

/**
 * @brief Wrap @p val with the format type @c Fmt when @c Fmt is not @c void.
 */
template <typename U, typename Fmt>
KFR_INTRINSIC Fmt wrap_fmt(const U& val, ctype_t<Fmt>)
{
    return Fmt{ val };
}
/**
 * @brief Identity pass-through used when no format type is specified.
 */
template <typename U>
KFR_INTRINSIC U wrap_fmt(const U& val, ctype_t<void>)
{
    return val;
}

/**
 * @brief Render a multi-dimensional array as a nested, brace-delimited string.
 *
 * The array shape is given by @p shape and each element is obtained by invoking
 * @p getter with a @c std::array<size_t, Dims> index. Newlines are inserted when
 * an inner dimension is exhausted or @p max_columns is reached.
 *
 * @tparam Fmt    Optional @ref fmt_t type used to format each element (@c void = default).
 * @tparam Dims   Number of dimensions.
 * @tparam Getter Callable returning the element at a given index.
 * @param shape          Extent of each dimension.
 * @param getter         Function returning the element for a multi-index.
 * @param max_columns    Maximum number of values printed per line (0 = unlimited).
 * @param max_dimensions Controls line breaks between inner dimensions.
 * @param separator      Separator between adjacent values.
 * @param open           String used to open a dimension.
 * @param close          String used to close a dimension.
 * @return Formatted string.
 */
template <typename Fmt = void, size_t Dims, typename Getter>
std::string array_to_string(const std::array<size_t, Dims>& shape, Getter&& getter, int max_columns = 16,
                            int max_dimensions = INT_MAX, std::string_view separator = ", ",
                            std::string_view open = "{", std::string_view close = "}")
{
    using shape_type = std::array<size_t, Dims>;
    using index_t    = size_t;

    if (max_columns == 0)
        max_columns = INT_MAX;
    std::string ss;
    for (index_t i = 0; i < Dims; ++i)
        ss += open;

    bool isempty = std::accumulate(shape.begin(), shape.end(), size_t(1), std::multiplies<size_t>{}) == 0;
    if (!isempty)
    {
        shape_type index{ 0 };
        std::string open_filler(open.size(), ' ');
        std::string_view separator_trimmed = separator.substr(0, 1 + separator.find_last_not_of(" \t"));
        int columns                        = 0;
        do
        {
            std::string str = as_string(wrap_fmt(getter(index), kfr::ctype<Fmt>));
            index_t z       = details::trailing_zeros(index);
            if ((z > 0 && columns > 0) || columns >= max_columns)
            {
                for (index_t i = 0; i < z; ++i)
                    ss += close;

                if (z > max_dimensions || columns >= max_columns)
                {
                    if (columns > 0)
                        ss += separator_trimmed;
                    ss += "\n";
                    for (index_t i = 0; i < Dims - z; ++i)
                        ss += open_filler;
                }
                else
                {
                    if (columns > 0)
                        ss += separator;
                }
                for (index_t i = 0; i < z; ++i)
                    ss += open;

                columns = 0;
            }
            else
            {
                if (columns > 0)
                    ss += separator;
            }
            ss += str;
            ++columns;
        } while (details::increment_indices<Dims>(index, shape));
    }
    for (index_t i = 0; i < Dims; ++i)
        ss += close;
    return ss;
}

/**
 * @brief Convenience overload of @ref array_to_string for a one-dimensional array
 *        accessed through a getter callable.
 *
 * @tparam Fmt    Optional @ref fmt_t type used to format each element (@c void = default).
 * @tparam Getter Callable returning the element at a given linear index.
 * @param size           Number of elements.
 * @param getter         Function returning the element for an index.
 * @param max_columns    Maximum number of values printed per line (0 = unlimited).
 * @param max_dimensions Controls line breaks.
 * @param separator      Separator between adjacent values.
 * @param open           String used to open the array.
 * @param close          String used to close the array.
 * @return Formatted string.
 */
template <typename Fmt = void, typename Getter>
std::string array_to_string(size_t size, Getter&& getter, int max_columns = 16, int max_dimensions = INT_MAX,
                            std::string_view separator = ", ", std::string_view open = "{",
                            std::string_view close = "}")
{
    return array_to_string<Fmt>(std::array<size_t, 1>{ size }, std::forward<Getter>(getter), max_columns,
                                max_dimensions, std::move(separator), std::move(open), std::move(close));
}
/**
 * @brief Convenience overload of @ref array_to_string for a raw pointer buffer.
 *
 * @tparam Fmt Optional @ref fmt_t type used to format each element (@c void = default).
 * @tparam T   Element type.
 * @param size           Number of elements.
 * @param data           Pointer to the first element.
 * @param max_columns    Maximum number of values printed per line (0 = unlimited).
 * @param max_dimensions Controls line breaks.
 * @param separator      Separator between adjacent values.
 * @param open           String used to open the array.
 * @param close          String used to close the array.
 * @return Formatted string.
 */
template <typename Fmt = void, typename T>
std::string array_to_string(size_t size, T* data, int max_columns = 16, int max_dimensions = INT_MAX,
                            std::string_view separator = ", ", std::string_view open = "{",
                            std::string_view close = "}")
{
    return array_to_string<Fmt>(
        std::array<size_t, 1>{ size }, [data](std::array<size_t, 1> i) { return data[i.front()]; },
        max_columns, max_dimensions, std::move(separator), std::move(open), std::move(close));
}

/**
 * @brief Representation of @c std::array as a brace-delimited list of its elements.
 */
template <typename T, size_t Size>
struct representation<std::array<T, Size>>
{
    using type = std::string;
    static std::string get(const std::array<T, Size>& value)
    {
        return array_to_string(value.size(), value.data());
    }
};
/**
 * @brief Representation of @c std::vector as a brace-delimited list of its elements.
 */
template <typename T, typename Allocator>
struct representation<std::vector<T, Allocator>>
{
    using type = std::string;
    static std::string get(const std::vector<T, Allocator>& value)
    {
        return array_to_string(value.size(), value.data());
    }
};
/**
 * @brief Representation of @c std::string_view as a @c std::string.
 */
template <>
struct representation<std::string_view>
{
    using type = std::string;
    static std::string get(const std::string_view& value) { return std::string(value); }
};

} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)
