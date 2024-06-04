// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SizeHelper.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a Size from the specified dimensions.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
SizeHelperFactory::FromDimensionsImpl(
    _In_ FLOAT width,
    _In_ FLOAT height,
    _Out_ wf::Size* pReturnValue)
{
    if (width < 0.0F || height < 0.0F)
    {
        return E_INVALIDARG;
    }

    pReturnValue->Width = width;
    pReturnValue->Height = height;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a size that represents an Empty size. Width and Height
//      are NegativeInfinity.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT 
SizeHelperFactory::get_EmptyImpl(_Out_ wf::Size* pValue)
{
    pValue->Height = static_cast<FLOAT>(DoubleUtil::NegativeInfinity);
    pValue->Width = static_cast<FLOAT>(DoubleUtil::NegativeInfinity);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this size is the Empty size.
//
//  Notes:
//      If size is 0 this Size still contains a 0 or 1 dimensional set
//      of points, so this method should not be used for 0 area.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT 
SizeHelperFactory::GetIsEmptyImpl(
    _In_ wf::Size target,
    _Out_ BOOLEAN* pValue)
{
    *pValue = (target.Width < 0.0F || target.Height < 0.0F);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares two Size structs for equality.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT 
SizeHelperFactory::EqualsImpl(
    _In_ wf::Size target,
    _In_ wf::Size value,
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isEmpty = FALSE;

    IFC(GetIsEmpty(target, &isEmpty));
    if (isEmpty)
    {
        IFC(GetIsEmpty(value, &isEmpty));
        *pReturnValue = isEmpty;
    }
    else
    {
        *pReturnValue = (target.Width == value.Width && target.Height == value.Height);
    }

Cleanup:
    RRETURN(hr);
}

