/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"
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

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpragmas")
#if CMT_HAS_WARNING("-Wformat-security") || defined CMT_COMPILER_GCC
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wformat-security")
#endif
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wused-but-marked-unused")

namespace cometa
{

template <typename T>
struct representation;

template <typename T>
using repr_type = typename representation<T>::type;

template <typename... Args>
CMT_INLINE std::string as_string(const Args&... args);

template <typename T, char t = static_cast<char>(-1), int width = -1, int prec = -1>
struct fmt_t
{
    const T& value;
};

namespace details
{

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
    return concat_cstring(make_cstring("%s"), make_cstring(cchars_t<chars...>()));
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
                          value_fmt(ctype_t<repr_type<T>>()).slice(csize_t<1>()));
}
template <typename T, char t, int width, int prec>
CMT_INLINE constexpr auto value_fmt(ctype_t<fmt_t<T, t, width, prec>> fmt)
{
    return concat_cstring(make_cstring("%"), value_fmt_arg(fmt), cstring<2>{ { t, 0 } });
}

template <typename T>
CMT_INLINE constexpr auto value_fmt(ctype_t<T>)
{
    return make_cstring("%s");
}

template <char... chars>
CMT_INLINE const char* pack_value(const cchars_t<chars...>&)
{
    return "";
}

#define CMT_STANDARD_PACK(type)                                                                              \
    CMT_INLINE type pack_value(type value) { return value; }

CMT_STANDARD_PACK(char)
CMT_STANDARD_PACK(signed char)
CMT_STANDARD_PACK(unsigned char)
CMT_STANDARD_PACK(signed short)
CMT_STANDARD_PACK(unsigned short)
CMT_STANDARD_PACK(signed int)
CMT_STANDARD_PACK(unsigned int)
CMT_STANDARD_PACK(signed long)
CMT_STANDARD_PACK(unsigned long)
CMT_STANDARD_PACK(signed long long)
CMT_STANDARD_PACK(unsigned long long)
CMT_STANDARD_PACK(double)
CMT_STANDARD_PACK(char*)
CMT_STANDARD_PACK(const char*)
CMT_STANDARD_PACK(void*)
CMT_STANDARD_PACK(const void*)

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

template <typename T>
CMT_INLINE auto pack_value(const T&)
{
    return pack_value(type_name<T>());
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
    constexpr auto fmt = value_fmt(ctype_t<std::decay_t<Arg>>());
    return build_fmt(replace_one(str, "{}", std::string(fmt.data())), ctypes_t<Args...>());
}
} // namespace details

template <typename T>
struct representation
{
    using type = T;
    static constexpr auto get(const T& value) CMT_NOEXCEPT { return details::pack_value(value); }
};

template <char t, int width = -1, int prec = -1, typename T>
CMT_INLINE fmt_t<T, t, width, prec> fmt(const T& value)
{
    return { value };
}

template <int width = -1, int prec = -1, typename T>
CMT_INLINE fmt_t<T, static_cast<char>(-1), width, prec> fmtwidth(const T& value)
{
    return { value };
}

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpragmas")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wgnu-string-literal-operator-template")

constexpr auto build_fmt_str(cchars_t<>, ctypes_t<>) { return make_cstring(""); }

template <char... chars, typename Arg, typename... Args>
constexpr auto build_fmt_str(cchars_t<'@', chars...>, ctypes_t<Arg, Args...>)
{
    return concat_cstring(details::value_fmt(ctype_t<std::decay_t<Arg>>()),
                          build_fmt_str(cchars_t<chars...>(), ctypes_t<Args...>()));
}

template <char ch, char... chars, typename... Args>
constexpr auto build_fmt_str(cchars_t<ch, chars...>, ctypes_t<Args...>)
{
    return concat_cstring(make_cstring(cchars_t<ch>()),
                          build_fmt_str(cchars_t<chars...>(), ctypes_t<Args...>()));
}

template <char... chars>
struct format_t
{
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

template <char... chars>
struct print_t
{
    template <typename... Args>
    CMT_INLINE void operator()(const Args&... args)
    {
        constexpr auto format_str = build_fmt_str(cchars_t<chars...>(), ctypes_t<repr_type<Args>...>());

        std::printf(format_str.data(), details::pack_value(args)...);
    }
};

#if defined CMT_COMPILER_GNU && !defined(CMT_COMPILER_INTEL)

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

#endif

CMT_PRAGMA_GNU(GCC diagnostic pop)

template <typename... Args>
CMT_INLINE void printfmt(const std::string& fmt, const Args&... args)
{
    const auto format_str = details::build_fmt(fmt, ctypes_t<repr_type<Args>...>());
    std::printf(format_str.data(), details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE void fprintfmt(FILE* f, const std::string& fmt, const Args&... args)
{
    const auto format_str = details::build_fmt(fmt, ctypes_t<repr_type<Args>...>());
    std::fprintf(f, format_str.data(), details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE int snprintfmt(char* str, size_t size, const std::string& fmt, const Args&... args)
{
    const auto format_str = details::build_fmt(fmt, ctypes_t<repr_type<Args>...>());
    return std::snprintf(str, size, format_str.data(),
                         details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE std::string format(const std::string& fmt, const Args&... args)
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

template <typename... Args>
CMT_INLINE void print(const Args&... args)
{
    constexpr const auto format_str = concat_cstring(details::get_value_fmt<Args>()...);
    const char* str                 = format_str.data();
    std::printf(str, details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE void println(const Args&... args)
{
    constexpr const auto format_str = concat_cstring(details::get_value_fmt<Args>()..., make_cstring("\n"));
    const char* str                 = format_str.data();
    std::printf(str, details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE void error(const Args&... args)
{
    constexpr const auto format_str = concat_cstring(details::get_value_fmt<Args>()...);
    const char* str                 = format_str.data();
    std::fprintf(stderr, str, details::pack_value(representation<Args>::get(args))...);
}

template <typename... Args>
CMT_INLINE void errorln(const Args&... args)
{
    constexpr const auto format_str = concat_cstring(details::get_value_fmt<Args>()..., make_cstring("\n"));
    const char* str                 = format_str.data();
    std::fprintf(stderr, str, details::pack_value(representation<Args>::get(args))...);
    std::fflush(stderr);
}

template <typename... Args>
CMT_INLINE std::string as_string(const Args&... args)
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
    using type = std::string;
    static std::string get(const named_arg<T>& value)
    {
        return std::string(value.name) + " = " + as_string(value.value);
    }
};

template <typename T1, typename T2>
struct representation<std::pair<T1, T2>>
{
    using type = std::string;
    static std::string get(const std::pair<T1, T2>& value)
    {
        return "(" + as_string(value.first) + "; " + as_string(value.second) + ")";
    }
};

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
CMT_INTRINSIC size_t trailing_zeros(const std::array<size_t, dims>& indices)
{
    for (size_t i = 0; i < dims; ++i)
    {
        if (indices[dims - 1 - i] != 0)
            return i;
    }
    return dims;
}

template <size_t dims>
CMT_INTRINSIC bool increment_indices(std::array<size_t, dims>& indices, const std::array<size_t, dims>& stop)
{
    indices[dims - 1] += 1;
    CMT_PRAGMA_GNU(clang diagnostic push)
#if CMT_HAS_WARNING("-Wpass-failed")
    CMT_PRAGMA_GNU(clang diagnostic ignored "-Wpass-failed")
#endif
    CMT_LOOP_UNROLL
    for (int i = dims - 1; i >= 0;)
    {
        if (CMT_LIKELY(indices[i] < stop[i]))
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
    CMT_PRAGMA_GNU(clang diagnostic pop)
    return true;
}
} // namespace details

template <typename U, typename Fmt>
CMT_INTRINSIC Fmt wrap_fmt(const U& val, ctype_t<Fmt>)
{
    return Fmt{ val };
}
template <typename U>
CMT_INTRINSIC U wrap_fmt(const U& val, ctype_t<void>)
{
    return val;
}

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
            std::string str = as_string(wrap_fmt(getter(index), cometa::ctype<Fmt>));
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

template <typename Fmt = void, typename Getter>
std::string array_to_string(size_t size, Getter&& getter, int max_columns = 16, int max_dimensions = INT_MAX,
                            std::string_view separator = ", ", std::string_view open = "{",
                            std::string_view close = "}")
{
    return array_to_string<Fmt>(std::array<size_t, 1>{ size }, std::forward<Getter>(getter), max_columns,
                                max_dimensions, std::move(separator), std::move(open), std::move(close));
}
template <typename Fmt = void, typename T>
std::string array_to_string(size_t size, T* data, int max_columns = 16, int max_dimensions = INT_MAX,
                            std::string_view separator = ", ", std::string_view open = "{",
                            std::string_view close = "}")
{
    return array_to_string<Fmt>(
        std::array<size_t, 1>{ size }, [data](std::array<size_t, 1> i) { return data[i.front()]; },
        max_columns, max_dimensions, std::move(separator), std::move(open), std::move(close));
}

template <typename T, size_t Size>
struct representation<std::array<T, Size>>
{
    using type = std::string;
    static std::string get(const std::array<T, Size>& value)
    {
        return array_to_string(value.size(), value.data());
    }
};
template <typename T, typename Allocator>
struct representation<std::vector<T, Allocator>>
{
    using type = std::string;
    static std::string get(const std::vector<T, Allocator>& value)
    {
        return array_to_string(value.size(), value.data());
    }
};
template <>
struct representation<std::string_view>
{
    using type = std::string;
    static std::string get(const std::string_view& value)
    {
        return std::string(value);
    }
};

} // namespace cometa

CMT_PRAGMA_GNU(GCC diagnostic pop)
