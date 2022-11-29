/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"

namespace cometa
{
template <typename Type, typename ErrEnum, ErrEnum OkValue = static_cast<ErrEnum>(0)>
struct result
{
    using value_type      = Type;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;

    using error_type = ErrEnum;

    constexpr static error_type ok_value = OkValue;

    constexpr result(const result&)         = default;
    constexpr result(result&&) CMT_NOEXCEPT = default;

    constexpr result(ErrEnum error) CMT_NOEXCEPT : m_error(error) {}

    template <typename ValueInit, CMT_ENABLE_IF(std::is_constructible_v<value_type, ValueInit>)>
    constexpr result(ValueInit&& value) CMT_NOEXCEPT : m_value(std::forward<ValueInit>(value)),
                                                       m_error(OkValue)
    {
    }

    constexpr result(const Type& value) CMT_NOEXCEPT : m_value(value), m_error(OkValue) {}
    constexpr result(Type&& value) CMT_NOEXCEPT : m_value(std::move(value)), m_error(OkValue) {}

    constexpr explicit operator bool() const { return m_error == OkValue; }
    constexpr const_reference operator*() const { return m_value; }
    constexpr reference operator*() { return m_value; }
    constexpr const_pointer operator->() const { return &m_value; }
    constexpr pointer operator->() { return &m_value; }

    constexpr const_reference value() const { return m_value; }
    constexpr reference value() { return m_value; }
    constexpr ErrEnum error() const { return m_error; }
    constexpr bool ok() const { return m_error == OkValue; }

private:
    Type m_value;
    ErrEnum m_error;
};
} // namespace cometa
