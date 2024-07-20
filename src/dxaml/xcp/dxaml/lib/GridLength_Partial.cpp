// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GridLength.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares two GridLengths for equality.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
GridLengthFactory::Equals(
    _In_ xaml::GridLength target,
    _In_ xaml::GridLength value,
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (target.GridUnitType == value.GridUnitType)
    {
        if (value.GridUnitType == xaml::GridUnitType_Auto)
        {
            *pReturnValue = TRUE;
        }
        else
        {
            *pReturnValue = (target.Value == value.Value);
        }
    }
    else
    {
        *pReturnValue = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a GridLength from the specified pixel length.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
GridLengthFactory::FromPixels(
    _In_ DOUBLE pixels,
    _Out_ xaml::GridLength* pReturnValue)
{
    RRETURN(FromValueAndType(pixels, xaml::GridUnitType_Pixel, pReturnValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a GridLength from the specified pixel length.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
GridLengthFactory::FromValueAndType(
    _In_ DOUBLE value,
    _In_ xaml::GridUnitType type,
    _Out_ xaml::GridLength* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (DoubleUtil::IsNaN(value) || DoubleUtil::IsInfinity(value) || value < 0.0 ||
        (type != xaml::GridUnitType_Auto &&
         type != xaml::GridUnitType_Pixel &&
         type != xaml::GridUnitType_Star))
    {
        RRETURN(E_INVALIDARG);
    }

    pReturnValue->Value = (type == xaml::GridUnitType_Auto) ? 1.0 : value;
    pReturnValue->GridUnitType = type;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns an initialized automatic GridLength.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
GridLengthFactory::get_Auto(_Out_ xaml::GridLength* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    pValue->Value = 1.0;
    pValue->GridUnitType = xaml::GridUnitType_Auto;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this GridLength holds an absolute (pixel) value.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
GridLengthFactory::GetIsAbsolute(_In_ xaml::GridLength target, _Out_ BOOLEAN* pValue)

{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = (target.GridUnitType == xaml::GridUnitType_Pixel);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this GridLength is automatic.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
GridLengthFactory::GetIsAuto(_In_ xaml::GridLength target, _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = (target.GridUnitType == xaml::GridUnitType_Auto);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this GridLength holds a weighted proportion
//      of available space.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
GridLengthFactory::GetIsStar(_In_ xaml::GridLength target, _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = (target.GridUnitType == xaml::GridUnitType_Star);

Cleanup:
    RRETURN(hr);
}

