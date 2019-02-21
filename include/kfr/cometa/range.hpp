/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"

namespace cometa
{

/// @brief Iterable range
template <typename T>
struct range
{
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using diff_type       = decltype(std::declval<T>() - std::declval<T>());

    constexpr range(value_type begin, value_type end, diff_type step) CMT_NOEXCEPT : min(begin),
                                                                                     max(end),
                                                                                     step(step)
    {
    }

    struct iterator
    {
        value_type value;
        diff_type step;
        constexpr const_reference operator*() const { return value; }
        constexpr const_pointer operator->() const { return &value; }
        constexpr iterator& operator++()
        {
            value += step;
            return *this;
        }
        constexpr iterator operator++(int)
        {
            iterator copy = *this;
            ++(*this);
            return copy;
        }
        constexpr bool operator!=(const iterator& other) const
        {
            return step > 0 ? value < other.value : value > other.value;
        }
    };
    value_type min;
    value_type max;
    diff_type step;
    constexpr iterator begin() const { return iterator{ min, step }; }
    constexpr iterator end() const { return iterator{ max, step }; }

    constexpr T distance() const { return max - min; }
};

/// @brief Make iterable range object
template <typename T>
constexpr range<T> make_range(T begin, T end)
{
    return range<T>(begin, end, end > begin ? 1 : -1);
}

/// @brief Make iterable range object with step
template <typename T, typename D>
constexpr range<std::common_type_t<T, D>> make_range(T begin, T end, D step)
{
    return range<std::common_type_t<T, D>>(begin, end, step);
}
} // namespace cometa
