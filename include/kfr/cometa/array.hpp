/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"
#include <array>
#include <iterator>
#include <vector>

namespace cometa
{

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
    constexpr array_ref(const array_ref&) noexcept = default;
    constexpr array_ref(array_ref&&) noexcept      = default;
    constexpr array_ref& operator=(const array_ref&) noexcept = default;
    constexpr array_ref& operator=(array_ref&&) noexcept = default;

    template <size_t N>
    constexpr array_ref(value_type (&arr)[N]) noexcept : m_data(arr), m_size(N)
    {
    }
    template <size_t N>
    constexpr array_ref(const std::array<T, N>& arr) noexcept : m_data(arr.data()), m_size(N)
    {
    }
    template <size_t N>
    constexpr array_ref(std::array<T, N>& arr) noexcept : m_data(arr.data()), m_size(N)
    {
    }
    template <typename... Ts>
    constexpr array_ref(const std::vector<T, Ts...>& vec) noexcept : m_data(vec.data()), m_size(vec.size())
    {
    }
    template <typename... Ts, CMT_ENABLE_IF(sizeof...(Ts), is_const<T>::value)>
    constexpr array_ref(const std::vector<remove_const<T>, Ts...>& vec) noexcept : m_data(vec.data()),
                                                                                   m_size(vec.size())
    {
    }
    template <typename... Ts>
    constexpr array_ref(std::vector<T, Ts...>& vec) noexcept : m_data(vec.data()), m_size(vec.size())
    {
    }
    template <typename InputIter>
    constexpr array_ref(InputIter first, InputIter last) noexcept : m_data(std::addressof(*first)),
                                                                    m_size(std::distance(first, last))
    {
    }
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

template <typename T, std::size_t size>
inline array_ref<T> make_array_ref(T (&data)[size])
{
    return array_ref<T>(data);
}

template <typename T>
inline array_ref<T> make_array_ref(T* data, std::size_t size)
{
    return array_ref<T>(data, data + size);
}

template <typename Container, CMT_ENABLE_IF(has_data_size<Container>::value),
          typename T = remove_pointer<decltype(std::declval<Container>().data())>>
inline array_ref<T> make_array_ref(Container& cont)
{
    return array_ref<T>(cont.data(), cont.size());
}

template <typename Container, CMT_ENABLE_IF(has_data_size<Container>::value),
          typename T = remove_pointer<decltype(std::declval<Container>().data())>>
inline array_ref<const T> make_array_ref(const Container& cont)
{
    return array_ref<const T>(cont.data(), cont.size());
}

template <typename T>
inline array_ref<T> make_array_ref(std::vector<T>& cont)
{
    return array_ref<T>(cont.data(), cont.size());
}
template <typename T>
inline array_ref<const T> make_array_ref(const std::vector<T>& cont)
{
    return array_ref<const T>(cont.data(), cont.size());
}
}
