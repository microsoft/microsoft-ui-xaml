// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CValueStores.h>
#include <CValue.h>
#include <CDependencyObject.h>
#include <ThemeResource.h>
#include <xstring_ptr.h>
#include <ValueObjectBase.h>

namespace CValueDetails
{
    // RefCounted - this definition is in cpp file in order to avoid including the world with h file.
    // Unfortunately, it means that all ValueTypeInfos which reference it need to be explicitly instantiated here...

    template struct ValueStores::RefCounted<valueObject, CDependencyObject>;
    template struct ValueStores::RefCounted<valueIUnknown, IUnknown>;
    template struct ValueStores::RefCounted<valueIInspectable, IInspectable>;
    template struct ValueStores::RefCounted<valueThemeResource, CThemeResource>;
    template struct ValueStores::RefCounted<valueVO, Flyweight::PropertyValueObjectBase>;

    template <ValueType valueType, typename Type>
    typename ValueStores::RefCounted<valueType, Type>::MappedType ValueStores::RefCounted<valueType, Type>::Get(
        _In_ const CValue& value)
    {
        return Accessor::Get(GetField<valueType>(value.m_value));
    }

    template <ValueType valueType, typename Type>
    void ValueStores::RefCounted<valueType, Type>::Set(
        _Out_ CValue& target,
        _In_ MappedType value)
    {
        Accessor::Set(GetField<valueType>(target.m_value), value);
    }

    template <ValueType valueType, typename Type>
    void ValueStores::RefCounted<valueType, Type>::Copy(
        _Out_ CValue& target,
        _In_ const CValue& source)
    {
        Accessor::Copy(GetField<valueType>(target.m_value), GetField<valueType>(source.m_value));
        AddRef(target);
    }

    template <ValueType valueType, typename Type>
    void ValueStores::RefCounted<valueType, Type>::Move(
        _Out_ CValue &target,
        _Inout_ CValue&& source)
    {
        Accessor::Copy(GetField<valueType>(target.m_value), GetField<valueType>(source.m_value));
    }

    template <ValueType valueType, typename Type>
    bool ValueStores::RefCounted<valueType, Type>::Compare(
        _In_ const CValue& dest,
        _In_ const CValue& source)
    {
        ASSERT(source.GetType() == valueType);
        return Get(dest) == Get(source);
    }

    template <ValueType valueType, typename Type>
    void ValueStores::RefCounted<valueType, Type>::Destroy(
        _Out_ CValue& target)
    {
        auto value = Get(target);

        if (value)
        {
            value->Release();
        }
    }

    template <ValueType valueType, typename Type>
    void ValueStores::RefCounted<valueType, Type>::AddRef(
        _Out_ CValue& target)
    {
        auto value = Get(target);

        if (value)
        {
            value->AddRef();
        }
    }

    template <ValueType valueType, typename Type>
    void ValueStores::RefCounted<valueType, Type>::Release(
        _Out_ CValue& target)
    {
        auto value = Get(target);

        if (value)
        {
            value->Release();
        }
    }

    // String

    template struct ValueStores::String<valueString>;

    template <ValueType valueType>
    typename ValueStores::String<valueType>::MappedType ValueStores::String<valueType>::Get(
        _In_ const CValue& value)
    {
        return xstring_ptr::Decode(Accessor::Get(GetField<valueType>(value.m_value)));
    }

    template <ValueType valueType>
    void ValueStores::String<valueType>::Set(
        _Out_ CValue& target,
        _In_ MappedType& value)
    {
        Accessor::Set(GetField<valueType>(target.m_value), xstring_ptr::Encode(value));
    }

    template <ValueType valueType>
    void ValueStores::String<valueType>::Copy(
        _Out_ CValue& target,
        _In_ const CValue& source)
    {
        Accessor::Set(GetField<valueType>(target.m_value), Accessor::Get(GetField<valueType>(source.m_value)).Clone());
    }

    template <ValueType valueType>
    void ValueStores::String<valueType>::Move(
        _Out_ CValue &target,
        _Inout_ CValue&& source)
    {
        auto& sourceField = GetField<valueType>(source.m_value);

        Accessor::Set(GetField<valueType>(target.m_value), Accessor::Get(sourceField));
        Accessor::Set(sourceField, xencoded_string_ptr::NullString());
    }

    template <ValueType valueType>
    bool ValueStores::String<valueType>::Compare(
        _In_ const CValue& dest,
        _In_ const CValue& source)
    {
        ASSERT(source.GetType() == valueType);
        return Get(dest) == Get(source);
    }

    template <ValueType valueType>
    void ValueStores::String<valueType>::Destroy(
        _Out_ CValue& target)
    {
        // Use Accessor::Get here because the ValueStores::Get calls into xstring_ptr::Decode which will return a copy of the string.
        // We need to get at the underlying string without duplicating it so we can actually release it.
        auto value = Accessor::Get(GetField<valueType>(target.m_value));
        value.Reset();
    }

    template <ValueType valueType>
    void ValueStores::String<valueType>::MoveXString(
        _Out_ CValue& target,
        _Inout_ xstring_ptr&& source)
    {
        Accessor::Set(GetField<valueType>(target.m_value), xstring_ptr::MoveEncode(std::move(source)));
    }
}