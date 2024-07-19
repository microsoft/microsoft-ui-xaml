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

#include "PrintDocument.g.h"
#include "UIElement.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::PrintDocumentGenerated::PrintDocumentGenerated()
{
}

DirectUI::PrintDocumentGenerated::~PrintDocumentGenerated()
{
}

HRESULT DirectUI::PrintDocumentGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PrintDocument)))
    {
        *ppObject = static_cast<DirectUI::PrintDocument*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Printing::IPrintDocument)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Printing::IPrintDocument*>(this);
    }
    else
    {
        RRETURN(DirectUI::DependencyObject::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::get_DesiredFormat(_Out_ DirectUI::PrintDocumentFormat* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PrintDocument_DesiredFormat, pValue));
}
_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::put_DesiredFormat(_In_ DirectUI::PrintDocumentFormat value)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PrintDocument_DesiredFormat, value));
}
IFACEMETHODIMP DirectUI::PrintDocumentGenerated::get_DocumentSource(_Outptr_result_maybenull_ ABI::Windows::Graphics::Printing::IPrintDocumentSource** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(static_cast<PrintDocument*>(this)->get_DocumentSourceImpl(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::get_PrintedPageCount(_Out_ INT* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PrintDocument_PrintedPageCount, pValue));
}

// Events.
_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::GetAddPagesEventSourceNoRef(_Outptr_ AddPagesEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::PrintDocument_AddPages, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<AddPagesEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::PrintDocument_AddPages, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::PrintDocument_AddPages, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::PrintDocumentGenerated::add_AddPages(_In_ ABI::Microsoft::UI::Xaml::Printing::IAddPagesEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    AddPagesEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetAddPagesEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::PrintDocumentGenerated::remove_AddPages(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    AddPagesEventSourceType* pEventSource = nullptr;
    ABI::Microsoft::UI::Xaml::Printing::IAddPagesEventHandler* pValue = (ABI::Microsoft::UI::Xaml::Printing::IAddPagesEventHandler*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetAddPagesEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::PrintDocument_AddPages));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::GetBeginPrintEventSourceNoRef(_Outptr_ BeginPrintEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::PrintDocument_BeginPrint, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<BeginPrintEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::PrintDocument_BeginPrint, this, /* bUseEventManager */ true);
        IFC(StoreEventSource(KnownEventIndex::PrintDocument_BeginPrint, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::add_BeginPrint(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    BeginPrintEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));

    IFC(GetBeginPrintEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::remove_BeginPrint(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    BeginPrintEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue = (ABI::Windows::Foundation::IEventHandler<IInspectable*>*)tToken.value;

    IFC(CheckThread());
    IFC(GetBeginPrintEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::PrintDocument_BeginPrint));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::GetEndPrintEventSourceNoRef(_Outptr_ EndPrintEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::PrintDocument_EndPrint, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<EndPrintEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::PrintDocument_EndPrint, this, /* bUseEventManager */ true);
        IFC(StoreEventSource(KnownEventIndex::PrintDocument_EndPrint, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::add_EndPrint(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    EndPrintEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));

    IFC(GetEndPrintEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::remove_EndPrint(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    EndPrintEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue = (ABI::Windows::Foundation::IEventHandler<IInspectable*>*)tToken.value;

    IFC(CheckThread());
    IFC(GetEndPrintEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::PrintDocument_EndPrint));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::GetGetPreviewPageEventSourceNoRef(_Outptr_ GetPreviewPageEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::PrintDocument_GetPreviewPage, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<GetPreviewPageEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::PrintDocument_GetPreviewPage, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::PrintDocument_GetPreviewPage, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::PrintDocumentGenerated::add_GetPreviewPage(_In_ ABI::Microsoft::UI::Xaml::Printing::IGetPreviewPageEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    GetPreviewPageEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetGetPreviewPageEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::PrintDocumentGenerated::remove_GetPreviewPage(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    GetPreviewPageEventSourceType* pEventSource = nullptr;
    ABI::Microsoft::UI::Xaml::Printing::IGetPreviewPageEventHandler* pValue = (ABI::Microsoft::UI::Xaml::Printing::IGetPreviewPageEventHandler*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetGetPreviewPageEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::PrintDocument_GetPreviewPage));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PrintDocumentGenerated::GetPaginateEventSourceNoRef(_Outptr_ PaginateEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::PrintDocument_Paginate, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<PaginateEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::PrintDocument_Paginate, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::PrintDocument_Paginate, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::PrintDocumentGenerated::add_Paginate(_In_ ABI::Microsoft::UI::Xaml::Printing::IPaginateEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    PaginateEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetPaginateEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::PrintDocumentGenerated::remove_Paginate(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    PaginateEventSourceType* pEventSource = nullptr;
    ABI::Microsoft::UI::Xaml::Printing::IPaginateEventHandler* pValue = (ABI::Microsoft::UI::Xaml::Printing::IPaginateEventHandler*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetPaginateEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::PrintDocument_Paginate));
    }

Cleanup:
    RRETURN(hr);
}

// Methods.
IFACEMETHODIMP DirectUI::PrintDocumentGenerated::AddPage(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pPageVisual)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "PrintDocument_AddPage", 0);
    }

    CUIElement* pPageVisualCore = static_cast<CUIElement*>(pPageVisual ? static_cast<DirectUI::UIElement*>(pPageVisual)->GetHandle() : nullptr);

    ARG_NOTNULL(pPageVisual, "pageVisual");

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));

    IFC(static_cast<CPrintDocument*>(GetHandle())->AddPage(pPageVisualCore));


Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "PrintDocument_AddPage", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PrintDocumentGenerated::AddPagesComplete()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "PrintDocument_AddPagesComplete", 0);
    }


    

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));

    IFC(static_cast<CPrintDocument*>(GetHandle())->AddPagesComplete());


Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "PrintDocument_AddPagesComplete", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PrintDocumentGenerated::InvalidatePreview()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "PrintDocument_InvalidatePreview", 0);
    }


    

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));

    IFC(static_cast<CPrintDocument*>(GetHandle())->InvalidatePreview());


Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "PrintDocument_InvalidatePreview", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PrintDocumentGenerated::SetPreviewPage(_In_ INT pageNumber, _In_ ABI::Microsoft::UI::Xaml::IUIElement* pPageVisual)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "PrintDocument_SetPreviewPage", 0);
    }

    INT pageNumberCore = pageNumber;
    CUIElement* pPageVisualCore = static_cast<CUIElement*>(pPageVisual ? static_cast<DirectUI::UIElement*>(pPageVisual)->GetHandle() : nullptr);

    ARG_NOTNULL(pPageVisual, "pageVisual");

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));

    IFC(static_cast<CPrintDocument*>(GetHandle())->SetPreviewPage(pageNumberCore, pPageVisualCore));


Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "PrintDocument_SetPreviewPage", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PrintDocumentGenerated::SetPreviewPageCount(_In_ INT count, _In_ ABI::Microsoft::UI::Xaml::Printing::PreviewPageCountType type)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "PrintDocument_SetPreviewPageCount", 0);
    }

    INT countCore = count;
    DirectUI::PreviewPageCountType typeCore = static_cast<DirectUI::PreviewPageCountType>(type);

    

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));

    IFC(static_cast<CPrintDocument*>(GetHandle())->SetPreviewPageCount(countCore, typeCore));


Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "PrintDocument_SetPreviewPageCount", hr);
    }
    RRETURN(hr);
}

HRESULT DirectUI::PrintDocumentFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Printing::IPrintDocumentFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Printing::IPrintDocumentFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Printing::IPrintDocumentStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Printing::IPrintDocumentStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::PrintDocumentFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Printing::IPrintDocument** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Printing::IPrintDocument);
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


IFACEMETHODIMP DirectUI::PrintDocumentFactory::get_DocumentSourceProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PrintDocument_DocumentSource, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_PrintDocument()
    {
        RRETURN(ctl::ActivationFactoryCreator<PrintDocumentFactory>::CreateActivationFactory());
    }
}
