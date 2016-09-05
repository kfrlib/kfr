/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"

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
    constexpr named(const char* name) noexcept : name(name) {}

    template <typename T>
    CMT_INTRIN constexpr named_arg<T> operator=(T&& value)
    {
        return named_arg<T>{ std::forward<T>(value), name };
    }
    const char* name;
};

inline named operator""_arg(const char* name, size_t) { return name; }
}
