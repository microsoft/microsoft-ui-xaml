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

#include "IRawElementProviderSimple.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::IRawElementProviderSimpleGenerated::IRawElementProviderSimpleGenerated()
{
}

DirectUI::IRawElementProviderSimpleGenerated::~IRawElementProviderSimpleGenerated()
{
}

HRESULT DirectUI::IRawElementProviderSimpleGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::IRawElementProviderSimple)))
    {
        *ppObject = static_cast<DirectUI::IRawElementProviderSimple*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Automation::Provider::IIRawElementProviderSimple)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Automation::Provider::IIRawElementProviderSimple*>(this);
    }
    else
    {
        RRETURN(DirectUI::DependencyObject::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.

// Events.

// Methods.


namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_IRawElementProviderSimple()
    {
        RRETURN(ctl::BetterActivationFactoryCreator::GetForDO(KnownTypeIndex::IRawElementProviderSimple));
    }
}
