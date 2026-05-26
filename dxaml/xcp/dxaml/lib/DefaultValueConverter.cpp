// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DefaultValueConverter.h"
#include "StaticStore.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;

// Body for pure virtual destructor (C++ limitation)
DefaultValueConverter::~DefaultValueConverter() {}

_Check_return_
HRESULT
DefaultValueConverter::CreateConverter(_In_ const CClassInfo *pSourceType,
                                       _In_ const CClassInfo *pTargetType,
                                       _Outptr_ IValueConverterInternal **ppConverter)
{
    HRESULT hr = S_OK;
    IValueConverterInternal *pNewConverter = NULL;

    IFCPTR(ppConverter);
    *ppConverter = NULL;

    if (pSourceType->GetIndex() == KnownTypeIndex::String)
    {
        if (pTargetType->IsNumericType())
        {
            IFC(NumberStringValueConverter::CreateConverter(pTargetType, /*fStringToNumber*/TRUE, &pNewConverter));
        }
        else if (pTargetType->GetIndex() == KnownTypeIndex::Uri)
        {
            IFC(StringToUriValueConverter::CreateConverter(&pNewConverter));
        }
        else if (pTargetType->GetIndex() == KnownTypeIndex::Guid)
        {
            IFC(StringToGuidValueConverter::CreateConverter(&pNewConverter));
        }
        else
        {
            IFC(CoreValueConverter::CreateConverter(&pNewConverter));
        }
    }
    else if (pTargetType->GetIndex() == KnownTypeIndex::String)
    {
        if (pSourceType->IsNumericType())
        {
            IFC(NumberStringValueConverter::CreateConverter(pSourceType, /*fStringToNumber*/FALSE, &pNewConverter));
        }
        else if (pSourceType->GetIndex() == KnownTypeIndex::Uri)
        {
            IFC(UriToStringValueConverter::CreateConverter(&pNewConverter));
        }
        else if (pSourceType->GetIndex() == KnownTypeIndex::Guid)
        {
            IFC(GuidToStringValueConverter::CreateConverter(&pNewConverter));
        }
        else
        {
            IFC(StringValueConverter::CreateConverter(&pNewConverter));
        }
    }
    else if (pSourceType->IsNumericType() && pTargetType->IsNumericType())
    {
        IFC(NumberTypeValueConverter::CreateConverter(pTargetType, pSourceType, &pNewConverter));
    }
    // we only care about the case where the Source is URI and target is object (ImageSource).
    else if (pSourceType->GetIndex() == KnownTypeIndex::Uri)
    {
       // first convert the URI to string.
       IFC(CoreUriToValueConverter::CreateConverter(&pNewConverter));
    }
    else if (pSourceType->GetIndex() == KnownTypeIndex::Boolean && pTargetType->GetIndex() == KnownTypeIndex::Visibility)
    {
        IFC(BoolToVisibilityValueConverter::CreateConverter(&pNewConverter));
    }

    *ppConverter = pNewConverter;
    pNewConverter = NULL;

Cleanup:
    ReleaseInterface(pNewConverter);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ConvertHelper
//
//  Synopsis: Converts value to the destination's type value.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
DefaultValueConverter::ConvertHelper(_In_ IInspectable *pInValue,
                                     _In_ const CClassInfo *pDestinationType,
                                     //CultureInfo culture,
                                     _In_ bool fForward,
                                     _Outptr_ IInspectable **ppDestinationValue)
{
    HRESULT hr = S_OK;
    wf::PropertyType valueType = wf::PropertyType_Empty;
    ctl::ComPtr<IInspectable> spNewValue;
    wrl_wrappers::HString strNewValue;
    wrl_wrappers::HString strValue;
    ctl::ComPtr<wf::IPropertyValue> spValue;

    spValue.Attach(ctl::get_property_value(pInValue));   // TODO: In Phase2 create the templates to get the data out

    IFCPTR(spValue);
    IFCPTR(ppDestinationValue);
    *ppDestinationValue = NULL;

    IFC(spValue->get_Type(&valueType));

    if (valueType == wf::PropertyType_String)
    {
        wf::PropertyType destinationType = wf::PropertyType_Empty;

        IFC(spValue->GetString(strValue.GetAddressOf()));

        switch(pDestinationType->GetIndex())
        {
            case KnownTypeIndex::Char16:
                destinationType = wf::PropertyType_Char16;
                break;
            case KnownTypeIndex::Int16:
                destinationType = wf::PropertyType_Int16;
                break;
            case KnownTypeIndex::UInt16:
                destinationType = wf::PropertyType_UInt16;
                break;
            case KnownTypeIndex::Int32:
                destinationType = wf::PropertyType_Int32;
                break;
            case KnownTypeIndex::UInt32:
                destinationType = wf::PropertyType_UInt32;
                break;
            case KnownTypeIndex::Int64:
                destinationType = wf::PropertyType_Int64;
                break;
            case KnownTypeIndex::UInt64:
                destinationType = wf::PropertyType_UInt64;
                break;
            case KnownTypeIndex::Float:
                destinationType = wf::PropertyType_Single;
                break;
            case KnownTypeIndex::Double:
                destinationType = wf::PropertyType_Double;
                break;

            default:
                // Can't convert this type
                IFC(E_FAIL);

        }

        IFC(ValueConversionHelpers::ConvertStringToValue(strValue.Get(), destinationType, &spNewValue));
        *ppDestinationValue = spNewValue.Detach();
    }
    else if (pDestinationType->GetIndex() == KnownTypeIndex::String)
    {
        if (!ValueConversionHelpers::CanConvertValueToString(valueType))
        {
            IFC(E_FAIL);
        }

        IFC(ValueConversionHelpers::ConvertValueToString(spValue.Get(), valueType, strNewValue.GetAddressOf()));
        IFC(PropertyValue::CreateFromString(strNewValue.Get(), &spNewValue));
        *ppDestinationValue = spNewValue.Detach();
    }
    else if (PropertyValue::IsNumericPropertyType(valueType) && pDestinationType->IsNumericType())
    {
        XDOUBLE sourceValue = 0;
        switch(valueType)
        {
            case wf::PropertyType_UInt8:
            {
                XUINT8 value = 0;
                IFC(spValue->GetUInt8(&value));
                sourceValue = static_cast<XDOUBLE>(value);
            }
            break;

            case wf::PropertyType_Char16:
            {
                WCHAR value = 0;
                IFC(spValue->GetChar16(&value));
                sourceValue = static_cast<XDOUBLE>(value);
            }
            break;

            case wf::PropertyType_Int16:
            {
                XINT16 value = 0;
                IFC(spValue->GetInt16(&value));
                sourceValue = static_cast<XDOUBLE>(value);
            }
            break;

            case wf::PropertyType_UInt16:
            {
                XUINT16 value = 0;
                IFC(spValue->GetUInt16(&value));
                sourceValue = static_cast<XDOUBLE>(value);
            }
            break;

            case wf::PropertyType_Int32:
            {
                XINT32 value = 0;
                IFC(spValue->GetInt32(&value));
                sourceValue = static_cast<XDOUBLE>(value);
            }
            break;

            case wf::PropertyType_UInt32:
            {
                XUINT32 value = 0;
                IFC(spValue->GetUInt32(&value));
                sourceValue = static_cast<XDOUBLE>(value);
            }
            break;

            case wf::PropertyType_Int64:
            {
                XINT64 value = 0;
                IFC(spValue->GetInt64(&value));
                sourceValue = static_cast<XDOUBLE>(value);
            }
            break;

            case wf::PropertyType_UInt64:
            {
                XUINT64 value = 0;
                IFC(spValue->GetUInt64(&value));
                sourceValue = static_cast<XDOUBLE>(value);
            }
            break;

            case wf::PropertyType_Single:
            {
                XFLOAT value = 0;
                IFC(spValue->GetSingle(&value));
                sourceValue = static_cast<XDOUBLE>(value);
            }
            break;

            case wf::PropertyType_Double:
            {
                XDOUBLE value = 0;
                IFC(spValue->GetDouble(&value));
                sourceValue = value;
            }
            break;

            default:
                IFC(E_FAIL);
        }

        switch(pDestinationType->GetIndex())
        {
            case KnownTypeIndex::Int32:
                IFC(PropertyValue::CreateFromInt32(static_cast<XINT32>(sourceValue), &spNewValue));
                break;
            case KnownTypeIndex::UInt32:
                IFC(PropertyValue::CreateFromUInt32(static_cast<XUINT32>(sourceValue), &spNewValue));
                break;
            case KnownTypeIndex::Char16:
                IFC(PropertyValue::CreateFromChar16(static_cast<XUINT16>(sourceValue), &spNewValue));
                break;
            case KnownTypeIndex::Int16:
                IFC(PropertyValue::CreateFromInt16(static_cast<XINT16>(sourceValue), &spNewValue));
                break;
            case KnownTypeIndex::UInt16:
                IFC(PropertyValue::CreateFromUInt16(static_cast<XUINT16>(sourceValue), &spNewValue));
                break;
            case KnownTypeIndex::Int64:
                IFC(PropertyValue::CreateFromInt64(static_cast<XINT64>(sourceValue), &spNewValue));
                break;
            case KnownTypeIndex::UInt64:
                IFC(PropertyValue::CreateFromUInt64(static_cast<XUINT64>(sourceValue), &spNewValue));
                break;
            case KnownTypeIndex::Float:
                IFC(PropertyValue::CreateFromSingle(static_cast<XFLOAT>(sourceValue), &spNewValue));
                break;
            case KnownTypeIndex::Double:
                IFC(PropertyValue::CreateFromDouble(sourceValue, &spNewValue));
                break;
            default:
                IFC(E_FAIL);
        }

        *ppDestinationValue = spNewValue.Detach();
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
BoolToVisibilityValueConverter::Convert(
    _In_ IInspectable *pSource,
    _In_ const CClassInfo *pTargetType,
    _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
    _Outptr_ IInspectable **ppTargetValue)
{
    ctl::ComPtr<wf::IPropertyValue> spSourceAsPV;
    BOOLEAN bSource;
    wf::PropertyType sourceType = wf::PropertyType_Empty;

    IFCEXPECT_RETURN(pTargetType->GetIndex() == KnownTypeIndex::Visibility);

    spSourceAsPV.Attach(ctl::get_property_value(pSource));
    IFC_RETURN(spSourceAsPV->get_Type(&sourceType));
    IFCEXPECT_RETURN(sourceType == wf::PropertyType_Boolean);

    IFC_RETURN(spSourceAsPV->GetBoolean(&bSource));

    if (bSource)
    {
        // true => Visible
        IFC_RETURN(PropertyValue::CreateFromVisibility(xaml::Visibility::Visibility_Visible, ppTargetValue));
    }
    else
    {
        // false => Collapsed
        IFC_RETURN(PropertyValue::CreateFromVisibility(xaml::Visibility::Visibility_Collapsed, ppTargetValue));
    }

    return S_OK;
}

_Check_return_
HRESULT
BoolToVisibilityValueConverter::ConvertBack(
    _In_ IInspectable *pTarget,
    _In_ const CClassInfo *pSourceType,
    _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
    _Outptr_ IInspectable **ppSourceValue)
{
    ctl::ComPtr<wf::IPropertyValue> spTargetAsPV;
    xaml::Visibility targetAsEnum;
    wf::PropertyType targetType = wf::PropertyType_Empty;

    IFCEXPECT_RETURN(pSourceType->GetIndex() == KnownTypeIndex::Boolean);

    spTargetAsPV.Attach(ctl::get_property_value(pTarget));
    IFC_RETURN(spTargetAsPV->get_Type(&targetType));
    IFCEXPECT_RETURN(targetType == wf::PropertyType_OtherType); // MUX expects enums to be wf::PropertyType_OtherType

    ctl::ComPtr<wf::IReference<xaml::Visibility>> spRef;
    IFC_RETURN(ctl::do_query_interface(spRef, pTarget));
    IFC_RETURN(spRef->get_Value(&targetAsEnum));

    switch (targetAsEnum)
    {
        case xaml::Visibility::Visibility_Visible:
        {
            // Visible => true
            IFC_RETURN(PropertyValue::CreateFromBoolean(TRUE, ppSourceValue));
            break;
        }
        case xaml::Visibility::Visibility_Collapsed:
        {
            // Collapsed => false
            IFC_RETURN(PropertyValue::CreateFromBoolean(FALSE, ppSourceValue));
            break;
        }
        default:
        {
            IFC_RETURN(E_FAIL);
        }
    }

    return S_OK;
}

_Check_return_
HRESULT
BoolToVisibilityValueConverter::CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter)
{
    *ppNewConverter = new BoolToVisibilityValueConverter();

    return S_OK;
}

_Check_return_
HRESULT
NumberStringValueConverter::CreateConverter(_In_ const CClassInfo *pNumberType,
                                            _In_ bool fStringToNumber,
                                            _Outptr_ IValueConverterInternal **ppConverter)
{
    HRESULT hr = S_OK;
    IValueConverterInternal *pNewConverter = NULL;
    const CClassInfo *pStringType = NULL;

    IFCPTR(ppConverter);
    *ppConverter = NULL;

    pStringType = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::String);

    if (fStringToNumber)
    {
        pNewConverter = new NumberStringValueConverter(pStringType, pNumberType);
    }
    else
    {
        pNewConverter = new NumberStringValueConverter(pNumberType, pStringType);
    }

    *ppConverter = pNewConverter;
    pNewConverter = NULL;

Cleanup:
    ReleaseInterface(pNewConverter);
    RRETURN(hr);
}

_Check_return_
HRESULT
NumberTypeValueConverter::CreateConverter(_In_ const CClassInfo *pSourceType,
                                          _In_ const CClassInfo *pTargetType,
                                          _Outptr_ IValueConverterInternal **ppConverter)
{
    HRESULT hr = S_OK;
    IValueConverterInternal *pNewConverter = NULL;

    IFCPTR(ppConverter);
    *ppConverter = NULL;

    pNewConverter = new NumberTypeValueConverter(pSourceType, pTargetType);

    *ppConverter = pNewConverter;
    pNewConverter = NULL;

Cleanup:
    ReleaseInterface(pNewConverter);
    RRETURN(hr);
}

_Check_return_
HRESULT
StringToUriValueConverter::Convert(
    _In_ IInspectable *pSource,
    _In_ const CClassInfo *pTargetType,
    _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
    _Outptr_ IInspectable **ppTargetValue)
{
    IFCEXPECT_RETURN(pTargetType->GetIndex() == KnownTypeIndex::Uri);

    wrl_wrappers::HString strUriString;
    IFC_RETURN(ctl::do_get_value(*strUriString.GetAddressOf(), pSource));

    ctl::ComPtr<wf::IUriRuntimeClassFactory> spUriFactory;
    IFC_RETURN(StaticStore::GetUriFactory(&spUriFactory));

    ctl::ComPtr<wf::IUriRuntimeClass> spUri;
    IFC_RETURN(spUriFactory->CreateUri(strUriString.Get(), &spUri));

    *ppTargetValue = spUri.Detach();

    return S_OK;
}

_Check_return_
HRESULT
StringToUriValueConverter::CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter)
{
    HRESULT hr = S_OK;

    *ppNewConverter = new StringToUriValueConverter();

    RRETURN(hr);//RRETURN_REMOVAL
}


_Check_return_
HRESULT
UriToStringValueConverter::Convert(
   _In_ IInspectable *pSource,
   _In_ const CClassInfo *pTargetType,
   _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
   _Outptr_ IInspectable **ppTargetValue)
{
    HRESULT hr = S_OK;
    wf::IUriRuntimeClass *pUri = NULL;
    wrl_wrappers::HString strUriString;

    IFCEXPECT(pTargetType->GetIndex() == KnownTypeIndex::String);

    IFC(ctl::do_query_interface(pUri, pSource));

    IFC(pUri->get_AbsoluteUri(strUriString.GetAddressOf()));

    IFC(PropertyValue::CreateFromString(strUriString.Get(), ppTargetValue));

Cleanup:

    ReleaseInterface(pUri);

    RRETURN(hr);
}

_Check_return_
HRESULT
UriToStringValueConverter::CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter)
{
    HRESULT hr = S_OK;

    *ppNewConverter = new UriToStringValueConverter();

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_
HRESULT
GuidToStringValueConverter::Convert(
    _In_ IInspectable *pSource,
    _In_ const CClassInfo *pTargetType,
    _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
    _Outptr_ IInspectable **ppTargetValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IPropertyValue> spPropertyValue;
    wrl_wrappers::HString strValue;

    IFC(ctl::do_query_interface(spPropertyValue, pSource));

    IFC(ValueConversionHelpers::ConvertValueToString(spPropertyValue.Get(), wf::PropertyType_Guid, strValue.GetAddressOf()));
    IFC(DirectUI::PropertyValue::CreateFromString(strValue.Get(), ppTargetValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
GuidToStringValueConverter::ConvertBack(
    _In_ IInspectable *pTarget,
    _In_ const CClassInfo *pSourceType,
    _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
    _Outptr_ IInspectable **ppSourceValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IPropertyValue> spPropertyValue;
    wrl_wrappers::HString strTarget;

    IFC(ctl::do_query_interface(spPropertyValue, pTarget));
    IFC(spPropertyValue->GetString(strTarget.GetAddressOf()));

    IFC(ValueConversionHelpers::ConvertStringToValue(strTarget.Get(), wf::PropertyType_Guid, ppSourceValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
GuidToStringValueConverter::CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter)
{
    HRESULT hr = S_OK;

    *ppNewConverter = new GuidToStringValueConverter();

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_
HRESULT
StringToGuidValueConverter::Convert(
    _In_ IInspectable *pSource,
    _In_ const CClassInfo *pTargetType,
    _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
    _Outptr_ IInspectable **ppTargetValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IPropertyValue> spPropertyValue;
    wrl_wrappers::HString strTarget;

    IFC(ctl::do_query_interface(spPropertyValue, pSource));
    IFC(spPropertyValue->GetString(strTarget.GetAddressOf()));

    IFC(ValueConversionHelpers::ConvertStringToValue(strTarget.Get(), wf::PropertyType_Guid, ppTargetValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
StringToGuidValueConverter::ConvertBack(
    _In_ IInspectable *pTarget,
    _In_ const CClassInfo *pSourceType,
    _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
    _Outptr_ IInspectable **ppSourceValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IPropertyValue> spPropertyValue;
    wrl_wrappers::HString strValue;

    IFC(ctl::do_query_interface(spPropertyValue, pTarget));

    IFC(ValueConversionHelpers::ConvertValueToString(spPropertyValue.Get(), wf::PropertyType_Guid, strValue.GetAddressOf()));
    IFC(DirectUI::PropertyValue::CreateFromString(strValue.Get(), ppSourceValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
StringToGuidValueConverter::CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter)
{
    HRESULT hr = S_OK;

    *ppNewConverter = new StringToGuidValueConverter();

    RRETURN(hr);//RRETURN_REMOVAL
}


_Check_return_
HRESULT
CoreUriToValueConverter::Convert(
   _In_ IInspectable *pSource,
   _In_ const CClassInfo *pTargetType,
   _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
   _Outptr_ IInspectable **ppTargetValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strUriString;
    ctl::ComPtr<wf::IUriRuntimeClass> spUri;

    // first get the string out of the source.
    IFC(ctl::do_query_interface(spUri, pSource));

    IFC(spUri->get_AbsoluteUri(strUriString.GetAddressOf()));

    IFCEXPECT(strUriString.Get() != NULL);

    IFC_NOTRACE(CoreValueConverter::ConvertStringToCoreValue(pTargetType, strUriString.Get(), ppTargetValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
CoreUriToValueConverter::CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter)
{
    HRESULT hr = S_OK;

    *ppNewConverter = new CoreUriToValueConverter();

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_
HRESULT
CoreValueConverter::Convert(
   _In_ IInspectable *pSource,
   _In_ const CClassInfo *pTargetType,
   _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
   _Outptr_ IInspectable **ppTargetValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strValue;

    // TODO: do_get_value for strings will not fail
    // if the incoming value is not a string
    IFC(ctl::do_get_value(*strValue.GetAddressOf(), pSource));
    IFCEXPECT_NOTRACE(strValue.Get() != NULL);

    IFC_NOTRACE(CoreValueConverter::ConvertStringToCoreValue(pTargetType,strValue.Get(), ppTargetValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
CoreValueConverter::ConvertStringToCoreValue(
   _In_ const CClassInfo *pTargetType,
   _In_ HSTRING hValue,
   _Outptr_ IInspectable **ppTargetValue)
{
    HRESULT hr = S_OK;
    xstring_ptr strValue;
    CValue sConvertedValue;

    IFC(xstring_ptr::CloneRuntimeStringHandle(hValue, &strValue));

    IFC_NOTRACE(CoreImports::ConvertStringToTypedCValue(DXamlCore::GetCurrentNoCreate()->GetHandle(),
        pTargetType->GetName(),
        strValue,
        &sConvertedValue));

    IFC(CValueBoxer::UnboxObjectValue(&sConvertedValue, pTargetType, ppTargetValue));

Cleanup:

    RRETURN(hr);

}
_Check_return_
HRESULT
CoreValueConverter::CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter)
{
    HRESULT hr = S_OK;

    *ppNewConverter = new CoreValueConverter();

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_
HRESULT
StringValueConverter::Convert(
   _In_ IInspectable *pSource,
   _In_ const CClassInfo *pTargetType,
   _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
   _Outptr_ IInspectable **ppTargetValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strObjectString;
    xaml_data::ICustomPropertyProvider *pProvider = NULL;

    // Attempt to get a string representation of the
    // object, using either the ICPP interface if present
    // or the type as a fallback
    pProvider = ctl::query_interface<xaml_data::ICustomPropertyProvider>(pSource);
    if (pProvider != NULL)
    {
        IFC(pProvider->GetStringRepresentation(strObjectString.GetAddressOf()));
    }
    else
    {
        IFC(pSource->GetRuntimeClassName(strObjectString.GetAddressOf()));
    }

    IFC(StaticStore::GetPropertyValueStaticsNoRef()->CreateString(strObjectString.Get(), ppTargetValue));

Cleanup:

    ReleaseInterface(pProvider);

    RRETURN(hr);
}

_Check_return_
HRESULT
StringValueConverter::CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter)
{
    HRESULT hr = S_OK;

    *ppNewConverter = new StringValueConverter();

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_ HRESULT
ValueConversionHelpers::ConvertGuidToString(
    _In_ GUID guid,
    _Out_ HSTRING *phstr)
{
    HRESULT hr = S_OK;
    LPOLESTR pGuidString = NULL;

    IFC(StringFromCLSID(guid, &pGuidString));

    IFC(wrl_wrappers::HStringReference(pGuidString, wcslen(pGuidString)).CopyTo(phstr));

Cleanup:
    CoTaskMemFree(pGuidString);
    RRETURN(hr);
}

_Check_return_ HRESULT
ValueConversionHelpers::ConvertStringToGuid(
    _In_ HSTRING phstr,
    _Out_ GUID *pguid)
{
    HRESULT hr = S_OK;

    IFC(CLSIDFromString(WindowsGetStringRawBuffer(phstr, NULL), pguid));

Cleanup:
    RRETURN(hr);
}

bool ValueConversionHelpers::CanConvertValueToString(wf::PropertyType propertyType)
{
    switch (propertyType)
    {
        // Keep this in sync with ConvertValueToString().

        case wf::PropertyType_UInt8:
        case wf::PropertyType_Int16:
        case wf::PropertyType_UInt16:
        case wf::PropertyType_Int32:
        case wf::PropertyType_UInt32:
        case wf::PropertyType_Int64:
        case wf::PropertyType_UInt64:
        case wf::PropertyType_Single:
        case wf::PropertyType_Double:
        case wf::PropertyType_Char16:
        case wf::PropertyType_Boolean:
        case wf::PropertyType_String:
        case wf::PropertyType_Guid:
            return true;

        default:
            return false;
    }
}

_Check_return_ HRESULT
ValueConversionHelpers::ConvertValueToString(
    _In_ wf::IPropertyValue* pPropertyValue,
    _In_ wf::PropertyType propertyType,
    _Out_ HSTRING *phstr)
{
    HRESULT hr = S_OK;
    WCHAR szValue[50];
    BOOLEAN createString = TRUE;

    szValue[0] = L'\0';

    *phstr = NULL;

    switch (propertyType)
    {
        // Keep this in sync with CanConvertValueToString().

       case wf::PropertyType_UInt8:
       {
           BYTE value = 0;
           XUINT32 val32 = 0;

           IFC(pPropertyValue->GetUInt8(&value));
           val32 = value;
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%u", val32) > 0);
           break;
       }

       case wf::PropertyType_Int16:
       {
           INT16 value = 0;
           IFC(pPropertyValue->GetInt16(&value));
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%hd", value) > 0);
           break;
       }

       case wf::PropertyType_UInt16:
       {
           UINT16 value = 0;
           IFC(pPropertyValue->GetUInt16(&value));
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%hu", value) > 0);
           break;
       }

       case wf::PropertyType_Int32:
       {
           INT32 value = 0;
           IFC(pPropertyValue->GetInt32(&value));
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%I32d", value) > 0);
           break;
       }

       case wf::PropertyType_UInt32:
       {
           UINT32 value = 0;
           IFC(pPropertyValue->GetUInt32(&value));
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%I32u", value) > 0);
           break;
       }

       case wf::PropertyType_Int64:
       {
           INT64 value = 0;
           IFC(pPropertyValue->GetInt64(&value));
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%I64d", value) > 0);
           break;
       }

       case wf::PropertyType_UInt64:
       {
           UINT64 value = 0;
           IFC(pPropertyValue->GetUInt64(&value));
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%I64u", value) > 0);
           break;
       }

       case wf::PropertyType_Single:
       {
           FLOAT value = 0;
           IFC(pPropertyValue->GetSingle(&value));
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%g", value) > 0);
           break;
       }

       case wf::PropertyType_Double:
       {
           DOUBLE value = 0;
           IFC(pPropertyValue->GetDouble(&value));
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%lg", value) > 0);
           break;
       }

       case wf::PropertyType_Char16:
       {
           WCHAR value = 0;
           IFC(pPropertyValue->GetChar16(&value));
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%c", value) > 0);
           break;
       }

       case wf::PropertyType_Boolean:
       {
           BOOLEAN value = FALSE;
           XUINT32 val32 = 0;

           IFC(pPropertyValue->GetBoolean(&value));
           val32 = value;
           IFCCHECK(swprintf_s(szValue, ARRAYSIZE(szValue), L"%u", val32) > 0);
           break;
       }

       case wf::PropertyType_String:
       {
           IFC(pPropertyValue->GetString(phstr));
           createString = FALSE;
           break;
       }

       case wf::PropertyType_Guid:
       {
           GUID value;

           IFC(pPropertyValue->GetGuid(&value));
           IFC(ConvertGuidToString(value, phstr));
           createString = FALSE;
           break;
       }

       default:
       {
           IFC(E_FAIL);
           break;
       }
    }

    if (createString)
    {
        IFC(wrl_wrappers::HStringReference(szValue, wcslen(szValue)).CopyTo(phstr));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ValueConversionHelpers::ConvertStringToValue(
    _In_ HSTRING hstr,
    _In_ wf::PropertyType propertyType,
    _Out_ IInspectable** ppPropertyValue)
{
    HRESULT hr = S_OK;
    PCWSTR pszValue = NULL;
    IInspectable* pPropertyValue = NULL;

    *ppPropertyValue = NULL;

    pszValue = WindowsGetStringRawBuffer(hstr, NULL);

    switch (propertyType)
    {
        case wf::PropertyType_UInt8:
        {
            UINT32 value = 0;
            IFCCHECK(swscanf_s(pszValue, L"%u", &value) > 0);
            ASSERT(value <= UINT8_MAX);
            IFC(DirectUI::PropertyValue::CreateFromUInt8(static_cast<UINT8>(value), &pPropertyValue));
            break;
        }

        case wf::PropertyType_Int16:
        {
            INT16 value = 0;
            IFCCHECK(swscanf_s(pszValue, L"%hd", &value) > 0);
            IFC(DirectUI::PropertyValue::CreateFromInt16(value, &pPropertyValue));
            break;
        }

        case wf::PropertyType_UInt16:
        {
            UINT16 value = 0;
            IFCCHECK(swscanf_s(pszValue, L"%hu", &value) > 0);
            IFC(DirectUI::PropertyValue::CreateFromUInt16(value, &pPropertyValue));
            break;
        }

        case wf::PropertyType_Int32:
        {
            INT32 value = 0;
            IFCCHECK(swscanf_s(pszValue, L"%I32d", &value) > 0);
            IFC(DirectUI::PropertyValue::CreateFromInt32(value, &pPropertyValue));
            break;
        }

        case wf::PropertyType_UInt32:
        {
            UINT32 value = 0;
            IFCCHECK(swscanf_s(pszValue, L"%I32u", &value) > 0);
            IFC(DirectUI::PropertyValue::CreateFromUInt32(value, &pPropertyValue));
            break;
        }

        case wf::PropertyType_Int64:
        {
            INT64 value = 0;
            IFCCHECK(swscanf_s(pszValue, L"%I64d", &value) > 0);
            IFC(DirectUI::PropertyValue::CreateFromInt64(value, &pPropertyValue));
            break;
        }

        case wf::PropertyType_UInt64:
        {
            UINT64 value = 0;
            IFCCHECK(swscanf_s(pszValue, L"%I64u", &value) > 0);
            IFC(DirectUI::PropertyValue::CreateFromUInt64(value, &pPropertyValue));
            break;
        }

        case wf::PropertyType_Single:
        {
            WCHAR* pszValueSuffix = nullptr;
            FLOAT value = static_cast<FLOAT>(wcstod(pszValue, &pszValueSuffix));
            IFCCHECK(0.0f != value || pszValue != pszValueSuffix);
            IFC(DirectUI::PropertyValue::CreateFromSingle(
                value,
                &pPropertyValue));
            break;
        }

        case wf::PropertyType_Double:
        {
            WCHAR* pszValueSuffix = nullptr;
            DOUBLE value = wcstod(pszValue, &pszValueSuffix);
            IFCCHECK(0.0 != value || pszValue != pszValueSuffix);
            IFC(DirectUI::PropertyValue::CreateFromDouble(
                value,
                &pPropertyValue));
            break;
        }

        case wf::PropertyType_Char16:
        {
            WCHAR value = 0;
            IFCCHECK(swscanf_s(pszValue, L"%c", &value, 1) > 0);
            IFC(DirectUI::PropertyValue::CreateFromChar16(value, &pPropertyValue));
            break;
        }

        case wf::PropertyType_Boolean:
        {
            UINT32 value = 0;
            IFCCHECK(swscanf_s(pszValue, L"%u", &value) > 0);
            IFC(DirectUI::PropertyValue::CreateFromBoolean(!!value, &pPropertyValue));
            break;
        }

        case wf::PropertyType_String:
        {
            IFC(DirectUI::PropertyValue::CreateFromString(hstr, &pPropertyValue));
            break;
        }

        case wf::PropertyType_Guid:
        {
            GUID value;
            IFC(ConvertStringToGuid(hstr, &value));
            IFC(DirectUI::PropertyValue::CreateFromGuid(value, &pPropertyValue));
            break;
        }

        default:
        {
            IFC(E_FAIL);
            break;
        }
    }

    *ppPropertyValue = pPropertyValue;
    pPropertyValue = NULL;

Cleanup:
    ReleaseInterface(pPropertyValue);
    RRETURN(hr);
}

