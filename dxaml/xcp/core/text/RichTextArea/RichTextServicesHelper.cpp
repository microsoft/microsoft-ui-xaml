// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace RichTextServices;

//------------------------------------------------------------------------
//  Summary:
//      Maps RichTextServices Result::Enum codes to HRESULT
//------------------------------------------------------------------------
#pragma warning (push)
// Description: Disable prefast warning 26020 : Error in annotation at d:\dev11\sl2\xcp\core\inc\richtextserviceshelper.h(46) on 'Enum' :no parameter named RichTextServices.
// Reason     : Per discussion with Prefast team, this annotation usage is correct but cannot be processed by OACR's parser. Upgrade to new version of OACR should fix this.
//              Tracked in bug 92086.
#pragma warning (disable : 26020)
_Check_return_ HRESULT RichTextServicesHelper::MapTxErr(_In_ Result::Enum txhr)
{
    HRESULT hr = E_UNEXPECTED;

    switch (txhr)
    {
        case Result::Success:
            hr = S_OK;
            break;

        case Result::OutOfMemory:
            hr = E_OUTOFMEMORY;
            break;

        case Result::Unexpected:
            hr = E_UNEXPECTED;
            break;

        case Result::InvalidParameter:
            hr = E_INVALIDARG;
            break;

        case Result::FormattingError:
            hr = E_FAIL;
            break;

        case Result::NotImplemented:
            hr = E_NOTIMPL;
            break;

        case Result::InvalidOperation:
            hr = E_FAIL;
            break;

        case Result::InternalError:
            hr = INTERNAL_ERROR;
            break;
    }

    return hr;
}
#pragma warning (pop)



//------------------------------------------------------------------------
//  Summary:
//      Maps xcp FlowDirection to RichTextServices FlowDirection
//------------------------------------------------------------------------
_Check_return_ RichTextServices::FlowDirection::Enum RichTextServicesHelper::XCPFlowDirectionToRTS(
    _In_ DirectUI::FlowDirection nFlowDirection
)
{
    // We rely on the flowdirection numeric value being the same as the
    // Unicode defined values for base bidirectional levels of LTR and
    // RTL.
    // i.e. LTR must be 0, and RTL must be 1.
    // The RichTextServices enum, the Silverlight enum, and the DWRITE
    // enum all conform to this standard.
    // Use a compile time check to confirm this here before mapping Silverlight
    // FlowDirection to RichTextServices using a simple typecast.
    STATIC_ASSERT(DirectUI::FlowDirection::LeftToRight == static_cast<DirectUI::FlowDirection>(0), LeftToRightMustBeZero);
    STATIC_ASSERT(DirectUI::FlowDirection::RightToLeft == static_cast<DirectUI::FlowDirection>(1), RightToLeftMustBeOne);
    STATIC_ASSERT(RichTextServices::FlowDirection::LeftToRight == 0, LeftToRightMustBeZero);
    STATIC_ASSERT(RichTextServices::FlowDirection::RightToLeft == 1, RightToLeftMustBeOne);
    return (RichTextServices::FlowDirection::Enum)nFlowDirection;
}

