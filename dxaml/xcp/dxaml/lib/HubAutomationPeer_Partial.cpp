// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HubAutomationPeer.g.h"
#include "Hub.g.h"
#include "Panel.g.h"
#include "ScrollViewer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT HubAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IHub* pOwner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IHubAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IHubAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* pOwnerAsUIE = NULL;

    IFCPTR(ppInstance);
    *ppInstance = NULL;

    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    if (ppInner)
    {
        *ppInner = NULL;
    }
    IFCPTR(pOwner);
    IFC(ctl::do_query_interface(pOwnerAsUIE, pOwner));

    IFC(ActivateInstance(pOuter,
            static_cast<Hub*>(pOwner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<HubAutomationPeer*>(pInstance)->put_Owner(pOwnerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(pOwnerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

IFACEMETHODIMP HubAutomationPeer::GetChildrenCore(
    _Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spAPChildren;
    ctl::ComPtr<IUIElement> spOwner;
    ctl::ComPtr<IFrameworkElement> spHeaderHostPart;
    ctl::ComPtr<IPanel> spPanelPart;

    IFCPTR(ppReturnValue);

    IFC(ctl::make(&spAPChildren));

    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner);

    // First child should be the Hub.Header.
    IFC(spOwner.Cast<Hub>()->GetHeaderHostPart(&spHeaderHostPart));
    if (spHeaderHostPart)
    {
        IFC(FrameworkElementAutomationPeer::GetAutomationPeerChildren(spHeaderHostPart.Cast<FrameworkElement>(), spAPChildren.Get()));
    }

    // Iterate over the Panel template part's AP children, which will be the HubSectionAPs.
    IFC(spOwner.Cast<Hub>()->GetPanelPart(&spPanelPart));
    if (spPanelPart)
    {
        IFC(FrameworkElementAutomationPeer::GetAutomationPeerChildren(spPanelPart.Cast<Panel>(), spAPChildren.Get()));
    }

    IFC(spAPChildren.MoveTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppReturnValue);
    *ppReturnValue = NULL;

    if (patternInterface == xaml_automation_peers::PatternInterface_Scroll)
    {
        ctl::ComPtr<IUIElement> spOwner;
        ctl::ComPtr<IScrollViewer> spScrollViewerPart;

        IFC(get_Owner(&spOwner));
        IFCPTR(spOwner);

        IFC(spOwner.Cast<Hub>()->GetScrollViewerPart(&spScrollViewerPart));
        if (spScrollViewerPart)
        {
            ctl::ComPtr<IAutomationPeer> spAutomationPeer;
            ctl::ComPtr<xaml_automation::Provider::IScrollProvider> spScrollProvider;

            IFC(spScrollViewerPart.Cast<ScrollViewer>()->GetOrCreateAutomationPeer(&spAutomationPeer));
            spScrollProvider = spAutomationPeer.AsOrNull<xaml_automation::Provider::IScrollProvider>();
            if (spScrollProvider)
            {
                IFC(spScrollProvider.CopyTo(ppReturnValue));
                IFC(spAutomationPeer.Cast<AutomationPeer>()->put_EventsSource(this));
            }
        }
    }
    else
    {
        IFC(HubAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    *returnValue = NULL;

    IFC(HubAutomationPeerGenerated::GetNameCore(returnValue));
    if (*returnValue == NULL)
    {
        ctl::ComPtr<IUIElement> spOwner;
        ctl::ComPtr<IInspectable> spHeader;

        IFC(get_Owner(&spOwner));
        IFCPTR(spOwner);

        IFC(spOwner.Cast<Hub>()->get_Header(&spHeader));
        if (spHeader)
        {
            wf::PropertyType inspectablePropertyType;

            IFC(ctl::do_get_property_type(spHeader.Get(), &inspectablePropertyType));
            if (inspectablePropertyType == wf::PropertyType::PropertyType_String)
            {
                wrl_wrappers::HString strAutomationName;

                IFC(ctl::do_get_value(*strAutomationName.GetAddressOf(), spHeader.Get()));
                IFC(strAutomationName.CopyTo(returnValue));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


IFACEMETHODIMP HubAutomationPeer::GetClassNameCore(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pReturnValue);

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Hub")).CopyTo(pReturnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pReturnValue);
    *pReturnValue = xaml_automation_peers::AutomationControlType_Group;

Cleanup:
    RRETURN(hr);
}
