/** @addtogroup meta
 *  @{
 */
#pragma once

#include "../meta.hpp"
#include <array>
#include <iterator>
#include <vector>

namespace kfr
{

template <typename Container>
using container_value_type = std::remove_pointer_t<decltype(std::data(std::declval<Container>()))>;

/// @brief Reference to array
template <typename T>
struct array_ref
{
public:
    using value_type             = T;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<pointer>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;

    constexpr array_ref() noexcept : m_data(nullptr), m_size(0) {}
    constexpr array_ref(const array_ref&) noexcept            = default;
    constexpr array_ref(array_ref&&) noexcept                 = default;
    constexpr array_ref& operator=(const array_ref&) noexcept = default;
    constexpr array_ref& operator=(array_ref&&) noexcept      = default;

    template <has_data_size Container>
    array_ref(Container&& cont) : array_ref(std::data(cont), std::size(cont))
    {
    }

    constexpr array_ref(std::initializer_list<T> vec) noexcept : m_data(vec.begin()), m_size(vec.size()) {}
    constexpr array_ref(T* data, size_type size) noexcept : m_data(data), m_size(size) {}

    constexpr reference front() const noexcept { return m_data[0]; }
    constexpr reference back() const noexcept { return m_data[m_size - 1]; }
    constexpr iterator begin() const noexcept { return m_data; }
    constexpr iterator end() const noexcept { return m_data + m_size; }
    constexpr const_iterator cbegin() const noexcept { return m_data; }
    constexpr const_iterator cend() const noexcept { return m_data + m_size; }
    constexpr pointer data() const noexcept { return m_data; }
    constexpr std::size_t size() const noexcept { return m_size; }
    constexpr bool empty() const noexcept { return !m_size; }
    constexpr reference operator[](std::size_t index) const { return m_data[index]; }

private:
    pointer m_data;
    size_type m_size;
};

template <typename T>
inline array_ref<T> make_array_ref(T* data, std::size_t size)
{
    return array_ref<T>(data, size);
}

template <has_data_size Container, typename T = container_value_type<Container>>
inline array_ref<T> make_array_ref(Container&& cont)
{
    return array_ref<T>(std::data(cont), std::size(cont));
}
} // namespace kfr
