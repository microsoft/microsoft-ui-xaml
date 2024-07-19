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

#include "RepositionThemeTransition.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::RepositionThemeTransitionGenerated::RepositionThemeTransitionGenerated()
{
}

DirectUI::RepositionThemeTransitionGenerated::~RepositionThemeTransitionGenerated()
{
}

HRESULT DirectUI::RepositionThemeTransitionGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::RepositionThemeTransition)))
    {
        *ppObject = static_cast<DirectUI::RepositionThemeTransition*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IRepositionThemeTransition)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IRepositionThemeTransition*>(this);
    }
    else
    {
        RRETURN(DirectUI::Transition::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::RepositionThemeTransitionGenerated::get_IsStaggeringEnabled(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::RepositionThemeTransition_IsStaggeringEnabled, pValue));
}
IFACEMETHODIMP DirectUI::RepositionThemeTransitionGenerated::put_IsStaggeringEnabled(_In_ BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::RepositionThemeTransition_IsStaggeringEnabled, value));
}

// Events.

// Methods.

HRESULT DirectUI::RepositionThemeTransitionFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IRepositionThemeTransitionStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IRepositionThemeTransitionStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.

// Dependency properties.
IFACEMETHODIMP DirectUI::RepositionThemeTransitionFactory::get_IsStaggeringEnabledProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::RepositionThemeTransition_IsStaggeringEnabled, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_RepositionThemeTransition()
    {
        RRETURN(ctl::ActivationFactoryCreator<RepositionThemeTransitionFactory>::CreateActivationFactory());
    }
}
