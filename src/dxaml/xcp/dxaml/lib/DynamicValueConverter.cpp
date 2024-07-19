// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DynamicValueConverter.h"

using namespace DirectUI;

_Check_return_
HRESULT
DynamicValueConverter::CreateConverter(_Outptr_ IValueConverterInternal **ppConverter)
{
    HRESULT hr = S_OK;
    IValueConverterInternal *pNewConverter = NULL;

    IFCPTR(ppConverter);
    *ppConverter = NULL;

    pNewConverter = new DynamicValueConverter();

    *ppConverter = pNewConverter;
    pNewConverter = NULL;

Cleanup:
    ReleaseInterface(pNewConverter);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   Convert
//
//  Synopsis: Converts source value to the target's type value.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
DynamicValueConverter::Convert(
    _In_ IInspectable *pSource,
    _In_ const CClassInfo *pTargetType,
    _In_opt_ IUnknown * pConverterParameter,
    //_In_opt_ CultureInfo culture,
    _Outptr_ IInspectable **ppTargetValue)
{
    HRESULT hr = S_OK;
    const CClassInfo* pSourceType = nullptr;

    IFCPTR(ppTargetValue);
    if (!PropertyValue::IsNullOrEmpty(pSource))
    {
        bool bIsInstanceOfType = false;
        IFC(MetadataAPI::IsInstanceOfType(pSource, pTargetType, &bIsInstanceOfType));
        if (bIsInstanceOfType)
        {
            *ppTargetValue = pSource;
            AddRefInterface(pSource);
        }
        else
        {
            IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(pSource, &pSourceType));

            // Convert the value if we can
            IFC(EnsureConverter(pSourceType, pTargetType));
            if (!m_pConverter)
            {
                // This failure is handled by the caller (see BindingExpression::ConvertToTarget).
                IFC_NOTRACE(E_FAIL);
            }

            IFC_NOTRACE(m_pConverter->Convert(pSource, pTargetType, pConverterParameter/*, culture*/, ppTargetValue));
        }
    }
    else
    {
        if (!pTargetType->IsValueType())
        {
            *ppTargetValue = pSource;
            AddRefInterface(pSource);
        }
        else
        {
            IFC(E_FAIL);
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ConvertBack
//
//  Synopsis: Converts target value back to the source's type value.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
DynamicValueConverter::ConvertBack(
    _In_ IInspectable *pTarget,
    _In_ const CClassInfo *pSourceType,
    _In_opt_ IUnknown * pConverterParameter,
    //_In_opt_ CultureInfo culture,
    _Outptr_ IInspectable **ppSourceValue)
{
    HRESULT hr = S_OK;
    const CClassInfo* pTargetType = nullptr;

    IFCPTR(ppSourceValue);

    if (!PropertyValue::IsNullOrEmpty(pTarget))
    {
        // Use the type of the value to establish the converter.
        IFC(MetadataAPI::GetClassInfoFromObject_ResolveWinRTPropertyOtherType(pTarget, &pTargetType));
        if (MetadataAPI::IsAssignableFrom(pSourceType, pTargetType))
        {
            *ppSourceValue = pTarget;
            AddRefInterface(pTarget);
        }
        else
        {
            // Convert the value if we can
            IFC(EnsureConverter(pSourceType, pTargetType));
            if (!m_pConverter)
            {
                IFC(E_FAIL);
            }

            IFC(m_pConverter->ConvertBack(pTarget, pSourceType, pConverterParameter/*, culture*/, ppSourceValue));
        }
    }
    else
    {
        // Null is ok as long as the target type accepts null
        if (!pSourceType->IsValueType())
        {
            *ppSourceValue = pTarget;
            AddRefInterface(pTarget);
        }
        else
        {
            // We can't convert NULL to a value type
            IFC(E_FAIL);
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   EnsureConverter
//
//  Synopsis: Creates the appropriate converter for the source & target types
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
DynamicValueConverter::EnsureConverter(
    _In_ const CClassInfo *pSourceType,
    _In_ const CClassInfo *pTargetType)
{
    HRESULT hr = S_OK;

    // See if our cached converter is OK.
    if ((m_pSourceType != pSourceType) || (m_pTargetType != pTargetType))
    {
        // Types have changed - get a new converter
        ReleaseInterface(m_pConverter);
        IFC(DefaultValueConverter::CreateConverter(pSourceType, pTargetType, &m_pConverter));

        m_pSourceType = pSourceType;
        m_pTargetType = pTargetType;
    }

Cleanup:
    RRETURN(hr);
}
