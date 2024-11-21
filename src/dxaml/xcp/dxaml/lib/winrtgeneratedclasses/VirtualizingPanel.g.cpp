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

#include "VirtualizingPanel.g.h"
#include "ItemContainerGenerator.g.h"
#include "ItemsChangedEventArgs.g.h"
#include "UIElement.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::VirtualizingPanelGenerated::VirtualizingPanelGenerated()
{
}

DirectUI::VirtualizingPanelGenerated::~VirtualizingPanelGenerated()
{
}

HRESULT DirectUI::VirtualizingPanelGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::VirtualizingPanel)))
    {
        *ppObject = static_cast<DirectUI::VirtualizingPanel*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanel)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanel*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanelProtected)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanelProtected*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanelOverrides)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanelOverrides*>(this);
    }
    else
    {
        RRETURN(DirectUI::Panel::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::VirtualizingPanelGenerated::get_ItemContainerGenerator(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::IItemContainerGenerator** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    *ppValue={};
    IFC(CheckThread());
    IFC(static_cast<VirtualizingPanel*>(this)->get_ItemContainerGeneratorImpl(ppValue));
Cleanup:
    RRETURN(hr);
}

// Events.

// Methods.
IFACEMETHODIMP DirectUI::VirtualizingPanelGenerated::AddInternalChild(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pChild)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_AddInternalChild", 0);
    }
    ARG_NOTNULL(pChild, "child");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<VirtualizingPanel*>(this)->AddInternalChildImpl(pChild));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_AddInternalChild", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::VirtualizingPanelGenerated::BringIndexIntoView(INT index)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_BringIndexIntoView", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<VirtualizingPanel*>(this)->BringIndexIntoViewImpl(index));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_BringIndexIntoView", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::VirtualizingPanelGenerated::BringIndexIntoViewProtected(INT index)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanelOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->BringIndexIntoView(index));
    }
    else
    {
        IFC(BringIndexIntoView(index));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::VirtualizingPanelGenerated::InsertInternalChild(INT index, _In_ ABI::Microsoft::UI::Xaml::IUIElement* pChild)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_InsertInternalChild", 0);
    }
    ARG_NOTNULL(pChild, "child");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<VirtualizingPanel*>(this)->InsertInternalChildImpl(index, pChild));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_InsertInternalChild", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::VirtualizingPanelGenerated::OnClearChildren()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_OnClearChildren", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<VirtualizingPanel*>(this)->OnClearChildrenImpl());
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_OnClearChildren", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::VirtualizingPanelGenerated::OnClearChildrenProtected()
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanelOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->OnClearChildren());
    }
    else
    {
        IFC(OnClearChildren());
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::VirtualizingPanelGenerated::OnItemsChanged(_In_ IInspectable* pSender, _In_ ABI::Microsoft::UI::Xaml::Controls::Primitives::IItemsChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_OnItemsChanged", 0);
    }
    ARG_NOTNULL(pSender, "sender");
    ARG_NOTNULL(pArgs, "args");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<VirtualizingPanel*>(this)->OnItemsChangedImpl(pSender, pArgs));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_OnItemsChanged", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::VirtualizingPanelGenerated::OnItemsChangedProtected(_In_ IInspectable* pSender, _In_ ABI::Microsoft::UI::Xaml::Controls::Primitives::IItemsChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanelOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->OnItemsChanged(pSender, pArgs));
    }
    else
    {
        IFC(OnItemsChanged(pSender, pArgs));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::VirtualizingPanelGenerated::RemoveInternalChildRange(INT index, INT range)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_RemoveInternalChildRange", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<VirtualizingPanel*>(this)->RemoveInternalChildRangeImpl(index, range));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "VirtualizingPanel_RemoveInternalChildRange", hr);
    }
    RRETURN(hr);
}

HRESULT DirectUI::VirtualizingPanelFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanelFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IVirtualizingPanelFactory*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableAbstractCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.

// Dependency properties.

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_VirtualizingPanel()
    {
        RRETURN(ctl::ActivationFactoryCreator<VirtualizingPanelFactory>::CreateActivationFactory());
    }
}
