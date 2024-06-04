// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ValueType.h>
#include <type_traits>

namespace CValueDetails
{
    template <ValueType valueType> struct ValueTypeInfo;

    template <ValueType, typename = void>
    struct is_array
    {
        static constexpr bool value = false;
    };

    template <ValueType valueType>
    struct is_array<valueType, typename std::enable_if<ValueTypeInfo<valueType>::Store::isArray>::type>
    {
        static constexpr bool value = true;
    };

    template <ValueType, typename = void>
    struct is_wrappable
    {
        static constexpr bool value = false;
    };

    template <ValueType valueType>
    struct is_wrappable<valueType, typename std::enable_if<ValueTypeInfo<valueType>::Store::isWrappable>::type>
    {
        static constexpr bool value = true;
    };

    template <ValueType, typename = void>
    struct is_ref_counted
    {
        static constexpr bool value = false;
    };

    template <ValueType valueType>
    struct is_ref_counted<valueType, typename std::enable_if<ValueTypeInfo<valueType>::Store::manageRefCounts>::type>
    {
        static constexpr bool value = true;
    };
}
