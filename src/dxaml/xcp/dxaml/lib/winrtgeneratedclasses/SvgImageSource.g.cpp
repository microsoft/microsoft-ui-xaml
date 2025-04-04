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

#include "SvgImageSource.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::SvgImageSourceGenerated::SvgImageSourceGenerated()
{
}

DirectUI::SvgImageSourceGenerated::~SvgImageSourceGenerated()
{
}

HRESULT DirectUI::SvgImageSourceGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::SvgImageSource)))
    {
        *ppObject = static_cast<DirectUI::SvgImageSource*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Imaging::ISvgImageSource)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Imaging::ISvgImageSource*>(this);
    }
    else
    {
        RRETURN(DirectUI::ImageSource::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::get_RasterizePixelHeight(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SvgImageSource_RasterizePixelHeight, pValue));
}
IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::put_RasterizePixelHeight(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SvgImageSource_RasterizePixelHeight, value));
}
IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::get_RasterizePixelWidth(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SvgImageSource_RasterizePixelWidth, pValue));
}
IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::put_RasterizePixelWidth(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SvgImageSource_RasterizePixelWidth, value));
}
IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::get_UriSource(_Outptr_result_maybenull_ ABI::Windows::Foundation::IUriRuntimeClass** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SvgImageSource_UriSource, ppValue));
}
IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::put_UriSource(_In_opt_ ABI::Windows::Foundation::IUriRuntimeClass* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SvgImageSource_UriSource, pValue));
}

// Events.
_Check_return_ HRESULT DirectUI::SvgImageSourceGenerated::GetOpenedEventSourceNoRef(_Outptr_ OpenedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::SvgImageSource_Opened, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<OpenedEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::SvgImageSource_Opened, this, /* bUseEventManager */ true);
        IFC(StoreEventSource(KnownEventIndex::SvgImageSource_Opened, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::add_Opened(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSource*, ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceOpenedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken)
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

IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::remove_Opened(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    OpenedEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSource*, ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceOpenedEventArgs*>* pValue = (ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSource*, ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceOpenedEventArgs*>*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetOpenedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::SvgImageSource_Opened));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::SvgImageSourceGenerated::GetOpenFailedEventSourceNoRef(_Outptr_ OpenFailedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::SvgImageSource_OpenFailed, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<OpenFailedEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::SvgImageSource_OpenFailed, this, /* bUseEventManager */ true);
        IFC(StoreEventSource(KnownEventIndex::SvgImageSource_OpenFailed, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::add_OpenFailed(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSource*, ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceFailedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    OpenFailedEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetOpenFailedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::remove_OpenFailed(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    OpenFailedEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSource*, ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceFailedEventArgs*>* pValue = (ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSource*, ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceFailedEventArgs*>*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetOpenFailedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::SvgImageSource_OpenFailed));
    }

Cleanup:
    RRETURN(hr);
}

// Methods.
IFACEMETHODIMP DirectUI::SvgImageSourceGenerated::SetSourceAsync(_In_ ABI::Windows::Storage::Streams::IRandomAccessStream* pStreamSource, _Outptr_ ABI::Windows::Foundation::IAsyncOperation<ABI::Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus>** ppReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "SvgImageSource_SetSourceAsync", 0);
    }
    ARG_NOTNULL(pStreamSource, "streamSource");
    ARG_VALIDRETURNPOINTER(ppReturnValue);
    *ppReturnValue={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<SvgImageSource*>(this)->SetSourceAsyncImpl(pStreamSource, ppReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "SvgImageSource_SetSourceAsync", hr);
    }
    RRETURN(hr);
}

HRESULT DirectUI::SvgImageSourceFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Imaging::ISvgImageSourceFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Imaging::ISvgImageSourceFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Imaging::ISvgImageSourceStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Imaging::ISvgImageSourceStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::SvgImageSourceFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Media::Imaging::ISvgImageSource** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Media::Imaging::ISvgImageSource);
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
IFACEMETHODIMP DirectUI::SvgImageSourceFactory::CreateInstanceWithUriSource(_In_ ABI::Windows::Foundation::IUriRuntimeClass* pUriSource, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Media::Imaging::ISvgImageSource** ppInstance)
{
    HRESULT hr = S_OK;
    ARG_NOTNULL(pUriSource, "uriSource");
    ARG_VALIDRETURNPOINTER(ppInstance);
    IFC(CreateInstanceWithUriSourceImpl(pUriSource, pOuter, ppInner, ppInstance));
Cleanup:
    return hr;
}

// Dependency properties.
IFACEMETHODIMP DirectUI::SvgImageSourceFactory::get_UriSourceProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SvgImageSource_UriSource, ppValue));
}
IFACEMETHODIMP DirectUI::SvgImageSourceFactory::get_RasterizePixelWidthProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SvgImageSource_RasterizePixelWidth, ppValue));
}
IFACEMETHODIMP DirectUI::SvgImageSourceFactory::get_RasterizePixelHeightProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SvgImageSource_RasterizePixelHeight, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_SvgImageSource()
    {
        RRETURN(ctl::ActivationFactoryCreator<SvgImageSourceFactory>::CreateActivationFactory());
    }
}
