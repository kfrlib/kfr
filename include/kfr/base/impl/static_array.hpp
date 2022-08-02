#include "../../cometa.hpp"
#include "../../kfr.h"

namespace kfr
{
using namespace cometa;

template <typename T, size_t>
using type_for = T;

template <typename T, typename indices_t>
struct static_array_base;

template <typename T, size_t... indices>
struct static_array_base<T, csizes_t<indices...>>
{
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using reference       = value_type&;
    using pointer         = value_type*;
    using iterator        = pointer;
    using const_reference = const value_type&;
    using const_pointer   = const value_type*;
    using const_iterator  = const_pointer;

    constexpr static size_t static_size = sizeof...(indices);

    constexpr static_array_base()                         = default;
    constexpr static_array_base(const static_array_base&) = default;
    constexpr static_array_base(static_array_base&&)      = default;

    KFR_MEM_INTRINSIC constexpr static_array_base(type_for<value_type, indices>... args)
    {
        (static_cast<void>(array[indices] = args), ...);
    }

    constexpr static_array_base& operator=(const static_array_base&) = default;
    constexpr static_array_base& operator=(static_array_base&&) = default;

    template <int dummy = 0, CMT_ENABLE_IF(dummy == 0 && static_size > 1)>
    KFR_MEM_INTRINSIC constexpr static_array_base(value_type value)
    {
        (static_cast<void>(array[indices] = value), ...);
    }

    KFR_MEM_INTRINSIC vec<T, static_size> operator*() const { return read<static_size>(data()); }

    KFR_MEM_INTRINSIC static_array_base(const vec<T, static_size>& v) { write(data(), v); }

    KFR_MEM_INTRINSIC constexpr const value_type* data() const { return std::data(array); }
    KFR_MEM_INTRINSIC constexpr value_type* data() { return std::data(array); }

    KFR_MEM_INTRINSIC constexpr const_iterator begin() const { return std::begin(array); }
    KFR_MEM_INTRINSIC constexpr iterator begin() { return std::begin(array); }
    KFR_MEM_INTRINSIC constexpr const_iterator cbegin() const { return std::begin(array); }

    KFR_MEM_INTRINSIC constexpr const_iterator end() const { return std::end(array); }
    KFR_MEM_INTRINSIC constexpr iterator end() { return std::end(array); }
    KFR_MEM_INTRINSIC constexpr const_iterator cend() const { return std::end(array); }

    KFR_MEM_INTRINSIC constexpr const_reference operator[](size_t index) const { return array[index]; }
    KFR_MEM_INTRINSIC constexpr reference operator[](size_t index) { return array[index]; }

    KFR_MEM_INTRINSIC constexpr const_reference front() const { return array[0]; }
    KFR_MEM_INTRINSIC constexpr reference front() { return array[0]; }

    KFR_MEM_INTRINSIC constexpr const_reference back() const { return array[static_size - 1]; }
    KFR_MEM_INTRINSIC constexpr reference back() { return array[static_size - 1]; }

    KFR_MEM_INTRINSIC constexpr bool empty() const { return false; }

    KFR_MEM_INTRINSIC constexpr size_t size() const { return std::size(array); }

    KFR_MEM_INTRINSIC constexpr bool operator==(const static_array_base& other) const
    {
        return ((array[indices] == other.array[indices]) && ...);
    }
    KFR_MEM_INTRINSIC constexpr bool operator!=(const static_array_base& other) const
    {
        return !operator==(other);
    }

private:
    T array[static_size];
};
} // namespace kfr
