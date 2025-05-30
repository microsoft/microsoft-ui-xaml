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

#include "AppBar.g.h"
#include "AppBarTemplateSettings.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::AppBarGenerated::AppBarGenerated()
{
}

DirectUI::AppBarGenerated::~AppBarGenerated()
{
}

HRESULT DirectUI::AppBarGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::AppBar)))
    {
        *ppObject = static_cast<DirectUI::AppBar*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IAppBar)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IAppBar*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IAppBarOverrides)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IAppBarOverrides*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::Internal::FrameworkUdk::IBackButtonPressedListener)))
    {
        *ppObject = static_cast<ABI::Microsoft::Internal::FrameworkUdk::IBackButtonPressedListener*>(this);
    }
    else
    {
        RRETURN(DirectUI::ContentControl::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::AppBarGenerated::get_ClosedDisplayMode(_Out_ ABI::Microsoft::UI::Xaml::Controls::AppBarClosedDisplayMode* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBar_ClosedDisplayMode, pValue));
}
IFACEMETHODIMP DirectUI::AppBarGenerated::put_ClosedDisplayMode(ABI::Microsoft::UI::Xaml::Controls::AppBarClosedDisplayMode value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBar_ClosedDisplayMode, value));
}
IFACEMETHODIMP DirectUI::AppBarGenerated::get_IsOpen(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBar_IsOpen, pValue));
}
IFACEMETHODIMP DirectUI::AppBarGenerated::put_IsOpen(BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBar_IsOpen, value));
}
IFACEMETHODIMP DirectUI::AppBarGenerated::get_IsSticky(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBar_IsSticky, pValue));
}
IFACEMETHODIMP DirectUI::AppBarGenerated::put_IsSticky(BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBar_IsSticky, value));
}
IFACEMETHODIMP DirectUI::AppBarGenerated::get_LightDismissOverlayMode(_Out_ ABI::Microsoft::UI::Xaml::Controls::LightDismissOverlayMode* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBar_LightDismissOverlayMode, pValue));
}
IFACEMETHODIMP DirectUI::AppBarGenerated::put_LightDismissOverlayMode(ABI::Microsoft::UI::Xaml::Controls::LightDismissOverlayMode value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBar_LightDismissOverlayMode, value));
}
IFACEMETHODIMP DirectUI::AppBarGenerated::get_TemplateSettings(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::Primitives::IAppBarTemplateSettings** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBar_TemplateSettings, ppValue));
}
_Check_return_ HRESULT DirectUI::AppBarGenerated::put_TemplateSettings(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::Primitives::IAppBarTemplateSettings* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBar_TemplateSettings, pValue));
}

// Events.
_Check_return_ HRESULT DirectUI::AppBarGenerated::GetClosedEventSourceNoRef(_Outptr_ ClosedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::AppBar_Closed, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<ClosedEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::AppBar_Closed, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::AppBar_Closed, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::AppBarGenerated::add_Closed(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    ClosedEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetClosedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::AppBarGenerated::remove_Closed(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    ClosedEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue = (ABI::Windows::Foundation::IEventHandler<IInspectable*>*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetClosedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::AppBar_Closed));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::AppBarGenerated::GetClosingEventSourceNoRef(_Outptr_ ClosingEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::AppBar_Closing, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<ClosingEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::AppBar_Closing, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::AppBar_Closing, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::AppBarGenerated::add_Closing(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    ClosingEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetClosingEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::AppBarGenerated::remove_Closing(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    ClosingEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue = (ABI::Windows::Foundation::IEventHandler<IInspectable*>*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetClosingEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::AppBar_Closing));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::AppBarGenerated::GetOpenedEventSourceNoRef(_Outptr_ OpenedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::AppBar_Opened, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<OpenedEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::AppBar_Opened, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::AppBar_Opened, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::AppBarGenerated::add_Opened(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    OpenedEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetOpenedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::AppBarGenerated::remove_Opened(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    OpenedEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue = (ABI::Windows::Foundation::IEventHandler<IInspectable*>*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetOpenedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::AppBar_Opened));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::AppBarGenerated::GetOpeningEventSourceNoRef(_Outptr_ OpeningEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::AppBar_Opening, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<OpeningEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::AppBar_Opening, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::AppBar_Opening, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::AppBarGenerated::add_Opening(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    OpeningEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetOpeningEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::AppBarGenerated::remove_Opening(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    OpeningEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue = (ABI::Windows::Foundation::IEventHandler<IInspectable*>*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetOpeningEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::AppBar_Opening));
    }

Cleanup:
    RRETURN(hr);
}

// Methods.
IFACEMETHODIMP DirectUI::AppBarGenerated::OnBackButtonPressed(_Out_ BOOLEAN* pResult)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBar_OnBackButtonPressed", 0);
    }
    ARG_VALIDRETURNPOINTER(pResult);
    *pResult={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBar*>(this)->OnBackButtonPressedImpl(pResult));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBar_OnBackButtonPressed", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarGenerated::OnClosed(_In_ IInspectable* pE)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBar_OnClosed", 0);
    }
    ARG_NOTNULL(pE, "e");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBar*>(this)->OnClosedImpl(pE));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBar_OnClosed", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::AppBarGenerated::OnClosedProtected(_In_ IInspectable* pE)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Controls::IAppBarOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->OnClosed(pE));
    }
    else
    {
        IFC(OnClosed(pE));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarGenerated::OnClosing(_In_ IInspectable* pE)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBar_OnClosing", 0);
    }
    ARG_NOTNULL(pE, "e");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBar*>(this)->OnClosingImpl(pE));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBar_OnClosing", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::AppBarGenerated::OnClosingProtected(_In_ IInspectable* pE)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Controls::IAppBarOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->OnClosing(pE));
    }
    else
    {
        IFC(OnClosing(pE));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarGenerated::OnOpened(_In_ IInspectable* pE)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBar_OnOpened", 0);
    }
    ARG_NOTNULL(pE, "e");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBar*>(this)->OnOpenedImpl(pE));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBar_OnOpened", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::AppBarGenerated::OnOpenedProtected(_In_ IInspectable* pE)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Controls::IAppBarOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->OnOpened(pE));
    }
    else
    {
        IFC(OnOpened(pE));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarGenerated::OnOpening(_In_ IInspectable* pE)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBar_OnOpening", 0);
    }
    ARG_NOTNULL(pE, "e");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBar*>(this)->OnOpeningImpl(pE));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBar_OnOpening", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::AppBarGenerated::OnOpeningProtected(_In_ IInspectable* pE)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Controls::IAppBarOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->OnOpening(pE));
    }
    else
    {
        IFC(OnOpening(pE));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::AppBarGenerated::EventAddHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo)
{
    switch (nEventIndex)
    {
    case KnownEventIndex::AppBar_Closed:
        {
            ctl::ComPtr<ABI::Windows::Foundation::IEventHandler<IInspectable*>> spEventHandler;
            IFC_RETURN(IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf()));

            if (nullptr != spEventHandler)
            {
                ClosedEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetClosedEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->AddHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::AppBar_Closing:
        {
            ctl::ComPtr<ABI::Windows::Foundation::IEventHandler<IInspectable*>> spEventHandler;
            IFC_RETURN(IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf()));

            if (nullptr != spEventHandler)
            {
                ClosingEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetClosingEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->AddHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::AppBar_Opened:
        {
            ctl::ComPtr<ABI::Windows::Foundation::IEventHandler<IInspectable*>> spEventHandler;
            IFC_RETURN(IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf()));

            if (nullptr != spEventHandler)
            {
                OpenedEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetOpenedEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->AddHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::AppBar_Opening:
        {
            ctl::ComPtr<ABI::Windows::Foundation::IEventHandler<IInspectable*>> spEventHandler;
            IFC_RETURN(IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf()));

            if (nullptr != spEventHandler)
            {
                OpeningEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetOpeningEventSourceNoRef(&pEventSource));
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

_Check_return_ HRESULT DirectUI::AppBarGenerated::EventRemoveHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler)
{
    switch (nEventIndex)
    {
    case KnownEventIndex::AppBar_Closed:
        {
            ctl::ComPtr<ABI::Windows::Foundation::IEventHandler<IInspectable*>> spEventHandler;
            IFC_RETURN(IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf()));

            if (nullptr != spEventHandler)
            {
                ClosedEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetClosedEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->RemoveHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::AppBar_Closing:
        {
            ctl::ComPtr<ABI::Windows::Foundation::IEventHandler<IInspectable*>> spEventHandler;
            IFC_RETURN(IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf()));

            if (nullptr != spEventHandler)
            {
                ClosingEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetClosingEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->RemoveHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::AppBar_Opened:
        {
            ctl::ComPtr<ABI::Windows::Foundation::IEventHandler<IInspectable*>> spEventHandler;
            IFC_RETURN(IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf()));

            if (nullptr != spEventHandler)
            {
                OpenedEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetOpenedEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->RemoveHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    case KnownEventIndex::AppBar_Opening:
        {
            ctl::ComPtr<ABI::Windows::Foundation::IEventHandler<IInspectable*>> spEventHandler;
            IFC_RETURN(IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf()));

            if (nullptr != spEventHandler)
            {
                OpeningEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetOpeningEventSourceNoRef(&pEventSource));
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

HRESULT DirectUI::AppBarFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IAppBarFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IAppBarFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IAppBarStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IAppBarStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::AppBarFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IAppBar** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Controls::IAppBar);
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
IFACEMETHODIMP DirectUI::AppBarFactory::get_IsOpenProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBar_IsOpen, ppValue));
}
IFACEMETHODIMP DirectUI::AppBarFactory::get_IsStickyProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBar_IsSticky, ppValue));
}
IFACEMETHODIMP DirectUI::AppBarFactory::get_ClosedDisplayModeProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBar_ClosedDisplayMode, ppValue));
}

IFACEMETHODIMP DirectUI::AppBarFactory::get_LightDismissOverlayModeProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBar_LightDismissOverlayMode, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_AppBar()
    {
        RRETURN(ctl::ActivationFactoryCreator<AppBarFactory>::CreateActivationFactory());
    }
}
