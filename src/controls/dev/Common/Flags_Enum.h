// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <type_traits>

// Specialize this trait with value = true to enable bitwise operators.

template <typename T>
struct is_flags_enum
{
    static constexpr bool value = false;
};

template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
constexpr T operator|(T lhs, T rhs) noexcept
{
    using type = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<type>(lhs) | static_cast<type>(rhs));
}

template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
constexpr T operator&(T lhs, T rhs) noexcept
{
    using type = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<type>(lhs) & static_cast<type>(rhs));
}

template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
constexpr T operator^(T lhs, T rhs) noexcept
{
    using type = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<type>(lhs) ^ static_cast<type>(rhs));
}

template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
constexpr T operator<<(T lhs, size_t shift) noexcept
{
    using type = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<type>(lhs) << shift);
}

template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
constexpr T operator>>(T lhs, size_t shift) noexcept
{
    using type = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<type>(lhs) >> shift);
}

template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
constexpr T operator~(T lhs) noexcept
{
    using type = std::underlying_type_t<T>;
    return static_cast<T>(~static_cast<type>(lhs));
}

namespace flags_enum
{
    // Tests if ANY of value bits are set on lhs.
    template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
    constexpr bool is_set(T lhs, T value) noexcept
    {
        using type = std::underlying_type_t<T>;
        return static_cast<type>(lhs & value) != static_cast<type>(0);
    }

    // Tests if ALL of value bits are set on lhs.
    template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
    constexpr bool are_all_set(T lhs, T value) noexcept
    {
        return (lhs & value) == value;
    }

    template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
    constexpr T set(T lhs, T value) noexcept
    {
        return lhs | value;
    }

    template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
    constexpr T unset(T lhs, T value) noexcept
    {
        return lhs & (~value);
    }

    template<typename T, typename = std::enable_if_t<std::is_enum<T>::value && is_flags_enum<T>::value>>
    constexpr std::underlying_type_t<T> underlying_type(T lhs) noexcept
    {
        return static_cast<std::underlying_type_t<T>>(lhs);
    }
}