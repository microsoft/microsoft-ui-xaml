// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.foundation.h>

namespace Flyweight
{
    class PropertyValueObjectBase;
}

class CDependencyObject;

// Methods for unboxing values stored by CValue.
// As long as Automation::CValue exists, these are templates.  Once it is removed, this code can be made into non-template functions.
namespace CValueConvert
{
    // Unbox passed IPropertyValue if it is one of supported types.
    template<typename CValueType>
    _Check_return_ HRESULT TryUnboxPropertyValue(
        _In_ wf::IPropertyValue* inValue,
        _Out_ CValueType& outValue,
        _Out_ bool& success);

    // Try unboxing valueIInspectable if it is passed in inValue, if it is not, wrap outValue around inValue.
    template<typename CValueType>
    _Check_return_ HRESULT EnsurePropertyValueUnboxed(
        _In_ const CValueType& inValue,
        _Out_ CValueType& outValue);

    // Unbox passed CDependencyObject if it is one of supported types.
    template<typename CValueType>
    _Check_return_ HRESULT TryUnboxCDependencyObjectValue(
        _In_ CDependencyObject* inValue,
        _Out_ CValueType& outValue,
        _Out_ bool& success);

    // Try unboxing valueObject if it is passed in inValue, if it is not, set and add-ref outValue to a value passed in inValue.
    template<typename CValueType>
    _Check_return_ HRESULT EnsureCDependencyObjectValueUnboxed(
        _In_ const CValueType& inValue,
        _Out_ CValueType& outValue);

    template<typename CValueType>
    _Check_return_ HRESULT TryUnboxValueObject(
        _In_ const Flyweight::PropertyValueObjectBase* inValue,
        _Out_ CValueType& outValue,
        _Out_ bool& success);

    template<typename CValueType>
    _Check_return_ HRESULT EnsureValueObjectUnboxed(
        _In_ const CValueType& inValue,
        _Out_ CValueType& outValue);
}