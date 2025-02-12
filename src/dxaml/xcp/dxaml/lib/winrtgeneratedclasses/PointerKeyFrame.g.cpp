// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#include "PointerKeyFrame.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::PointerKeyFrame::PointerKeyFrame()
{
}

DirectUI::PointerKeyFrame::~PointerKeyFrame()
{
}

HRESULT DirectUI::PointerKeyFrame::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PointerKeyFrame)))
    {
        *ppObject = static_cast<DirectUI::PointerKeyFrame*>(this);
    }
    else
    {
        RRETURN(DirectUI::DependencyObject::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
_Check_return_ HRESULT DirectUI::PointerKeyFrame::get_PointerValue(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PointerKeyFrame_PointerValue, pValue));
}
_Check_return_ HRESULT DirectUI::PointerKeyFrame::put_PointerValue(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PointerKeyFrame_PointerValue, value));
}
_Check_return_ HRESULT DirectUI::PointerKeyFrame::get_TargetValue(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PointerKeyFrame_TargetValue, pValue));
}
_Check_return_ HRESULT DirectUI::PointerKeyFrame::put_TargetValue(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PointerKeyFrame_TargetValue, value));
}

// Events.

// Methods.


namespace DirectUI
{
}
