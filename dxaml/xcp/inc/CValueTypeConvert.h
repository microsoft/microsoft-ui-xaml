// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <type_traits>
#include "StaticAssertFalse.h"

enum ValueType : XUINT32;

namespace CValueDetails
{
    // Support for value type conversions.
    // Developer specifies allowed conversions in specialization of ConversionSpec as 
    // a list of supported value types from which to convert.  Conversions template
    // supports compile-time query for containment of value type, allows indexed access
    // to value types it holds and provides count of listed value types.
    // Count is used by CValue::TryGetValue to dispatch to a method with correct number
    // of cases in switch statement, which in turn invokes appropriate Convert method
    // supplied in ConversionSpec specialization.

    // Tag for dispatching on value type.
    template <ValueType valueType>
    struct tag_value_type {};

    // Tag for dispatching on count of value types.
    template <size_t count>
    struct tag_conversion_count {};

    // Check if value type is in variadic template args list.
    template <ValueType VT, ValueType... VTs>
    struct check_conversion_allowed
    {
        template <ValueType valueType>
        static constexpr bool check()
        {
            return VT == valueType || check_conversion_allowed<VTs...>::template check<valueType>();
        }
    };

    // Check if value type is in variadic template args list - specialization for last arg.
    template <ValueType VT>
    struct check_conversion_allowed<VT>
    {
        template <ValueType valueType>
        static constexpr bool check()
        {
            return VT == valueType;
        }
    };

    // Get value type stored in variadic template args list at position index.
    template <size_t index, ValueType VT, ValueType... VTs>
    struct get_conversion
    {
        static constexpr ValueType get()
        {
            return get_conversion<index - 1, VTs...>::get();
        }
    };

    // Get value type stored in variadic template args list at position index - specialization for a hit before end of list.
    template <ValueType VT, ValueType... VTs>
    struct get_conversion<0, VT, VTs...>
    {
        static constexpr ValueType get()
        {
            return VT;
        }
    };

    // Get value type stored in variadic template args list at position index - specialization for a hit at end of list.
    template <ValueType VT>
    struct get_conversion<0, VT>
    {
        static constexpr ValueType get()
        {
            return VT;
        }
    };

    template <ValueType... VTs>
    struct conversions
    {
        static constexpr size_t count = sizeof...(VTs);

        template <ValueType valueType>
        static constexpr bool check()
        {
            return check_conversion_allowed<VTs...>::template check<valueType>();
        }

        template <size_t index>
        static constexpr ValueType get()
        {
            return get_conversion<index, VTs...>::get();
        }
    };

    template <>
    struct conversions<>
    {
        static constexpr size_t count = 0;

        template <ValueType valueType>
        static constexpr bool check()
        {
            return false;
        }

        template <size_t index>
        static ValueType get()
        {
            static_assert_false("Trying to call get on non-convertible type.");
        }
    };

    // By default, there are no allowed conversions.  To override default, specialize for
    // value type you need, add template arguments to conversions<...> template and
    // provide appropriate conversion methods.  For example see ConversionSpec<valueDouble>.
    template <ValueType valueType>
    struct ConversionSpec
    {
        using conversions = conversions<>;
    };
}
