/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"

namespace cometa
{
template <typename T>
struct range
{
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using diff_type       = decltype(std::declval<T>() - std::declval<T>());

    constexpr range(value_type begin, value_type end, diff_type step) noexcept : value_begin(begin),
                                                                                 value_end(end),
                                                                                 step(step)
    {
    }

    struct iterator
    {
        value_type value;
        diff_type step;
        const_reference operator*() const { return value; }
        const_pointer operator->() const { return &value; }
        iterator& operator++()
        {
            value += step;
            return *this;
        }
        iterator operator++(int)
        {
            iterator copy = *this;
            ++(*this);
            return copy;
        }
        bool operator!=(const iterator& other) const
        {
            return step > 0 ? value < other.value : value > other.value;
        }
    };
    value_type value_begin;
    value_type value_end;
    diff_type step;
    iterator begin() const { return iterator{ value_begin, step }; }
    iterator end() const { return iterator{ value_end, step }; }
};

template <typename T>
range<T> make_range(T begin, T end)
{
    return range<T>(begin, end, end > begin ? 1 : -1);
}

template <typename T, typename diff_type = decltype(std::declval<T>() - std::declval<T>())>
range<T> make_range(T begin, T end, diff_type step)
{
    return range<T>(begin, end, step);
}
}
