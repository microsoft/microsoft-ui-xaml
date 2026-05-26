// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CornerRadius.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a CornerRadius from the specified radii.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
CornerRadiusFactory::FromRadii(
    DOUBLE topLeft,
    DOUBLE topRight,
    DOUBLE bottomRight,
    DOUBLE bottomLeft,
    _Out_ xaml::CornerRadius* pReturnValue)
{

    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (topLeft < 0.0 || DoubleUtil::IsNaN(topLeft) ||
        topRight < 0.0 || DoubleUtil::IsNaN(topRight) ||
        bottomRight < 0.0 || DoubleUtil::IsNaN(bottomRight) ||
        bottomLeft < 0.0 || DoubleUtil::IsNaN(bottomLeft))
    {
        RRETURN(E_INVALIDARG);
    }

    pReturnValue->TopLeft = topLeft;
    pReturnValue->TopRight = topRight;
    pReturnValue->BottomRight = bottomRight;
    pReturnValue->BottomLeft = bottomLeft;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a CornerRadius from the specified uniform radius.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
CornerRadiusFactory::FromUniformRadius(
    DOUBLE uniformRadius,
    _Out_ xaml::CornerRadius* pReturnValue)
{
    RRETURN(FromRadii(uniformRadius, uniformRadius, uniformRadius, uniformRadius, pReturnValue));
}

