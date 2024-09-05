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

#include "PopInThemeAnimation.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::PopInThemeAnimationGenerated::PopInThemeAnimationGenerated()
{
}

DirectUI::PopInThemeAnimationGenerated::~PopInThemeAnimationGenerated()
{
}

HRESULT DirectUI::PopInThemeAnimationGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PopInThemeAnimation)))
    {
        *ppObject = static_cast<DirectUI::PopInThemeAnimation*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IPopInThemeAnimation)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IPopInThemeAnimation*>(this);
    }
    else
    {
        RRETURN(DirectUI::DynamicTimeline::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::PopInThemeAnimationGenerated::get_FromHorizontalOffset(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PopInThemeAnimation_FromHorizontalOffset, pValue));
}
IFACEMETHODIMP DirectUI::PopInThemeAnimationGenerated::put_FromHorizontalOffset(_In_ DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PopInThemeAnimation_FromHorizontalOffset, value));
}
IFACEMETHODIMP DirectUI::PopInThemeAnimationGenerated::get_FromVerticalOffset(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PopInThemeAnimation_FromVerticalOffset, pValue));
}
IFACEMETHODIMP DirectUI::PopInThemeAnimationGenerated::put_FromVerticalOffset(_In_ DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PopInThemeAnimation_FromVerticalOffset, value));
}
IFACEMETHODIMP DirectUI::PopInThemeAnimationGenerated::get_TargetName(_Out_ HSTRING* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PopInThemeAnimation_TargetName, pValue));
}
IFACEMETHODIMP DirectUI::PopInThemeAnimationGenerated::put_TargetName(_In_opt_ HSTRING value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PopInThemeAnimation_TargetName, value));
}

// Events.

// Methods.

HRESULT DirectUI::PopInThemeAnimationFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IPopInThemeAnimationStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IPopInThemeAnimationStatics*>(this);
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
IFACEMETHODIMP DirectUI::PopInThemeAnimationFactory::get_TargetNameProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PopInThemeAnimation_TargetName, ppValue));
}
IFACEMETHODIMP DirectUI::PopInThemeAnimationFactory::get_FromHorizontalOffsetProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PopInThemeAnimation_FromHorizontalOffset, ppValue));
}
IFACEMETHODIMP DirectUI::PopInThemeAnimationFactory::get_FromVerticalOffsetProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PopInThemeAnimation_FromVerticalOffset, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_PopInThemeAnimation()
    {
        RRETURN(ctl::ActivationFactoryCreator<PopInThemeAnimationFactory>::CreateActivationFactory());
    }
}