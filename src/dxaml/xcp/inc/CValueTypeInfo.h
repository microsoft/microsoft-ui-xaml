// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ValueType.h>
#include <CValueStores.h>
#include <CValueTypeConvert.h>
#include <InternalEventHandler.h>
#include <operators.h>
#include <minxcptypes.h>
#include <Indexes.g.h>
#include <windows.foundation.h>

class CValue;
class CDependencyObject;
class CThemeResource;
struct IUnknown;
struct IInspectable;

namespace Flyweight
{
    class PropertyValueObjectBase;
}

namespace CValueDetails
{
    // Holds info about basic type associated with value type, store type and empty value.
    // Store::StoredType is the type stored in Value union.
    // Store::MappedType is the type exposed outside of CValue.
    template <ValueType valueType> struct ValueTypeInfo {};

    template <>
    struct ValueTypeInfo<valueAny>
    {
        static constexpr ValueType myValueType = valueAny;
        using Type = nullptr_t;
        using Store = ValueStores::Empty<false>;
        static constexpr Type Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueNull>
    {
        static constexpr ValueType myValueType = valueNull;
        using Type = nullptr_t;
        using Store = ValueStores::Empty<true>;
        static constexpr Type Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueBool>
    {
        static constexpr ValueType myValueType = valueBool;
        using Type = bool;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = false;
    };

    template <>
    struct ValueTypeInfo<valueEnum>
    {
        struct Value
        {
            bool operator==(const Value& other) const
            {
                return m_value == other.m_value &&
                    m_typeIndex == other.m_typeIndex;
            }

            operator uint64_t() const { return m_value; }

            uint32_t m_value;
            KnownTypeIndex m_typeIndex;
        };

        static constexpr ValueType myValueType = valueEnum;
        using Type = Value;
        using Store = ValueStores::Value<myValueType, Type>;
        static const Type Empty;
    };

    template <>
    struct ValueTypeInfo<valueEnum8>
    {
        struct Value
        {
            bool operator==(const Value& other) const
            {
                return m_value == other.m_value &&
                    m_typeIndex == other.m_typeIndex;
            }

            operator uint64_t() const { return m_value; }

            uint8_t m_value;
            KnownTypeIndex m_typeIndex;
        };

        static constexpr ValueType myValueType = valueEnum8;
        using Type = Value;
        using Store = ValueStores::Value<myValueType, Type>;
        static const Type Empty;
    };

    template <>
    struct ValueTypeInfo<valueSigned>
    {
        static constexpr ValueType myValueType = valueSigned;
        using Type = int32_t;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = 0;
    };

    template <>
    struct ValueTypeInfo<valueUnsigned>
    {
        static constexpr ValueType myValueType = valueUnsigned;
        using Type = uint32_t;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = 0U;
    };

    template <>
    struct ValueTypeInfo<valueInt64>
    {
        static constexpr ValueType myValueType = valueInt64;
        using Type = int64_t;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = 0;
    };

    template <>
    struct ValueTypeInfo<valueUInt64>
    {
        static constexpr ValueType myValueType = valueUInt64;
        using Type = uint64_t;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = 0;
    };

    template <>
    struct ValueTypeInfo<valueFloat>
    {
        static constexpr ValueType myValueType = valueFloat;
        using Type = float;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = 0.0f;
    };

    template <>
    struct ValueTypeInfo<valueDouble>
    {
        static constexpr ValueType myValueType = valueDouble;
        using Type = double;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = 0.0;
    };

    template <>
    struct ValueTypeInfo<valueSignedArray>
    {
        static constexpr ValueType myValueType = valueSignedArray;
        using Type = int32_t;
        using Store = ValueStores::Array<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueFloatArray>
    {
        static constexpr ValueType myValueType = valueFloatArray;
        using Type = float;
        using Store = ValueStores::Array<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueDoubleArray>
    {
        static constexpr ValueType myValueType = valueDoubleArray;
        using Type = double;
        using Store = ValueStores::Array<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valuePointArray>
    {
        static constexpr ValueType myValueType = valuePointArray;
        using Type = XPOINTF;
        using Store = ValueStores::Array<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueString>
    {
        static constexpr ValueType myValueType = valueString;
        using Store = ValueStores::String<myValueType>;
        static Store::MappedType Empty;
    };

    template <>
    struct ValueTypeInfo<valueColor>
    {
        static constexpr ValueType myValueType = valueColor;
        using Type = uint32_t;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = 0;
    };

    template <>
    struct ValueTypeInfo<valuePoint>
    {
        static constexpr ValueType myValueType = valuePoint;
        using Type = XPOINTF;
        using Store = ValueStores::Reference<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueSize>
    {
        static constexpr ValueType myValueType = valueSize;
        using Type = XSIZEF;
        using Store = ValueStores::Reference<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueRect>
    {
        static constexpr ValueType myValueType = valueRect;
        using Type = XRECTF;
        using Store = ValueStores::Reference<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueThickness>
    {
        static constexpr ValueType myValueType = valueThickness;
        using Type = XTHICKNESS;
        using Store = ValueStores::Reference<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueGridLength>
    {
        static constexpr ValueType myValueType = valueGridLength;
        using Type = XGRIDLENGTH;
        using Store = ValueStores::Reference<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueCornerRadius>
    {
        static constexpr ValueType myValueType = valueCornerRadius;
        using Type = XCORNERRADIUS;
        using Store = ValueStores::Reference<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueDateTime>
    {
        static constexpr ValueType myValueType = valueDateTime;
        using Type = wf::DateTime;
        using Store = ValueStores::Value<myValueType, Type>;
        static const Type Empty;
    };

    template <>
    struct ValueTypeInfo<valueTimeSpan>
    {
        static constexpr ValueType myValueType = valueTimeSpan;
        using Type = wf::TimeSpan;
        using Store = ValueStores::Value<myValueType, Type>;
        static const Type Empty;
    };

    template <>
    struct ValueTypeInfo<valueObject>
    {
        static constexpr ValueType myValueType = valueObject;
        using Type = CDependencyObject;
        using Store = ValueStores::RefCounted<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueInternalHandler>
    {
        static constexpr ValueType myValueType = valueInternalHandler;
        using Type = INTERNAL_EVENT_HANDLER;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueIUnknown>
    {
        static constexpr ValueType myValueType = valueIUnknown;
        using Type = IUnknown;
        using Store = ValueStores::RefCounted<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueIInspectable>
    {
        static constexpr ValueType myValueType = valueIInspectable;
        using Type = IInspectable;
        using Store = ValueStores::RefCounted<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueTypeHandle>
    {
        static constexpr ValueType myValueType = valueTypeHandle;
        using Type = KnownTypeIndex;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = KnownTypeIndex::UnknownType;
    };

    template <>
    struct ValueTypeInfo<valueThemeResource>
    {
        static constexpr ValueType myValueType = valueThemeResource;
        using Type = CThemeResource;
        using Store = ValueStores::RefCounted<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valuePointer>
    {
        static constexpr ValueType myValueType = valuePointer;
        using Type = void*;
        using Store = ValueStores::Value<myValueType, Type>;
        static constexpr Type Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueVO>
    {
        static constexpr ValueType myValueType = valueVO;
        using Type = Flyweight::PropertyValueObjectBase;
        using Store = ValueStores::RefCounted<myValueType, Type>;
        static constexpr Type* Empty = nullptr;
    };

    template <>
    struct ValueTypeInfo<valueTextRange>
    {
        static constexpr ValueType myValueType = valueTextRange;
        using Type = TextRangeData;
        using Store = ValueStores::Value<myValueType, Type>;
        static const Type Empty;
    };
    // Conversions

    template <>
    struct ConversionSpec<valueDouble>
    {
        using conversions = conversions<
            valueFloat>;

        using return_type = typename ValueTypeInfo<valueDouble>::Store::MappedType;
        using valueFloat_type = typename ValueTypeInfo<valueFloat>::Store::MappedType;

        static return_type Convert(
            valueFloat_type value,
            tag_value_type<valueFloat>)
        {
            return static_cast<return_type>(value);
        }
    };

    template <>
    struct ConversionSpec<valueEnum>
    {
        using conversions = conversions<valueEnum8>;

        using return_type = typename ValueTypeInfo<valueEnum>::Store::MappedType;
        using valueEnum8_type = typename ValueTypeInfo<valueEnum8>::Store::MappedType;

        static return_type Convert(
            valueEnum8_type value,
            tag_value_type<valueEnum8>)
        {
            return return_type({ static_cast<uint32_t>(value.m_value), value.m_typeIndex });
        }
    };

    template <>
    struct ConversionSpec<valueEnum8>
    {
        using conversions = conversions<valueEnum>;

        using return_type = typename ValueTypeInfo<valueEnum8>::Store::MappedType;
        using valueEnum_type = typename ValueTypeInfo<valueEnum>::Store::MappedType;

        static return_type Convert(
            valueEnum_type value,
            tag_value_type<valueEnum>)
        {
            ASSERT(value.m_value <= UINT8_MAX);
            return return_type({ static_cast<uint8_t>(value.m_value), value.m_typeIndex });
        }
    };


}
