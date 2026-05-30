// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HubSectionAutomationPeer.g.h"
#include "HubSection.g.h"
#include "Button.g.h"
#include "ContentPresenter.g.h"
#include "Hub.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT HubSectionAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IHubSection* pOwner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IHubSectionAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IHubSectionAutomationPeer* pInstance = NULL;
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
            static_cast<HubSection*>(pOwner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<HubSectionAutomationPeer*>(pInstance)->put_Owner(pOwnerAsUIE));

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

IFACEMETHODIMP HubSectionAutomationPeer::GetChildrenCore(
    _Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spAPChildren;
    ctl::ComPtr<IUIElement> spOwner;
    ctl::ComPtr<IButton> spHeaderButtonPart;        // header template part for windows Hub
    ctl::ComPtr<IFrameworkElement> spHeaderPart;    // header template part for phone Hub
    ctl::ComPtr<IContentPresenter> spContentPresenterPart;

    IFCPTR_RETURN(ppReturnValue);

    IFC_RETURN(ctl::make(&spAPChildren));

    IFC_RETURN(get_Owner(&spOwner));
    IFCPTR_RETURN(spOwner);

    IFC_RETURN(spOwner.Cast<HubSection>()->GetHeaderButtonPart(&spHeaderButtonPart));
    if (spHeaderButtonPart)
    {
        IFC_RETURN(spHeaderButtonPart.As<IFrameworkElement>(&spHeaderPart));
    }

    if (spHeaderPart)
    {
        BOOLEAN isHeaderInteractive = FALSE;

        // First, add the HubSection.Header (and header button plus the "see more button" if it's interactive) to the AP children.
        IFC_RETURN(spOwner.Cast<HubSection>()->get_IsHeaderInteractive(&isHeaderInteractive));
        if (isHeaderInteractive)
        {
            // If HubSection.IsHeaderInteractive is TRUE, then we will include the HeaderButton and the "see more" button
            // in our AP children since they support invoke.
            ctl::ComPtr<IAutomationPeer> spHeaderAutomationPeer;

            IFC_RETURN(spHeaderPart.Cast<FrameworkElement>()->GetOrCreateAutomationPeer(&spHeaderAutomationPeer));
            if (spHeaderAutomationPeer)
            {
                IFC_RETURN(spAPChildren->Append(spHeaderAutomationPeer.Get()));
            }

            ctl::ComPtr<IButton> spSeeMoreButtonPart;
            IFC_RETURN(spOwner.Cast<HubSection>()->GetSeeMoreButtonPart(&spSeeMoreButtonPart));

            if (spSeeMoreButtonPart)
            {
                ctl::ComPtr<IFrameworkElement> spSeeMoreButtonPartAsFE;
                ctl::ComPtr<IAutomationPeer> spSeeMoreButtonAutomationPeer;

                IFC_RETURN(spSeeMoreButtonPart.As(&spSeeMoreButtonPartAsFE));

                IFC_RETURN(spSeeMoreButtonPartAsFE.Cast<FrameworkElement>()->GetOrCreateAutomationPeer(&spSeeMoreButtonAutomationPeer));
                if (spSeeMoreButtonAutomationPeer)
                {
                    IFC_RETURN(spAPChildren->Append(spSeeMoreButtonAutomationPeer.Get()));
                }
            }
        }
        else
        {
            // If HubSection.IsHeaderInteractive is FALSE, then we do not include the button itself in the
            // UIA tree but we do include its children to get the Header/HeaderTemplate.
            IFC_RETURN(FrameworkElementAutomationPeer::GetAutomationPeerChildren(spHeaderPart.Cast<FrameworkElement>(), spAPChildren.Get()));
        }
    }

    // Then, add the HubSection.ContentTemplate to the AP children.
    IFC_RETURN(spOwner.Cast<HubSection>()->GetContentPresenterPart(&spContentPresenterPart));
    if (spContentPresenterPart)
    {
        IFC_RETURN(FrameworkElementAutomationPeer::GetAutomationPeerChildren(spContentPresenterPart.Cast<ContentPresenter>(), spAPChildren.Get()));
    }

    IFC_RETURN(spAPChildren.MoveTo(ppReturnValue));

    return S_OK;
}

IFACEMETHODIMP HubSectionAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    *returnValue = NULL;

    IFC(HubSectionAutomationPeerGenerated::GetNameCore(returnValue));
    if (*returnValue == NULL)
    {
        ctl::ComPtr<IUIElement> spOwner;
        ctl::ComPtr<IInspectable> spHeader;

        IFC(get_Owner(&spOwner));
        IFCPTR(spOwner);

        IFC(spOwner.Cast<HubSection>()->get_Header(&spHeader));
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

IFACEMETHODIMP HubSectionAutomationPeer::GetClassNameCore(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pReturnValue);

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"HubSection")).CopyTo(pReturnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubSectionAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pReturnValue);
    *pReturnValue = xaml_automation_peers::AutomationControlType_ListItem;

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubSectionAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppReturnValue);
    *ppReturnValue = NULL;

    if (patternInterface == xaml_automation_peers::PatternInterface_ScrollItem)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(HubSectionAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT HubSectionAutomationPeer::ScrollIntoViewImpl()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spHubSectionAsUI;
    ctl::ComPtr<IHubSection> spHubSection;
    ctl::ComPtr<Hub> spParentHub;

    IFC(get_Owner(&spHubSectionAsUI));
    IFCEXPECT(spHubSectionAsUI);
    IFC(spHubSectionAsUI.As(&spHubSection));

    IFC(spHubSection.Cast<HubSection>()->GetParentHub(&spParentHub));
    if (spParentHub)
    {
        IFC(spParentHub->ScrollToSection(spHubSection.Get()));
    }

Cleanup:
    RRETURN(hr);
}
