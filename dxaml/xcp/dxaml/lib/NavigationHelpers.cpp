// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "NavigationHelpers.h"
#include "NavigationEventArgs.g.h"
#include "NavigatingCancelEventArgs.g.h"
#include "DefaultValueConverter.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
NavigationHelpers::CreateINavigationEventArgs(
    _In_ IInspectable *pContentIInspectable,
    _In_opt_ IInspectable *pParameterIInspectable,
    _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo, 
    _In_ HSTRING descriptor,
    _In_ xaml::Navigation::NavigationMode navigationMode,
    _Outptr_ xaml::Navigation::INavigationEventArgs **ppINavigationEventArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<NavigationEventArgs> spNavigationEventArgs;
    wxaml_interop::TypeName sourcePageType = {};

    IFCPTR(ppINavigationEventArgs);
    *ppINavigationEventArgs = NULL;

    IFCPTR(descriptor);

    IFC(ctl::make(&spNavigationEventArgs));
    IFC(MetadataAPI::GetTypeNameByFullName(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(descriptor), &sourcePageType));

    // All properties can be null.
    IFC(spNavigationEventArgs->put_SourcePageType(sourcePageType));
    IFC(spNavigationEventArgs->put_Content(pContentIInspectable));
    IFC(spNavigationEventArgs->put_NavigationMode(navigationMode));
    IFC(spNavigationEventArgs->put_Parameter(pParameterIInspectable));
    IFC(spNavigationEventArgs->put_NavigationTransitionInfo(pTransitionInfo));

    *ppINavigationEventArgs = spNavigationEventArgs.Detach();

Cleanup:
    DELETE_STRING(sourcePageType.Name);
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHelpers::CreateINavigatingCancelEventArgs(
    _In_opt_ IInspectable *pParameterIInspectable, 
    _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo, 
    _In_ HSTRING descriptor,
    _In_ xaml::Navigation::NavigationMode navigationMode,
    _Outptr_ xaml::Navigation::INavigatingCancelEventArgs **ppINavigatingCancelEventArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<NavigatingCancelEventArgs> spNavigatingCancelEventArgs;
    wxaml_interop::TypeName sourcePageType = {};

    IFCPTR(ppINavigatingCancelEventArgs);
    *ppINavigatingCancelEventArgs = NULL;

    IFCPTR(descriptor);

    IFC(ctl::make(&spNavigatingCancelEventArgs));

    IFC(MetadataAPI::GetTypeNameByFullName(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(descriptor), &sourcePageType));

    // All properties can be null.
    IFC(spNavigatingCancelEventArgs->put_SourcePageType(sourcePageType));
    IFC(spNavigatingCancelEventArgs->put_NavigationMode(navigationMode));
    IFC(spNavigatingCancelEventArgs->put_Parameter(pParameterIInspectable));
    IFC(spNavigatingCancelEventArgs->put_NavigationTransitionInfo(pTransitionInfo));

    *ppINavigatingCancelEventArgs = spNavigatingCancelEventArgs.Detach();

Cleanup:
    DELETE_STRING(sourcePageType.Name);
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHelpers::WriteUINT32ToString(
    _In_ UINT32 value, 
    _Inout_ string &buffer)
{
    HRESULT hr = S_OK;
    VARIANT src;
    VARIANT dest;

    VariantInit(&src);
    VariantInit(&dest);

    // Convert UINT32 to String
    V_VT(&src) = VT_UI4;
    V_UI4(&src) = value;
    IFC(VariantChangeType(&dest, &src, 0, VT_BSTR));

    // Format: <UINT32>,
    buffer.append(V_BSTR(&dest));
    buffer.append(L",");

Cleanup:
    VariantClear(&src);
    VariantClear(&dest);
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHelpers::ReadUINT32FromString(
    _In_ string &buffer,    
    size_t currentPosition,    
    _Out_ UINT32* pValue,
    _Out_ size_t* pNextPosition)     
{
    HRESULT hr = S_OK;
    VARIANT src;
    VARIANT dest;
    string subString;
    BSTR bstrValue = NULL;

    VariantInit(&src);
    VariantInit(&dest);

    *pValue = 0;

    // Read next substring 
    IFC(NavigationHelpers::ReadNextSubString(buffer, currentPosition, subString, pNextPosition)); 

    // Convert substring to UINT32
    bstrValue = SysAllocString(subString.c_str());
    IFCOOMFAILFAST(bstrValue);
    V_VT(&src) = VT_BSTR;
    V_BSTR(&src) = bstrValue;
    bstrValue = NULL;
    IFC(VariantChangeType(&dest, &src, 0, VT_UI4));
    *pValue = V_UI4(&dest);    
            
Cleanup:
    VariantClear(&src);
    VariantClear(&dest);
    SysFreeString(bstrValue);
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHelpers::WriteHSTRINGToString(
    _In_opt_ HSTRING hstr,
    _Inout_ string &buffer)
{
    LPCWSTR psz = NULL;
    XUINT32 length = 0;

    // Write <length>,<string>, if a string was provided
    // Write 0, if string is empty or NULL. Empty strings are read back as NULL.
    if (hstr)
    {
        psz = HStringUtil::GetRawBuffer(hstr, &length);
        IFC_RETURN(NavigationHelpers::WriteUINT32ToString(length, buffer));
        if (length > 0)
        {
            buffer.append(psz);
            buffer.append(L",");
        }
    }
    else
    {       
        IFC_RETURN(NavigationHelpers::WriteUINT32ToString(0, buffer));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: NavigationHelpers::WriteNavigationParameterToString
//
//  Synopsis:    
//     Serialize NavigationParameter into string if conversion is supported
//  Serialization Format: 
//      NULL param or parameter type not supported: <wf::PropertyType_Empty>,
//      Non-NULL param: <parameter type (wf::PropertyType)>,<length of serialized parameter>,<serialized parameter>,
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHelpers::WriteNavigationParameterToString(     
    _In_opt_ IInspectable* pNavigationParameter,
    _Inout_ string &buffer,
    _Out_ BOOLEAN *pIsParameterTypeSupported)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strPropertyValue;
    wf::PropertyType propertyType = wf::PropertyType_Empty;
    BOOLEAN isParameterTypeSupported = FALSE;

    *pIsParameterTypeSupported = FALSE;
    
    if (pNavigationParameter)
    {   
        IFC(NavigationHelpers::ConvertNavigationParameterToHSTRING(
            pNavigationParameter,
            strPropertyValue.GetAddressOf(), 
            &propertyType, 
            &isParameterTypeSupported));

        if (!isParameterTypeSupported)
        {
            propertyType = wf::PropertyType_Empty;
        }
    }
    else
    {
        // NULL parameters are supported
        isParameterTypeSupported = TRUE;
        propertyType = wf::PropertyType_Empty;
    }

    // Write property type. wf::PropertyType_Empty will be written if 
    // property value was not provided or parameted serialization is not supported
    IFC(NavigationHelpers::WriteUINT32ToString(propertyType, buffer));

    // Write property value as a string. 
    if (propertyType != wf::PropertyType_Empty)
    {
        IFC(NavigationHelpers::WriteHSTRINGToString(strPropertyValue.Get(), buffer));  
    }

     *pIsParameterTypeSupported = isParameterTypeSupported;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: NavigationHelpers::ReadNavigationParameterFromString
//
//  Synopsis:    
//     Create NavigationParameter from serialized string
//  Serialization Format: 
//      NULL param or parameter type not supported: <wf::PropertyType_Empty>,
//      Non-NULL param: <parameter type (wf::PropertyType)>,<length of serialized parameter>,<serialized parameter>,
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHelpers::ReadNavigationParameterFromString(
    _In_ string &buffer,    
    size_t currentPosition,    
    _Out_ IInspectable** ppNavigationParameter,
    _Out_ size_t* pNextPosition)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strPropertyValue;
    wf::PropertyType propertyType = wf::PropertyType_Empty;
    IInspectable* pNavigationParameter = NULL;
    UINT32 value = 0;
    BOOLEAN isParameterTypeSupported = FALSE;

    *ppNavigationParameter = NULL;

    // Read parameter's property type
    IFC(NavigationHelpers::ReadUINT32FromString(buffer, currentPosition, 
            &value, pNextPosition));
    currentPosition = *pNextPosition;
    propertyType = static_cast<wf::PropertyType>(value);

    if (propertyType == wf::PropertyType_Empty)
    {
        // NULL parameter or parameter serialization is not supported
        goto Cleanup;
    }
    
    // Read parameter's serialized property value 
    IFC(NavigationHelpers::ReadHSTRINGFromString(
        buffer, 
        currentPosition, 
        strPropertyValue.GetAddressOf(), 
        pNextPosition));
    currentPosition = *pNextPosition;

    // Create NavigationParameter from serialized state in HSTRING
    IFC(ConvertHSTRINGToNavigationParameter(strPropertyValue.Get(), propertyType, &pNavigationParameter, &isParameterTypeSupported));
    IFCCHECK(isParameterTypeSupported);

    *ppNavigationParameter = pNavigationParameter;
    pNavigationParameter = NULL;
    
Cleanup:
    ReleaseInterface(pNavigationParameter);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: NavigationHelpers::ReadHSTRINGFromString
//
//  Synopsis:    
//     Return HSTRING. Can return NULL if NULL or empty HSTRING was stored.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHelpers::ReadHSTRINGFromString(
    _In_ string &buffer,    
    size_t currentPosition,    
    _Out_ HSTRING* phstr,
    _Out_ size_t* pNextPosition)
{   
    HRESULT hr = S_OK;
    size_t nextPosition = string::npos;
    UINT32 subStringLength = 0;
    string subString;

    *phstr = NULL;
    
    // Read length 
    IFC(NavigationHelpers::ReadUINT32FromString(buffer, currentPosition, &subStringLength, &nextPosition));
    currentPosition = nextPosition;

    // Read sub string if it is not a NULL string
    if (subStringLength)
    {
        IFC(NavigationHelpers::ReadNextSubString(buffer, currentPosition, subStringLength, subString, &nextPosition));
        currentPosition = nextPosition;
        
        IFC(::WindowsCreateString(subString.c_str(), subString.length(), phstr));
    }
    
    *pNextPosition = nextPosition;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: NavigationHelpers::ReadNextSubString
//
//  Synopsis:    
//     Return substring from current position of given string to next 
//  delimiter. If no delimiter is found, read from current position
//  to end of given string.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHelpers::ReadNextSubString(
    _In_ string &buffer,    
    size_t currentPosition,    
    _Inout_ string &subString,
    _Out_ size_t* pNextPosition)
{
    HRESULT hr = S_OK;
    size_t bufferLength = 0;
    size_t delimiterPosition = 0;

    // Reached end of string?
    IFCCHECK(currentPosition != string::npos);

    bufferLength = buffer.length();
    IFCCHECK((bufferLength > 0) && (currentPosition < bufferLength));

    // Find ',' delimiter after the substring to be read
    delimiterPosition = buffer.find(L",", currentPosition);
    if (delimiterPosition == string::npos)
    {
        // Delimiter not found. Use string's terminator as delimiter.
        delimiterPosition = bufferLength;
    }
    IFCCHECK(delimiterPosition > currentPosition);

    // Get substring
    IFC(ReadNextSubString(buffer, currentPosition, delimiterPosition - currentPosition, subString, pNextPosition)); 
            
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: NavigationHelpers::ReadNextSubString
//
//  Synopsis:    
//     Given the length of a substring, return substring from current position
//  of given string.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHelpers::ReadNextSubString(
    _In_ string &buffer,    
    size_t currentPosition,
    size_t subStringLength,
    _Inout_ string &subString,
    _Out_ size_t* pNextPosition)
{
    HRESULT hr = S_OK;
    size_t bufferLength = 0;
    size_t nextPosition = string::npos;

    // Reached end of string?
    ASSERT(currentPosition != string::npos);
    IFCCHECK(currentPosition != string::npos);

    bufferLength = buffer.length();
    IFCCHECK((bufferLength > 0) && ((currentPosition + subStringLength) <= bufferLength));

    // Get substring 
    subString = buffer.substr(currentPosition, subStringLength);
    
    // Skip over delimiter, and adjust for length of string
    nextPosition = currentPosition + subStringLength + 1;
    if (nextPosition >= bufferLength)
    {
        // End of string
        *pNextPosition = string::npos;
    }
    else
    {
        *pNextPosition = nextPosition;
    }
            
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: NavigationHelpers::ConvertNavigationParameterToHSTRING
//
//  Synopsis:    
//     Convert NavigationParameter to a HSTRING. String, char, numeric and
//  GUID property values are supported. Return the property type of
//  the NavigationParameter and indicates if the conversion is supported. 
//  Returns NULL HSTRING if conversion is not supported.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHelpers::ConvertNavigationParameterToHSTRING(
    _In_ IInspectable *pNavigationParameter, 
    _Out_ HSTRING *phstr,
    _Out_  wf::PropertyType *pParameterType,            
    _Out_ BOOLEAN *pIsConversionSupported)
{    
    HRESULT hr = S_OK;
    wf::PropertyType propertyType = wf::PropertyType_Empty;
    BOOLEAN supportedType = TRUE;
    wrl_wrappers::HString strValue;
    wf::IPropertyValue* pPropertyValue = NULL;

    *pParameterType = wf::PropertyType_Empty;
    *pIsConversionSupported = FALSE;
    *phstr = NULL;

    // Only Navigation Parameter which is a IPropertyValue can be converted
    pPropertyValue = ctl::get_property_value(pNavigationParameter);
    if (!pPropertyValue)
    {
        goto Cleanup;
    }

    IFC(pPropertyValue->get_Type(&propertyType));

    if (ValueConversionHelpers::CanConvertValueToString(propertyType))
    {
        IFC(ValueConversionHelpers::ConvertValueToString(pPropertyValue, propertyType, strValue.GetAddressOf()));
    }
    else
    {
        supportedType = FALSE;
    }

    *pIsConversionSupported = supportedType;
    *pParameterType = propertyType;
    *phstr = strValue.Detach();

Cleanup:
    ReleaseInterface(pPropertyValue);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: NavigationHelpers::ConvertHSTRINGToNavigationParameter
//
//  Synopsis:    
//     Use serialized parameter value from HSTRING and parameter type to 
//  re-create NavigationParameter. Indicates if conversion is supported.
//  Returns NULL NavigationParameter if conversion is not supported.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHelpers::ConvertHSTRINGToNavigationParameter(
    _In_ HSTRING hstr,
    _In_ wf::PropertyType parameterType,
    _Out_ IInspectable** ppNavigationParameter,
    _Out_ BOOLEAN *pIsConversionSupported)
{
    HRESULT hr = S_OK;
    BOOLEAN supportedType = TRUE;
    IInspectable* pNavigationParameter = NULL;

    *pIsConversionSupported = FALSE;
    *ppNavigationParameter = NULL;

    switch (parameterType)
    {            
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
        {
            IFC(ValueConversionHelpers::ConvertStringToValue(hstr, parameterType, &pNavigationParameter));
            break;
        }     

        default:
        {
            supportedType = FALSE;
            break;
        }
    }
    
    *pIsConversionSupported = supportedType;
    *ppNavigationParameter = pNavigationParameter;
    pNavigationParameter = NULL;
    
Cleanup:
    ReleaseInterface(pNavigationParameter);
    RRETURN(hr);
}

