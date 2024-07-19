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

#include "NavigationTransitionInfo.g.h"
#include "UIElement.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::NavigationTransitionInfoGenerated::NavigationTransitionInfoGenerated()
{
}

DirectUI::NavigationTransitionInfoGenerated::~NavigationTransitionInfoGenerated()
{
}

HRESULT DirectUI::NavigationTransitionInfoGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::NavigationTransitionInfo)))
    {
        *ppObject = static_cast<DirectUI::NavigationTransitionInfo*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoOverrides)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoOverrides*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoOverridesPrivate)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoOverridesPrivate*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoPrivate)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoPrivate*>(this);
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
IFACEMETHODIMP DirectUI::NavigationTransitionInfoGenerated::CreateStoryboards(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _In_ ABI::Microsoft::UI::Xaml::Media::Animation::NavigationTrigger trigger, _In_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Media::Animation::Storyboard*>* pStoryboards)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "NavigationTransitionInfo_CreateStoryboards", 0);
    }
    ARG_NOTNULL(pElement, "element");
    ARG_NOTNULL(pStoryboards, "storyboards");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<NavigationTransitionInfo*>(this)->CreateStoryboardsImpl(pElement, trigger, pStoryboards));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "NavigationTransitionInfo_CreateStoryboards", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::NavigationTransitionInfoGenerated::CreateStoryboardsCore(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _In_ ABI::Microsoft::UI::Xaml::Media::Animation::NavigationTrigger trigger, _In_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Media::Animation::Storyboard*>* pStoryboards)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "NavigationTransitionInfo_CreateStoryboardsCore", 0);
    }
    ARG_NOTNULL(pElement, "element");
    ARG_NOTNULL(pStoryboards, "storyboards");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<NavigationTransitionInfo*>(this)->CreateStoryboardsCoreImpl(pElement, trigger, pStoryboards));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "NavigationTransitionInfo_CreateStoryboardsCore", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::NavigationTransitionInfoGenerated::GetNavigationStateCore(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "NavigationTransitionInfo_GetNavigationStateCore", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<NavigationTransitionInfo*>(this)->GetNavigationStateCoreImpl(pReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "NavigationTransitionInfo_GetNavigationStateCore", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::NavigationTransitionInfoGenerated::GetNavigationStateCoreProtected(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->GetNavigationStateCore(pReturnValue));
    }
    else
    {
        IFC(GetNavigationStateCore(pReturnValue));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::NavigationTransitionInfoGenerated::SetNavigationStateCore(_In_ HSTRING navigationState)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "NavigationTransitionInfo_SetNavigationStateCore", 0);
    }
    ARG_NOTNULL(navigationState, "navigationState");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<NavigationTransitionInfo*>(this)->SetNavigationStateCoreImpl(navigationState));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "NavigationTransitionInfo_SetNavigationStateCore", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::NavigationTransitionInfoGenerated::SetNavigationStateCoreProtected(_In_ HSTRING navigationState)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->SetNavigationStateCore(navigationState));
    }
    else
    {
        IFC(SetNavigationStateCore(navigationState));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}

HRESULT DirectUI::NavigationTransitionInfoFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoFactory*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableAbstractCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::NavigationTransitionInfoFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo);
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
    const HRESULT hr = ctl::ValidateFactoryCreateInstanceWithBetterAggregableAbstractCoreObjectActivationFactory(pOuter, ppInner, reinterpret_cast<IUnknown**>(ppInstance), GetTypeIndex(), false /*isFreeThreaded*/);
    IFC_RETURN(hr);
    return S_OK;
}

// Dependency properties.

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_NavigationTransitionInfo()
    {
        RRETURN(ctl::ActivationFactoryCreator<NavigationTransitionInfoFactory>::CreateActivationFactory());
    }
}
