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

#include "Underline.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::Underline::Underline()
{
}

DirectUI::Underline::~Underline()
{
}

HRESULT DirectUI::Underline::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::Underline)))
    {
        *ppObject = static_cast<DirectUI::Underline*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Documents::IUnderline)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Documents::IUnderline*>(this);
    }
    else
    {
        RRETURN(DirectUI::Span::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.

// Events.

// Methods.


namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_Underline()
    {
        RRETURN(ctl::BetterActivationFactoryCreator::GetForDO(KnownTypeIndex::Underline));
    }
}
