/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4814))

namespace cometa
{
template <typename T>
struct named_arg
{
    T value;
    const char* name;
};

struct named
{
    constexpr named(const char* name) CMT_NOEXCEPT : name(name) {}

    template <typename T>
    CMT_MEM_INTRINSIC constexpr named_arg<T> operator=(T&& value)
    {
        return named_arg<T>{ std::forward<T>(value), name };
    }
    const char* name;
};

inline named operator""_arg(const char* name, size_t) { return name; }
} // namespace cometa

CMT_PRAGMA_MSVC(warning(pop))
