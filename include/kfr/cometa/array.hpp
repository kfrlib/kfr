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

    constexpr array_ref() CMT_NOEXCEPT : m_data(nullptr), m_size(0) {}
    constexpr array_ref(const array_ref&) CMT_NOEXCEPT = default;
    constexpr array_ref(array_ref&&) CMT_NOEXCEPT      = default;
#ifdef CMT_COMPILER_GNU
    constexpr array_ref& operator=(const array_ref&) CMT_NOEXCEPT = default;
    constexpr array_ref& operator=(array_ref&&) CMT_NOEXCEPT = default;
#else
    array_ref& operator=(const array_ref&) = default;
    array_ref& operator=(array_ref&&) = default;
#endif

    template <size_t N>
    constexpr array_ref(value_type (&arr)[N]) CMT_NOEXCEPT : m_data(arr), m_size(N)
    {
    }
    template <size_t N>
    constexpr array_ref(const std::array<T, N>& arr) CMT_NOEXCEPT : m_data(arr.data()), m_size(N)
    {
    }
    template <size_t N>
    constexpr array_ref(std::array<T, N>& arr) CMT_NOEXCEPT : m_data(arr.data()), m_size(N)
    {
    }
    template <typename Alloc>
    constexpr array_ref(const std::vector<T, Alloc>& vec) CMT_NOEXCEPT : m_data(vec.data()),
                                                                         m_size(vec.size())
    {
    }

    template <typename Container, CMT_ENABLE_IF(has_data_size<Container>)>
    array_ref(Container& cont) : array_ref(cont.data(), cont.size())
    {
    }

    constexpr array_ref(const std::initializer_list<T>& vec) CMT_NOEXCEPT : m_data(vec.begin()),
                                                                            m_size(vec.size())
    {
    }
    template <typename InputIter>
    constexpr array_ref(InputIter first, InputIter last) CMT_NOEXCEPT : m_data(std::addressof(*first)),
                                                                        m_size(std::distance(first, last))
    {
    }
    constexpr array_ref(T* data, size_type size) CMT_NOEXCEPT : m_data(data), m_size(size) {}

    constexpr reference front() const CMT_NOEXCEPT { return m_data[0]; }
    constexpr reference back() const CMT_NOEXCEPT { return m_data[m_size - 1]; }
    constexpr iterator begin() const CMT_NOEXCEPT { return m_data; }
    constexpr iterator end() const CMT_NOEXCEPT { return m_data + m_size; }
    constexpr const_iterator cbegin() const CMT_NOEXCEPT { return m_data; }
    constexpr const_iterator cend() const CMT_NOEXCEPT { return m_data + m_size; }
    constexpr pointer data() const CMT_NOEXCEPT { return m_data; }
    constexpr std::size_t size() const CMT_NOEXCEPT { return m_size; }
    constexpr bool empty() const CMT_NOEXCEPT { return !m_size; }
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

template <typename Container, CMT_ENABLE_IF(has_data_size<Container>),
          typename T = remove_pointer<decltype(std::declval<Container>().data())>>
inline array_ref<T> make_array_ref(Container& cont)
{
    return array_ref<T>(cont.data(), cont.size());
}

template <typename Container, CMT_ENABLE_IF(has_data_size<Container>),
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

template <typename C>
constexpr auto elementtype(C& c)
{
    return c[0];
}
template <typename C>
constexpr auto elementtype(const C& c)
{
    return c[0];
}
template <typename E>
constexpr E elementtype(const std::initializer_list<E>&)
{
    return {};
}
template <typename T, std::size_t N>
constexpr T elementtype(T (&)[N])
{
    return {};
}

template <typename C>
constexpr auto data(C& c) -> decltype(c.data())
{
    return c.data();
}
template <typename C>
constexpr auto data(const C& c) -> decltype(c.data())
{
    return c.data();
}
template <typename T, std::size_t N>
constexpr T* data(T (&array)[N]) CMT_NOEXCEPT
{
    return array;
}
template <typename T>
constexpr T* data(T* array) CMT_NOEXCEPT
{
    return array;
}
template <typename E>
constexpr const E* data(const std::initializer_list<E>& il) CMT_NOEXCEPT
{
    return il.begin();
}

template <typename C>
constexpr auto size(const C& c) -> decltype(c.size())
{
    return c.size();
}
template <typename T, std::size_t N>
constexpr std::size_t size(const T (&)[N]) CMT_NOEXCEPT
{
    return N;
}
} // namespace cometa
