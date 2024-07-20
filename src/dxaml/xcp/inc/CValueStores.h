// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ValueType.h>

class CValue;
struct xencoded_string_ptr;
class xstring_ptr;

namespace CValueDetails
{
    struct ValueStores
    {
        // Stores and accesses value stored with alignment natural to architecture.
        template <typename T>
        struct NaturalAccessor
        {
            using StoredType = T;

            static T Get(
                _In_ const StoredType& field)
            {
                return field;
            }

            static void Set(
                _Out_ StoredType& field,
                _In_ const T value)
            {
                field = value;
            }

            static void Copy(
                _Out_ StoredType& dest,
                _In_ const StoredType& source)
            {
                dest = source;
            }
        };

        // Stores and accesses value stored with alignment forced to 32-bits.
        template <typename T>
        struct PiecewiseAccessor
        {
            using StoredType = uint32_t[2];

            static T Get(
                _In_ const StoredType& field)
            {
                alignas(8) StoredType aligned;
                Copy(aligned, field);
                return *reinterpret_cast<T*>(&aligned);
            }

            static void Set(
                _Out_ StoredType& field,
                _In_ const T value)
            {
                const uint32_t* ptr = reinterpret_cast<const uint32_t*>(&value);
                field[0] = ptr[0];
                field[1] = ptr[1];
            }

            static void Copy(
                _Out_ StoredType& dest,
                _In_ const StoredType& source)
            {
                dest[0] = source[0];
                dest[1] = source[1];
            }
        };

        // Selects accessor for type T for architecture.
        template <typename T, size_t size>
        struct _pick_accessor
        {
            using Accessor = NaturalAccessor<T>;
            static_assert(size < 8, "Only size <= 8 is currently supported.");
        };

        // Specialization used to force 4-byte alignment.
        template <typename T>
        struct _pick_accessor<T, 8>
        {
            using Accessor = PiecewiseAccessor<T>;
        };

        template <typename T>
        struct pick_accessor
        {
            using Accessor = typename _pick_accessor<T, sizeof(T)>::Accessor;
        };

        // Degenerate case which does not store values, e.g. valueAny, valueNull.
        template <bool alwaysEqual>
        struct Empty
        {
            using StoredType = pick_accessor<nullptr_t>::Accessor::StoredType;
            using MappedType = nullptr_t;
            using ConstMappedType = const nullptr_t;

            static constexpr const bool isArray = false;
            static constexpr const bool isWrappable = false;
            static constexpr const bool manageRefCounts = false;

            static MappedType Get(
                _In_ const CValue& value)
            {
                return nullptr;
            }

            static void Set(
                _Out_ CValue& target,
                _In_ MappedType value)
            {}

            static void Copy(
                _Out_ CValue& target,
                _In_ const CValue& source)
            {}

            static void Move(
                _Out_ CValue& target,
                _Inout_ CValue&& source)
            {}

            static constexpr bool Compare(
                _In_ const CValue& dest,
                _In_ const CValue& source)
            {
                return alwaysEqual;
            }

            static void Destroy(
                _Inout_ CValue& target)
            {}
        };

        // Stores value types (e.g. valueSigned, valueDouble).
        template <ValueType valueType, typename Type>
        struct Value
        {
            using Accessor = typename pick_accessor<Type>::Accessor;
            using StoredType = typename Accessor::StoredType;
            using MappedType = Type;
            using ConstMappedType = const Type;

            static constexpr const bool isArray = false;
            static constexpr const bool isWrappable = false;
            static constexpr const bool manageRefCounts = false;

            static MappedType Get(
                _In_ const CValue& value)
            {
                return Accessor::Get(GetField<valueType>(value.m_value));
            }

            static void Set(
                _Out_ CValue& target,
                _In_ MappedType value)
            {
                Accessor::Set(GetField<valueType>(target.m_value), value);
            }

            static void Copy(
                _Out_ CValue& target,
                _In_ const CValue& source)
            {
                Accessor::Copy(GetField<valueType>(target.m_value), GetField<valueType>(source.m_value));
            }

            static void Move(
                _Out_ CValue &target,
                _Inout_ CValue&& source)
            {
                Accessor::Copy(GetField<valueType>(target.m_value), GetField<valueType>(source.m_value));
            }

            static bool Compare(
                _In_ const CValue& dest,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueType);
                return Get(dest) == Get(source);
            }

            static void Destroy(
                _Inout_ CValue& target)
            {}
        };

        // Stores heap-allocated value types (e.g. valueThickness, valuePoint).
        template <ValueType valueType, typename Type>
        struct Reference
        {
            using Accessor = typename pick_accessor<Type*>::Accessor;
            using StoredType = typename Accessor::StoredType;
            using MappedType = Type*;
            using ConstMappedType = const Type*;

            static constexpr const bool isArray = false;
            static constexpr const bool isWrappable = true;
            static constexpr const bool manageRefCounts = true;

            static MappedType Get(
                _In_ const CValue& value)
            {
                return Accessor::Get(GetField<valueType>(value.m_value));
            }

            static void Set(
                _Out_ CValue& target,
                _In_ MappedType value)
            {
                Accessor::Set(GetField<valueType>(target.m_value), value);
            }

            static void Copy(
                _Out_ CValue& target,
                _In_ const CValue& source)
            {
                Type* temp = nullptr;
                // Structs are heap allocated, make sure the pointer isn't null
                // before trying to copy. This way we keep the target valueType the same.
                if (auto val = Get(source))
                {
                    temp = new Type();
                    *temp = *val;
                }
                Accessor::Set(GetField<valueType>(target.m_value), temp);
            }

            static void Move(
                _Out_ CValue &target,
                _Inout_ CValue&& source)
            {
                Accessor::Copy(GetField<valueType>(target.m_value), GetField<valueType>(source.m_value));
            }

            static bool Compare(
                _In_ const CValue& dest,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueType);
                auto destVal = Get(dest);
                auto sourceVal = Get(source);
                if (destVal != nullptr && sourceVal != nullptr)
                {
                    // If both aren't null, then check the value
                    return *destVal == *sourceVal;
                }
                else
                {
                    // Otherwise either one or both are null, we can just do a simple
                    // comparison of the pointers.
                    return destVal == sourceVal;
                }
            }

            static void Destroy(
                _Inout_ CValue& target)
            {
                delete Get(target);
            }
        };

        // Stores heap-allocated array types (e.g. valueSignedArray).
        template <ValueType valueType, typename Type>
        struct Array
        {
            using Accessor = typename pick_accessor<Type*>::Accessor;
            using StoredType = typename Accessor::StoredType;
            using MappedType = Type*;
            using ConstMappedType = const Type*;

            static constexpr const bool isArray = true;
            static constexpr const bool isWrappable = true;
            static constexpr const bool manageRefCounts = false;

            static MappedType Get(
                _In_ const CValue& value)
            {
                return Accessor::Get(GetField<valueType>(value.m_value));
            }

            static void Set(
                _Out_ CValue& target,
                _In_ MappedType value)
            {
                Accessor::Set(GetField<valueType>(target.m_value), value);
            }

            static void Copy(
                _Out_ CValue& target,
                _In_ const CValue& source)
            {
                auto count = source.GetArrayElementCountInternal();
                Type* temp = new Type[count];
                memcpy(temp, Get(source), sizeof(Type) * count);
                Accessor::Set(GetField<valueType>(target.m_value), temp);
            }

            static void Move(
                _Out_ CValue &target,
                _Inout_ CValue&& source)
            {
                Accessor::Copy(GetField<valueType>(target.m_value), GetField<valueType>(source.m_value));
            }

            static bool Compare(
                _In_ const CValue& dest,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueType);
                return Get(dest) == Get(source) &&
                       dest.GetArrayElementCountInternal() == source.GetArrayElementCountInternal();
            }

            static void Destroy(
                _Inout_ CValue& target)
            {
                delete[] Get(target);
            }
        };

        // Stores reference-counted object pointers (e.g. valueObject).
        template <ValueType valueType, typename Type>
        struct RefCounted
        {
            using Accessor = typename pick_accessor<Type*>::Accessor;
            using StoredType = typename Accessor::StoredType;
            using MappedType = Type*;
            using ConstMappedType = const Type*;

            static constexpr const bool isArray = false;
            static constexpr const bool isWrappable = true;
            static constexpr const bool manageRefCounts = true;

            static MappedType Get(
                _In_ const CValue& value);

            static void Set(
                _Out_ CValue& target,
                _In_ MappedType value);

            static void Copy(
                _Out_ CValue& target,
                _In_ const CValue& source);

            static void Move(
                _Out_ CValue &target,
                _Inout_ CValue&& source);

            static bool Compare(
                _In_ const CValue& dest,
                _In_ const CValue& source);

            static void Destroy(
                _Inout_ CValue& target);

            static void AddRef(
                _Out_ CValue& target);

            static void Release(
                _Out_ CValue& target);
        };

        // Stores strings as xencoded_string_ptr (e.g. valueString)
        template <ValueType valueType>
        struct String
        {
            using Accessor = typename pick_accessor<xencoded_string_ptr>::Accessor;
            using StoredType = typename Accessor::StoredType;
            using MappedType = xstring_ptr;
            using ConstMappedType = const xstring_ptr;

            static constexpr const bool isArray = false;
            static constexpr const bool isWrappable = false;
            static constexpr const bool manageRefCounts = false;

            static MappedType Get(
                _In_ const CValue& value);

            static void Set(
                _Out_ CValue& target,
                _In_ MappedType& value);

            static void Copy(
                _Out_ CValue& target,
                _In_ const CValue& source);

            static void Move(
                _Out_ CValue &target,
                _Inout_ CValue&& source);

            static bool Compare(
                _In_ const CValue& dest,
                _In_ const CValue& source);

            static void Destroy(
                _Inout_ CValue& target);

            static void MoveXString(
                _Out_ CValue& target,
                _Inout_ xstring_ptr&& source);
        };
    };
}