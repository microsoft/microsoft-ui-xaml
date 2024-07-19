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

#include "FlyoutPresenter.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::FlyoutPresenterGenerated::FlyoutPresenterGenerated()
{
}

DirectUI::FlyoutPresenterGenerated::~FlyoutPresenterGenerated()
{
}

HRESULT DirectUI::FlyoutPresenterGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::FlyoutPresenter)))
    {
        *ppObject = static_cast<DirectUI::FlyoutPresenter*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IFlyoutPresenter)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IFlyoutPresenter*>(this);
    }
    else
    {
        RRETURN(DirectUI::ContentControl::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::FlyoutPresenterGenerated::get_IsDefaultShadowEnabled(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::FlyoutPresenter_IsDefaultShadowEnabled, pValue));
}
IFACEMETHODIMP DirectUI::FlyoutPresenterGenerated::put_IsDefaultShadowEnabled(_In_ BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::FlyoutPresenter_IsDefaultShadowEnabled, value));
}

// Events.

// Methods.

HRESULT DirectUI::FlyoutPresenterFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IFlyoutPresenterFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IFlyoutPresenterFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IFlyoutPresenterStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IFlyoutPresenterStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::FlyoutPresenterFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IFlyoutPresenter** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Controls::IFlyoutPresenter);
    const GUID metadataAPIGUID = MetadataAPI::GetClassInfoByIndex(GetTypeIndex())->GetGuid();
    const KnownTypeIndex typeIndex = GetTypeIndex();

    if(uuidofGUID != metadataAPIGUID)
    {
        XAML_FAIL_FAST();
    }
#endif

    // Can't just IFC(_RETURN) this because for some validate calls (those with multiple template parameters), the
    // preprocessor gets confused at the "," in the template type-list before the function's opening parenthesis.
    // So we'll use IFC_RETURN syntax with a local hr variable, kind of weirdly.
    const HRESULT hr = ctl::ValidateFactoryCreateInstanceWithBetterAggregableCoreObjectActivationFactory(pOuter, ppInner, reinterpret_cast<IUnknown**>(ppInstance), GetTypeIndex(), false /*isFreeThreaded*/);
    IFC_RETURN(hr);
    return S_OK;
}

// Dependency properties.
IFACEMETHODIMP DirectUI::FlyoutPresenterFactory::get_IsDefaultShadowEnabledProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::FlyoutPresenter_IsDefaultShadowEnabled, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_FlyoutPresenter()
    {
        RRETURN(ctl::ActivationFactoryCreator<FlyoutPresenterFactory>::CreateActivationFactory());
    }
}
