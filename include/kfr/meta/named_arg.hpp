/** @addtogroup meta
 *  @{
 */
#pragma once

#include "../meta.hpp"

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4814))

namespace kfr
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
    KFR_MEM_INTRINSIC constexpr named_arg<T> operator=(T&& value)
    {
        return named_arg<T>{ std::forward<T>(value), name };
    }
    const char* name;
};

inline named operator""_arg(const char* name, size_t) { return name; }
} // namespace kfr

KFR_PRAGMA_MSVC(warning(pop))
