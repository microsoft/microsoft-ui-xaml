// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Value.h"
#include "StaticStore.h"
#include "DXamlTypes.h"
#include "TypeTableStructs.h"
#include "MetadataAPI.h"
#include "Enums.g.h"
#include "LifetimeUtils.h"


REFERENCE_ELEMENT_NAME_IMPL(xaml::CornerRadius, L"Microsoft.UI.Xaml.CornerRadius");
REFERENCE_ELEMENT_NAME_IMPL(xaml::Duration, L"Microsoft.UI.Xaml.Duration");
REFERENCE_ELEMENT_NAME_IMPL(wut::FontWeight, L"Windows.UI.Text.FontWeight");
REFERENCE_ELEMENT_NAME_IMPL(xaml::GridLength, L"Microsoft.UI.Xaml.GridLength");
REFERENCE_ELEMENT_NAME_IMPL(xaml::Thickness, L"Microsoft.UI.Xaml.Thickness");
REFERENCE_ELEMENT_NAME_IMPL(xaml_animation::KeyTime, L"Microsoft.UI.Xaml.Media.Animation.KeyTime");
REFERENCE_ELEMENT_NAME_IMPL(xaml_animation::RepeatBehavior, L"Microsoft.UI.Xaml.Media.Animation.RepeatBehavior");
REFERENCE_ELEMENT_NAME_IMPL(wu::Color, L"Windows.UI.Color");
REFERENCE_ELEMENT_NAME_IMPL(xaml_media::Matrix, L"Microsoft.UI.Xaml.Media.Matrix");
REFERENCE_ELEMENT_NAME_IMPL(xaml_media::Media3D::Matrix3D, L"Microsoft.UI.Xaml.Media.Media3D.Matrix3D");
REFERENCE_ELEMENT_NAME_IMPL(wxaml_interop::TypeName, L"Windows.UI.Xaml.Interop.TypeName");
REFERENCE_ELEMENT_NAME_IMPL(xaml_docs::TextRange, L"Microsoft.UI.Xaml.Documents.TextRange");
REFERENCE_ELEMENT_NAME_IMPL(wf::Point, L"Windows.Foundation.Point");
REFERENCE_ELEMENT_NAME_IMPL(wf::Rect, L"Windows.Foundation.Rect");

void DirectUI::ReferenceDetails::ReferenceTraits<wxaml_interop::TypeName>::Destroy(wxaml_interop::TypeName& member)
{
    ::WindowsDeleteString(member.Name);
}

STDMETHODIMP DirectUI::ReferenceDetails::ReferenceTraits<wxaml_interop::TypeName>::Set(
    wxaml_interop::TypeName& member,
    const wxaml_interop::TypeName& param
    )
{
    ::WindowsDeleteString(member.Name);

    member.Kind = param.Kind;
    return WindowsDuplicateString(param.Name, &member.Name);
}

STDMETHODIMP DirectUI::ReferenceDetails::ReferenceTraits<wxaml_interop::TypeName>::Get(
    const wxaml_interop::TypeName& member,
    wxaml_interop::TypeName* param)
{
    param->Kind = member.Kind;
    return WindowsDuplicateString(member.Name, &param->Name);
}

_Check_return_ HRESULT
DirectUI::PropertyValue::AreEqual(_In_ IInspectable* oldValue, _In_ IInspectable* newValue, _Out_ bool* areEqual) noexcept
{
    wf::PropertyType oldValueType;
    wf::PropertyType newValueType;
    ctl::ComPtr<wf::IPropertyValue> oldValueAsPV;
    ctl::ComPtr<wf::IPropertyValue> newValueAsPV;

    // Check if these are the same object by comparing the identity unknowns.
    *areEqual = ctl::are_equal(oldValue, newValue);
    if (*areEqual)
    {
        return S_OK;
    }

    // If exactly one of the values is nullptr, they're not equal.
    if (oldValue == nullptr || newValue == nullptr)
    {
        return S_OK;
    }

    oldValueAsPV.Attach(ctl::get_property_value(oldValue));
    newValueAsPV.Attach(ctl::get_property_value(newValue));

    // If we're not dealing with PropertyValues, then the objects aren't equal, because we already
    // did a reference check.
    if (oldValueAsPV != nullptr && newValueAsPV != nullptr)
    {
        // If the types don't match, then they're not equal.
        IFC_RETURN(oldValueAsPV->get_Type(&oldValueType));
        IFC_RETURN(newValueAsPV->get_Type(&newValueType));
        if (oldValueType == newValueType)
        {
            switch (oldValueType)
            {
            // For strings we do a deep check.
            case wf::PropertyType_String:
                {
                    wrl_wrappers::HString strOldString;
                    wrl_wrappers::HString strNewString;

                    IFC_RETURN(oldValueAsPV->GetString(strOldString.GetAddressOf()));
                    IFC_RETURN(newValueAsPV->GetString(strNewString.GetAddressOf()));

                    if (strOldString == strNewString)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Char16:
                {
                    WCHAR oldChar = 0;
                    WCHAR newChar = 0;

                    IFC_RETURN(oldValueAsPV->GetChar16(&oldChar));
                    IFC_RETURN(newValueAsPV->GetChar16(&newChar));

                    if (oldChar == newChar)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Boolean:
                {
                    BOOLEAN oldValueBool = FALSE;
                    BOOLEAN newValueBool = FALSE;

                    IFC_RETURN(oldValueAsPV->GetBoolean(&oldValueBool));
                    IFC_RETURN(newValueAsPV->GetBoolean(&newValueBool));

                    if (oldValueBool == newValueBool)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_UInt8:
                {
                    XUINT8 oldValueUint8 = 0;
                    XUINT8 newValueUint8 = 0;

                    IFC_RETURN(oldValueAsPV->GetUInt8(&oldValueUint8));
                    IFC_RETURN(newValueAsPV->GetUInt8(&newValueUint8));

                    if (oldValueUint8 == newValueUint8)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Int16:
                {
                    XINT16 oldValueInt16 = 0;
                    XINT16 newValueInt16 = 0;

                    IFC_RETURN(oldValueAsPV->GetInt16(&oldValueInt16));
                    IFC_RETURN(newValueAsPV->GetInt16(&newValueInt16));

                    if (oldValueInt16 == newValueInt16)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_UInt16:
                {
                    XUINT16 oldValueUint16 = 0;
                    XUINT16 newValueUint16 = 0;

                    IFC_RETURN(oldValueAsPV->GetUInt16(&oldValueUint16));
                    IFC_RETURN(newValueAsPV->GetUInt16(&newValueUint16));

                    if (oldValueUint16 == newValueUint16)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Int32:
                {
                    XINT32 oldValueInt32 = 0;
                    XINT32 newValueInt32 = 0;

                    IFC_RETURN(oldValueAsPV->GetInt32(&oldValueInt32));
                    IFC_RETURN(newValueAsPV->GetInt32(&newValueInt32));

                    if (oldValueInt32 == newValueInt32)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_UInt32:
                {
                    XUINT32 oldValueUint32 = 0;
                    XUINT32 newValueUint32 = 0;

                    IFC_RETURN(oldValueAsPV->GetUInt32(&oldValueUint32));
                    IFC_RETURN(newValueAsPV->GetUInt32(&newValueUint32));

                    if (oldValueUint32 == newValueUint32)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Int64:
                {
                    XINT64 oldValueInt64 = 0;
                    XINT64 newValueInt64 = 0;

                    IFC_RETURN(oldValueAsPV->GetInt64(&oldValueInt64));
                    IFC_RETURN(newValueAsPV->GetInt64(&newValueInt64));

                    if (oldValueInt64 == newValueInt64)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_UInt64:
                {
                    XUINT64 oldValueUint64 = 0;
                    XUINT64 newValueUint64 = 0;

                    IFC_RETURN(oldValueAsPV->GetUInt64(&oldValueUint64));
                    IFC_RETURN(newValueAsPV->GetUInt64(&newValueUint64));

                    if (oldValueUint64 == newValueUint64)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Single:
                {
                    FLOAT oldValueFloat = 0.0f;
                    FLOAT newValueFloat = 0.0f;

                    IFC_RETURN(oldValueAsPV->GetSingle(&oldValueFloat));
                    IFC_RETURN(newValueAsPV->GetSingle(&newValueFloat));

                    if (oldValueFloat == newValueFloat)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Double:
                {
                    XDOUBLE oldValueDouble = 0;
                    XDOUBLE newValueDouble = 0;

                    IFC_RETURN(oldValueAsPV->GetDouble(&oldValueDouble));
                    IFC_RETURN(newValueAsPV->GetDouble(&newValueDouble));

                    if (oldValueDouble == newValueDouble)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Guid:
                {
                    GUID oldValueGuid = {};
                    GUID newValueGuid = {};

                    IFC_RETURN(oldValueAsPV->GetGuid(&oldValueGuid));
                    IFC_RETURN(newValueAsPV->GetGuid(&newValueGuid));

                    if (oldValueGuid == newValueGuid)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_DateTime:
                {
                    wf::DateTime oldValueDateTime = {};
                    wf::DateTime newValueDateTime = {};

                    IFC_RETURN(oldValueAsPV->GetDateTime(&oldValueDateTime));
                    IFC_RETURN(newValueAsPV->GetDateTime(&newValueDateTime));

                    if (oldValueDateTime.UniversalTime == newValueDateTime.UniversalTime)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_TimeSpan:
                {
                    wf::TimeSpan oldValueTimeSpan = {};
                    wf::TimeSpan newValueTimeSpan = {};

                    IFC_RETURN(oldValueAsPV->GetTimeSpan(&oldValueTimeSpan));
                    IFC_RETURN(newValueAsPV->GetTimeSpan(&newValueTimeSpan));

                    if (oldValueTimeSpan.Duration == newValueTimeSpan.Duration)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Point:
                {
                    wf::Point oldValuePoint = {};
                    wf::Point newValuePoint = {};

                    IFC_RETURN(oldValueAsPV->GetPoint(&oldValuePoint));
                    IFC_RETURN(newValueAsPV->GetPoint(&newValuePoint));

                    if (oldValuePoint.X == newValuePoint.X &&
                        oldValuePoint.Y == newValuePoint.Y)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Size:
                {
                    wf::Size oldValueSize = {};
                    wf::Size newValueSize = {};

                    IFC_RETURN(oldValueAsPV->GetSize(&oldValueSize));
                    IFC_RETURN(newValueAsPV->GetSize(&newValueSize));

                    if (oldValueSize.Width == newValueSize.Width &&
                        oldValueSize.Height == newValueSize.Height)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_Rect:
                {
                    wf::Rect oldValueRect = {};
                    wf::Rect newValueRect = {};

                    IFC_RETURN(oldValueAsPV->GetRect(&oldValueRect));
                    IFC_RETURN(newValueAsPV->GetRect(&newValueRect));

                    if (oldValueRect.X == newValueRect.X &&
                        oldValueRect.Y == newValueRect.Y &&
                        oldValueRect.Width == newValueRect.Width &&
                        oldValueRect.Height == newValueRect.Height)
                    {
                        *areEqual = true;
                    }
                }
                break;
            case wf::PropertyType_OtherType:
                {
                    const CClassInfo* oldTypeInfo = nullptr;
                    const CClassInfo* newTypeInfo = nullptr;

                    // TODO: Add TryGetClassInfoFromWinRTPropertyType to MetadataAPI and use that instead.
                    if (SUCCEEDED(MetadataAPI::GetClassInfoFromWinRTPropertyType(oldValueAsPV.Get(), wf::PropertyType_OtherType, &oldTypeInfo)) &&
                        SUCCEEDED(MetadataAPI::GetClassInfoFromWinRTPropertyType(newValueAsPV.Get(), wf::PropertyType_OtherType, &newTypeInfo)))
                    {
                        // Make sure the types match. If this is a built-in enum, we know how to get the underlying values.
                        if (oldTypeInfo == newTypeInfo && newTypeInfo->IsBuiltinType() && newTypeInfo->IsEnum())
                        {
                            UINT oldEnumValue = 0, newEnumValue = 0;
                            IFC_RETURN(GetEnumValueFromKnownWinRTBox(oldValueAsPV.Get(), oldTypeInfo, &oldEnumValue));
                            IFC_RETURN(GetEnumValueFromKnownWinRTBox(newValueAsPV.Get(), newTypeInfo, &newEnumValue));
                            *areEqual = (oldEnumValue == newEnumValue);
                        }
                    }
                }
                break;
            }
        }
    }

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateEmpty(_Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateEmpty(ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromBoolean(_In_ BOOLEAN bValue, _Out_ IInspectable **ppValue)
{
    //IFC(StaticStore::GetPropertyValueStaticsNoRef()->CreateBoolean(bValue, ppValue));
    IFC_RETURN(StaticStore::GetBoolean(bValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromUInt8(_In_ BYTE nValue, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateUInt8(nValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromInt16(_In_ INT16 nValue, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateInt16(nValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromUInt16(_In_ UINT16 nValue, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateUInt16(nValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromInt32(_In_ INT32 nValue, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateInt32(nValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromUInt32(_In_ UINT32 nValue, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateUInt32(nValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromInt64(_In_ INT64 nValue, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateInt64(nValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromUInt64(_In_ UINT64 nValue, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateUInt64(nValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromDouble(_In_ DOUBLE nValue, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateDouble(nValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromSingle(_In_ FLOAT nValue, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateSingle(nValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromString(_In_opt_ HSTRING hString, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateString(hString, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromChar16(_In_ WCHAR chValue, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateChar16(chValue, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromTimeSpan(_In_ wf::TimeSpan value, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateTimeSpan(value, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromDateTime(_In_ wf::DateTime value, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateDateTime(value, ppValue));

    return S_OK;
}


_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromPoint(_In_ wf::Point value, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreatePoint(value, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromRect(_In_ wf::Rect value, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateRect(value, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromSize(_In_ wf::Size value, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateSize(value, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromGuid(_In_ GUID value, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateGuid(value, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromVisibility(_In_ xaml::Visibility visibility, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetVisibilityValue(visibility, ppValue));

    return S_OK;
}

_Check_return_
HRESULT
DirectUI::PropertyValue::CreateFromTextRange(_In_ xaml_docs::TextRange value, _Out_ IInspectable **ppValue)
{
    IFC_RETURN(StaticStore::GetTextRangeValue(value, ppValue));

    return S_OK;
}

bool
DirectUI::PropertyValue::IsNullOrEmpty(_In_ IInspectable *pInValue)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    bool fResult = false;
    wf::PropertyType valueType = wf::PropertyType_Empty;
    wf::IPropertyValue* pValue = NULL;

    if (pInValue == NULL)
    {
        fResult = TRUE;
        goto Cleanup;
    }

    pValue = ctl::get_property_value(pInValue);
    if (pValue != NULL)
    {
        IFC(pValue->get_Type(&valueType));
        fResult = valueType == wf::PropertyType_Empty ? TRUE : FALSE;
    }

Cleanup:

    ReleaseInterface(pValue);

    return fResult;
}

_Check_return_
HRESULT
DirectUI::ValueWeakReference::Create(_In_ IInspectable *value, _Out_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;
    DirectUI::ValueWeakReference *pNewValue = NULL;
    IWeakReference *pWeakReference = NULL;

    IFC(ctl::as_weakref(pWeakReference, value));

    pNewValue = new DirectUI::ValueWeakReference();

    pNewValue->m_pWeakReference = pWeakReference;
    pWeakReference = NULL;
    *ppValue = pNewValue;
    pNewValue = NULL;

Cleanup:

    ReleaseInterface(pNewValue);
    ReleaseInterface(pWeakReference);

    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::ValueWeakReference::GetInspectable(_Out_ IInspectable **value)
{
    return m_pWeakReference->Resolve(__uuidof(IInspectable), value);
}

