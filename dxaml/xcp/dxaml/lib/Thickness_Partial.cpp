// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Thickness.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a Thickness from the specified lengths.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
ThicknessFactory::FromLengths(
    _In_ DOUBLE left,
    _In_ DOUBLE top,
    _In_ DOUBLE right,
    _In_ DOUBLE bottom,
    _Out_ xaml::Thickness* pReturnValue)
{

    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    pReturnValue->Left = left;
    pReturnValue->Top = top;
    pReturnValue->Right = right;
    pReturnValue->Bottom = bottom;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a Thickness from the specified uniform length.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
ThicknessFactory::FromUniformLength(
    _In_ DOUBLE uniformLength,
    _Out_ xaml::Thickness* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    pReturnValue->Left = uniformLength;
    pReturnValue->Top = uniformLength;
    pReturnValue->Right = uniformLength;
    pReturnValue->Bottom = uniformLength;

Cleanup:
    RRETURN(hr);
}

