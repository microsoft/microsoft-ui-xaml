// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Flyweight
{
    class PropertyValueObjectBase;
}

// Repackages values to match property types.
class ValueBuffer
{
public:
    ValueBuffer(_In_ CCoreServices* core)
        : m_core(core)
    {}

    _Check_return_ HRESULT Repackage(
        _Const_ _In_ const CDependencyProperty* pDP);

    _Check_return_ HRESULT RepackageValue(
        _Const_ _In_ const CDependencyProperty* pDP,
        _Const_ _In_ const CValue* value);

    _Check_return_ HRESULT RepackageValueAndSetPtr(
        _Const_ _In_ const CDependencyProperty* pDP,
        _Const_ _In_ const CValue* value, 
        _Inout_ CValue** ppValue);

    _Check_return_ HRESULT RepackageValueAndCopy(
        _Const_ _In_ const CDependencyProperty* pDP,
        _Const_ _In_ const CValue* value,
        _Inout_ CValue& outValue);

    CValue* GetValuePtr()
    {
        return &m_value;
    }

    CValue* operator&()
    {
        return &m_value;
    }

private:
    union
    {
        XFLOAT _4floats[4];
        XTHICKNESS _thickness;
        XGRIDLENGTH _gridLength;
        XCORNERRADIUS _cornerRadius;
        XRECTF _rect;
        XUINT8 _2bytes[2];
        XPOINTD _ptd;
    } m_primitive = {};

    CCoreServices* m_core{};
    xref_ptr<CDependencyObject> m_obj;
    xref_ptr<Flyweight::PropertyValueObjectBase> m_vo;
    CValue m_value;

    _Check_return_ HRESULT RepackageValue_Object(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue,
        bool failOnUnhandled);

    _Check_return_ HRESULT RepackageValue_Float(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_Int(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_Enum(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_Bool(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_Size(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_Point(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_Null(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_DynamicToObject(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_Double(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_TimeSpan(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_Thickness(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_String(
        _In_ const CDependencyProperty* pDP,
        _In_ ValueType storageType,
        _Inout_ CValue*& pValue);

    _Check_return_ HRESULT RepackageValue_TextRange(
        _In_ const CDependencyProperty* dp,
        _In_ ValueType storageType,
        _Inout_ CValue*& valueInOut);

    _Check_return_ HRESULT ValueObjectFromDouble(
        KnownTypeIndex type,
        double value);

    _Check_return_ HRESULT ValueObjectFromString(
        KnownTypeIndex type,
        _In_ const xstring_ptr_view& value);
};