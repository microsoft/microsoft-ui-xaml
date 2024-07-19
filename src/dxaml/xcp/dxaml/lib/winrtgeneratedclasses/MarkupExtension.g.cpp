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

#include "MarkupExtension.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::MarkupExtension::MarkupExtension()
{
}

DirectUI::MarkupExtension::~MarkupExtension()
{
}

HRESULT DirectUI::MarkupExtension::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::MarkupExtension)))
    {
        *ppObject = static_cast<DirectUI::MarkupExtension*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Markup::IMarkupExtension)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Markup::IMarkupExtension*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Markup::IMarkupExtensionOverrides)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Markup::IMarkupExtensionOverrides*>(this);
    }
    else
    {
        RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.

// Events.

// Methods.
IFACEMETHODIMP DirectUI::MarkupExtension::ProvideValue(_Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "MarkupExtension_ProvideValue", 0);
    }
    ARG_VALIDRETURNPOINTER(ppReturnValue);
    IFC(CheckThread());
    IFC(static_cast<MarkupExtension*>(this)->ProvideValueImpl(ppReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "MarkupExtension_ProvideValue", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::MarkupExtension::ProvideValueProtected(_Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Markup::IMarkupExtensionOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->ProvideValue(ppReturnValue));
    }
    else
    {
        IFC(ProvideValue(ppReturnValue));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::MarkupExtension::ProvideValueWithIXamlServiceProvider(_In_ ABI::Microsoft::UI::Xaml::IXamlServiceProvider* pServiceProvider, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "MarkupExtension_ProvideValueWithIXamlServiceProvider", 0);
    }
    ARG_NOTNULL(pServiceProvider, "serviceProvider");
    ARG_VALIDRETURNPOINTER(ppReturnValue);
    IFC(CheckThread());
    IFC(static_cast<MarkupExtension*>(this)->ProvideValueWithIXamlServiceProviderImpl(pServiceProvider, ppReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "MarkupExtension_ProvideValueWithIXamlServiceProvider", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::MarkupExtension::ProvideValueWithIXamlServiceProviderProtected(_In_ ABI::Microsoft::UI::Xaml::IXamlServiceProvider* pServiceProvider, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Markup::IMarkupExtensionOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->ProvideValueWithIXamlServiceProvider(pServiceProvider, ppReturnValue));
    }
    else
    {
        IFC(ProvideValueWithIXamlServiceProvider(pServiceProvider, ppReturnValue));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}

HRESULT DirectUI::MarkupExtensionFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Markup::IMarkupExtensionFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Markup::IMarkupExtensionFactory*>(this);
    }
    else
    {
        RRETURN(ctl::AggregableActivationFactory<DirectUI::MarkupExtension>::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::MarkupExtensionFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Markup::IMarkupExtension** ppInstance)
{


    // Can't just IFC(_RETURN) this because for some validate calls (those with multiple template parameters), the
    // preprocessor gets confused at the "," in the template type-list before the function's opening parenthesis.
    // So we'll use IFC_RETURN syntax with a local hr variable, kind of weirdly.
    const HRESULT hr = ctl::ValidateFactoryCreateInstanceWithAggregableActivationFactory<DirectUI::MarkupExtension,ABI::Microsoft::UI::Xaml::Markup::IMarkupExtension>(pOuter, ppInner, reinterpret_cast<IUnknown**>(ppInstance), GetTypeIndex(), false /*isFreeThreaded*/);
    IFC_RETURN(hr);
    return S_OK;
}

// Dependency properties.

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_MarkupExtension()
    {
        RRETURN(ctl::ActivationFactoryCreator<MarkupExtensionFactory>::CreateActivationFactory());
    }
}
