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

#include "Frame.g.h"
#include "FrameNavigationOptions.g.h"
#include "NavigationTransitionInfo.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::FrameGenerated::FrameGenerated()
{
}

DirectUI::FrameGenerated::~FrameGenerated()
{
}

HRESULT DirectUI::FrameGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::Frame)))
    {
        *ppObject = static_cast<DirectUI::Frame*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IFrame)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IFrame*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IFramePrivate)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IFramePrivate*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::INavigate)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::INavigate*>(this);
    }
    else
    {
        RRETURN(DirectUI::ContentControl::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::FrameGenerated::get_BackStack(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Navigation::PageStackEntry*>** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(static_cast<Frame*>(this)->get_BackStackImpl(ppValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::get_BackStackDepth(_Out_ INT* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Frame_BackStackDepth, pValue));
}
_Check_return_ HRESULT DirectUI::FrameGenerated::put_BackStackDepth(_In_ INT value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Frame_BackStackDepth, value));
}
IFACEMETHODIMP DirectUI::FrameGenerated::get_CacheSize(_Out_ INT* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Frame_CacheSize, pValue));
}
IFACEMETHODIMP DirectUI::FrameGenerated::put_CacheSize(_In_ INT value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Frame_CacheSize, value));
}
IFACEMETHODIMP DirectUI::FrameGenerated::get_CanGoBack(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Frame_CanGoBack, pValue));
}
_Check_return_ HRESULT DirectUI::FrameGenerated::put_CanGoBack(_In_ BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Frame_CanGoBack, value));
}
IFACEMETHODIMP DirectUI::FrameGenerated::get_CanGoForward(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Frame_CanGoForward, pValue));
}
_Check_return_ HRESULT DirectUI::FrameGenerated::put_CanGoForward(_In_ BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Frame_CanGoForward, value));
}
IFACEMETHODIMP DirectUI::FrameGenerated::get_CurrentSourcePageType(_Out_ ABI::Windows::UI::Xaml::Interop::TypeName* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Frame_CurrentSourcePageType, pValue));
}
_Check_return_ HRESULT DirectUI::FrameGenerated::put_CurrentSourcePageType(_In_ ABI::Windows::UI::Xaml::Interop::TypeName value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Frame_CurrentSourcePageType, value));
}
IFACEMETHODIMP DirectUI::FrameGenerated::get_ForwardStack(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Navigation::PageStackEntry*>** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(static_cast<Frame*>(this)->get_ForwardStackImpl(ppValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::get_IsNavigationStackEnabled(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Frame_IsNavigationStackEnabled, pValue));
}
IFACEMETHODIMP DirectUI::FrameGenerated::put_IsNavigationStackEnabled(_In_ BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Frame_IsNavigationStackEnabled, value));
}
IFACEMETHODIMP DirectUI::FrameGenerated::get_SourcePageType(_Out_ ABI::Windows::UI::Xaml::Interop::TypeName* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Frame_SourcePageType, pValue));
}
IFACEMETHODIMP DirectUI::FrameGenerated::put_SourcePageType(_In_ ABI::Windows::UI::Xaml::Interop::TypeName value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Frame_SourcePageType, value));
}

// Events.
_Check_return_ HRESULT DirectUI::FrameGenerated::GetNavigatedEventSourceNoRef(_Outptr_ NavigatedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::Frame_Navigated, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<NavigatedEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::Frame_Navigated, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::Frame_Navigated, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::FrameGenerated::add_Navigated(_In_ ABI::Microsoft::UI::Xaml::Navigation::INavigatedEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    NavigatedEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetNavigatedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::FrameGenerated::remove_Navigated(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    NavigatedEventSourceType* pEventSource = nullptr;
    ABI::Microsoft::UI::Xaml::Navigation::INavigatedEventHandler* pValue = (ABI::Microsoft::UI::Xaml::Navigation::INavigatedEventHandler*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetNavigatedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::Frame_Navigated));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::FrameGenerated::GetNavigatingEventSourceNoRef(_Outptr_ NavigatingEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::Frame_Navigating, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<NavigatingEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::Frame_Navigating, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::Frame_Navigating, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::FrameGenerated::add_Navigating(_In_ ABI::Microsoft::UI::Xaml::Navigation::INavigatingCancelEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    NavigatingEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetNavigatingEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::FrameGenerated::remove_Navigating(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    NavigatingEventSourceType* pEventSource = nullptr;
    ABI::Microsoft::UI::Xaml::Navigation::INavigatingCancelEventHandler* pValue = (ABI::Microsoft::UI::Xaml::Navigation::INavigatingCancelEventHandler*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetNavigatingEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::Frame_Navigating));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::FrameGenerated::GetNavigationFailedEventSourceNoRef(_Outptr_ NavigationFailedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::Frame_NavigationFailed, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<NavigationFailedEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::Frame_NavigationFailed, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::Frame_NavigationFailed, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::FrameGenerated::add_NavigationFailed(_In_ ABI::Microsoft::UI::Xaml::Navigation::INavigationFailedEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    NavigationFailedEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetNavigationFailedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::FrameGenerated::remove_NavigationFailed(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    NavigationFailedEventSourceType* pEventSource = nullptr;
    ABI::Microsoft::UI::Xaml::Navigation::INavigationFailedEventHandler* pValue = (ABI::Microsoft::UI::Xaml::Navigation::INavigationFailedEventHandler*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetNavigationFailedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::Frame_NavigationFailed));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::FrameGenerated::GetNavigationStoppedEventSourceNoRef(_Outptr_ NavigationStoppedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::Frame_NavigationStopped, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<NavigationStoppedEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::Frame_NavigationStopped, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::Frame_NavigationStopped, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::FrameGenerated::add_NavigationStopped(_In_ ABI::Microsoft::UI::Xaml::Navigation::INavigationStoppedEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    NavigationStoppedEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetNavigationStoppedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::FrameGenerated::remove_NavigationStopped(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    NavigationStoppedEventSourceType* pEventSource = nullptr;
    ABI::Microsoft::UI::Xaml::Navigation::INavigationStoppedEventHandler* pValue = (ABI::Microsoft::UI::Xaml::Navigation::INavigationStoppedEventHandler*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetNavigationStoppedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::Frame_NavigationStopped));
    }

Cleanup:
    RRETURN(hr);
}

// Methods.
IFACEMETHODIMP DirectUI::FrameGenerated::GetNavigationState(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_GetNavigationState", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->GetNavigationStateImpl(pReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_GetNavigationState", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::GetNavigationTransitionInfoOverride(_Outptr_ ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo** ppInfoOverride, _Out_ BOOLEAN* pIsBackNavigation, _Out_ BOOLEAN* pIsInitialPage)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_GetNavigationTransitionInfoOverride", 0);
    }
    ARG_NOTNULL(ppInfoOverride, "infoOverride");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->GetNavigationTransitionInfoOverrideImpl(ppInfoOverride, pIsBackNavigation, pIsInitialPage));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_GetNavigationTransitionInfoOverride", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::GoBack()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_GoBack", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->GoBackImpl());
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_GoBack", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::GoBackWithTransitionInfo(_In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo* pTransitionInfoOverride)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_GoBackWithTransitionInfo", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->GoBackWithTransitionInfoImpl(pTransitionInfoOverride));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_GoBackWithTransitionInfo", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::GoForward()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_GoForward", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->GoForwardImpl());
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_GoForward", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::Navigate(_In_ ABI::Windows::UI::Xaml::Interop::TypeName sourcePageType, _In_opt_ IInspectable* pParameter, _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_Navigate", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->NavigateImpl(sourcePageType, pParameter, pReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_Navigate", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::Navigate(_In_ ABI::Windows::UI::Xaml::Interop::TypeName sourcePageType, _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_Navigate", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->NavigateImpl(sourcePageType, pReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_Navigate", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::NavigateToType(_In_ ABI::Windows::UI::Xaml::Interop::TypeName sourcePageType, _In_opt_ IInspectable* pParameter, _In_opt_ ABI::Microsoft::UI::Xaml::Navigation::IFrameNavigationOptions* pNavigationOptions, _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_NavigateToType", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->NavigateToTypeImpl(sourcePageType, pParameter, pNavigationOptions, pReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_NavigateToType", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::NavigateWithTransitionInfo(_In_ ABI::Windows::UI::Xaml::Interop::TypeName sourcePageType, _In_opt_ IInspectable* pParameter, _In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo* pInfoOverride, _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_NavigateWithTransitionInfo", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->NavigateWithTransitionInfoImpl(sourcePageType, pParameter, pInfoOverride, pReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_NavigateWithTransitionInfo", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::SetNavigationState(_In_ HSTRING navigationState)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_SetNavigationState", 0);
    }
    ARG_NOTNULL(navigationState, "navigationState");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->SetNavigationStateImpl(navigationState));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_SetNavigationState", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::SetNavigationStateWithNavigationControl(_In_ HSTRING navigationState, _In_ BOOLEAN suppressNavigate)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_SetNavigationStateWithNavigationControl", 0);
    }
    ARG_NOTNULL(navigationState, "navigationState");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->SetNavigationStateWithNavigationControlImpl(navigationState, suppressNavigate));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_SetNavigationStateWithNavigationControl", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::FrameGenerated::SetNavigationTransitionInfoOverride(_In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo* pInfoOverride)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Frame_SetNavigationTransitionInfoOverride", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Frame*>(this)->SetNavigationTransitionInfoOverrideImpl(pInfoOverride));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Frame_SetNavigationTransitionInfoOverride", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::FrameGenerated::EventAddHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo)
{
    switch (nEventIndex)
    {
    case KnownEventIndex::Frame_Navigated:
        {
            ctl::ComPtr<ABI::Microsoft::UI::Xaml::Navigation::INavigatedEventHandler> spEventHandler;
            IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf());

            if (nullptr != spEventHandler)
            {
                NavigatedEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetNavigatedEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->AddHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::Frame_Navigating:
        {
            ctl::ComPtr<ABI::Microsoft::UI::Xaml::Navigation::INavigatingCancelEventHandler> spEventHandler;
            IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf());

            if (nullptr != spEventHandler)
            {
                NavigatingEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetNavigatingEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->AddHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::Frame_NavigationFailed:
        {
            ctl::ComPtr<ABI::Microsoft::UI::Xaml::Navigation::INavigationFailedEventHandler> spEventHandler;
            IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf());

            if (nullptr != spEventHandler)
            {
                NavigationFailedEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetNavigationFailedEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->AddHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::Frame_NavigationStopped:
        {
            ctl::ComPtr<ABI::Microsoft::UI::Xaml::Navigation::INavigationStoppedEventHandler> spEventHandler;
            IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf());

            if (nullptr != spEventHandler)
            {
                NavigationStoppedEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetNavigationStoppedEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->AddHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    default:
        IFC_RETURN(DirectUI::ContentControlGenerated::EventAddHandlerByIndex(nEventIndex, pHandler, handledEventsToo));
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::FrameGenerated::EventRemoveHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler)
{
    switch (nEventIndex)
    {
    case KnownEventIndex::Frame_Navigated:
        {
            ctl::ComPtr<ABI::Microsoft::UI::Xaml::Navigation::INavigatedEventHandler> spEventHandler;
            IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf());

            if (nullptr != spEventHandler)
            {
                NavigatedEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetNavigatedEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->RemoveHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::Frame_Navigating:
        {
            ctl::ComPtr<ABI::Microsoft::UI::Xaml::Navigation::INavigatingCancelEventHandler> spEventHandler;
            IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf());

            if (nullptr != spEventHandler)
            {
                NavigatingEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetNavigatingEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->RemoveHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::Frame_NavigationFailed:
        {
            ctl::ComPtr<ABI::Microsoft::UI::Xaml::Navigation::INavigationFailedEventHandler> spEventHandler;
            IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf());

            if (nullptr != spEventHandler)
            {
                NavigationFailedEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetNavigationFailedEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->RemoveHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::Frame_NavigationStopped:
        {
            ctl::ComPtr<ABI::Microsoft::UI::Xaml::Navigation::INavigationStoppedEventHandler> spEventHandler;
            IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf());

            if (nullptr != spEventHandler)
            {
                NavigationStoppedEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetNavigationStoppedEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->RemoveHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    default:
        IFC_RETURN(DirectUI::ContentControlGenerated::EventRemoveHandlerByIndex(nEventIndex, pHandler));
        break;
    }

    return S_OK;
}

HRESULT DirectUI::FrameFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IFrameFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IFrameFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IFrameStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IFrameStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::FrameFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IFrame** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Controls::IFrame);
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
IFACEMETHODIMP DirectUI::FrameFactory::get_CacheSizeProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Frame_CacheSize, ppValue));
}
IFACEMETHODIMP DirectUI::FrameFactory::get_CanGoBackProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Frame_CanGoBack, ppValue));
}
IFACEMETHODIMP DirectUI::FrameFactory::get_CanGoForwardProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Frame_CanGoForward, ppValue));
}
IFACEMETHODIMP DirectUI::FrameFactory::get_CurrentSourcePageTypeProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Frame_CurrentSourcePageType, ppValue));
}
IFACEMETHODIMP DirectUI::FrameFactory::get_SourcePageTypeProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Frame_SourcePageType, ppValue));
}
IFACEMETHODIMP DirectUI::FrameFactory::get_BackStackDepthProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Frame_BackStackDepth, ppValue));
}
IFACEMETHODIMP DirectUI::FrameFactory::get_BackStackProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Frame_BackStack, ppValue));
}
IFACEMETHODIMP DirectUI::FrameFactory::get_ForwardStackProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Frame_ForwardStack, ppValue));
}
IFACEMETHODIMP DirectUI::FrameFactory::get_IsNavigationStackEnabledProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Frame_IsNavigationStackEnabled, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_Frame()
    {
        RRETURN(ctl::ActivationFactoryCreator<FrameFactory>::CreateActivationFactory());
    }
}
