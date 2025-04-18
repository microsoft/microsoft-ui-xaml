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

#include "BounceEase.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::BounceEase::BounceEase()
{
}

DirectUI::BounceEase::~BounceEase()
{
}

HRESULT DirectUI::BounceEase::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::BounceEase)))
    {
        *ppObject = static_cast<DirectUI::BounceEase*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IBounceEase)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IBounceEase*>(this);
    }
    else
    {
        RRETURN(DirectUI::EasingFunctionBase::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::BounceEase::get_Bounces(_Out_ INT* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::BounceEase_Bounces, pValue));
}
IFACEMETHODIMP DirectUI::BounceEase::put_Bounces(INT value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::BounceEase_Bounces, value));
}
IFACEMETHODIMP DirectUI::BounceEase::get_Bounciness(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::BounceEase_Bounciness, pValue));
}
IFACEMETHODIMP DirectUI::BounceEase::put_Bounciness(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::BounceEase_Bounciness, value));
}

// Events.

// Methods.

HRESULT DirectUI::BounceEaseFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IBounceEaseStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IBounceEaseStatics*>(this);
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
IFACEMETHODIMP DirectUI::BounceEaseFactory::get_BouncesProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::BounceEase_Bounces, ppValue));
}
IFACEMETHODIMP DirectUI::BounceEaseFactory::get_BouncinessProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::BounceEase_Bounciness, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_BounceEase()
    {
        RRETURN(ctl::ActivationFactoryCreator<BounceEaseFactory>::CreateActivationFactory());
    }
}
