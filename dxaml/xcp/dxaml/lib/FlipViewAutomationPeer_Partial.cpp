// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FlipViewAutomationPeer.g.h"
#include "FlipView.g.h"
#include "ButtonBase.g.h"
#include "FlipViewItemDataAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT FlipViewAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IFlipView* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IFlipViewAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IFlipViewAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<FlipView*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<FlipViewAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Initializes a new instance of the FlipViewAutomationPeer class.
FlipViewAutomationPeer::FlipViewAutomationPeer()
{
}

// Deconstructor
FlipViewAutomationPeer::~FlipViewAutomationPeer()
{
}

IFACEMETHODIMP FlipViewAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    // FlipView is only adding the current selected item to the UIA tree to prevent narrator focus from going to hidden/off-screen elements
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spAPChildren;    
    ctl::ComPtr<IInspectable> spSelectedItem;

    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;

    IFC_RETURN(get_Owner(&spOwner));
    IFC_RETURN(ctl::make(&spAPChildren));

    IFC_RETURN(spOwner.Cast<FlipView>()->get_SelectedItem(&spSelectedItem));
    if (spSelectedItem)
    {
        ctl::ComPtr<xaml_automation_peers::IItemAutomationPeer> spItemPeer;
        IFC_RETURN(CreateItemAutomationPeer(spSelectedItem.Get(), &spItemPeer));
        if(spItemPeer)
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spItemPeerAsAP;
            IFC_RETURN(spItemPeer.As(&spItemPeerAsAP));
            IFC_RETURN(spAPChildren->Append(spItemPeerAsAP.Get()));

            if(spAPChildren.Get())
            {
                ctl::ComPtr<ButtonBase> spPreviousButton;
                ctl::ComPtr<ButtonBase> spNextButton;
                ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spPreviousButtonAP;
                ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spNextButtonAP;
                IFC_RETURN(spOwner.Cast<FlipView>()->GetPreviousAndNextButtons(&spPreviousButton, &spNextButton));
                if(spNextButton)
                {
                    IFC_RETURN(spNextButton->GetOrCreateAutomationPeer(&spNextButtonAP));
                    if(spNextButtonAP)
                    {
                        IFC_RETURN(spAPChildren->InsertAt(0, spNextButtonAP.Get()));
                    }
                }
                if(spPreviousButton)
                {
                    IFC_RETURN(spPreviousButton->GetOrCreateAutomationPeer(&spPreviousButtonAP));
                    if(spPreviousButtonAP)
                    {
                        IFC_RETURN(spAPChildren->InsertAt(0, spPreviousButtonAP.Get()));
                    }
                }
            }
        }
    }

    *ppReturnValue = spAPChildren.Detach();
    return S_OK;
}

IFACEMETHODIMP FlipViewAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"FlipView")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP FlipViewAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_FlipView;
    RRETURN(S_OK);
}

_Check_return_ HRESULT FlipViewAutomationPeer::OnCreateItemAutomationPeerImpl(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IFlipViewItemDataAutomationPeer* pFlipViewItemDataAutomationPeer = NULL;
    xaml_automation_peers::IFlipViewItemDataAutomationPeerFactory* pFlipViewItemDataAPFactory = NULL;
    IActivationFactory* pActivationFactory = NULL;
    IInspectable* inner = NULL;

    pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::FlipViewItemDataAutomationPeerFactory>::CreateActivationFactory();
    IFC(ctl::do_query_interface(pFlipViewItemDataAPFactory, pActivationFactory));

    IFC(static_cast<FlipViewItemDataAutomationPeerFactory*>(pFlipViewItemDataAPFactory)->CreateInstanceWithParentAndItem(item,
        this,
        NULL,
        &inner,
        &pFlipViewItemDataAutomationPeer));
    IFC(ctl::do_query_interface(*returnValue, pFlipViewItemDataAutomationPeer));

Cleanup:
    ReleaseInterface(pFlipViewItemDataAutomationPeer);
    ReleaseInterface(pFlipViewItemDataAPFactory);
    ReleaseInterface(pActivationFactory);
    ReleaseInterface(inner);
    RRETURN(hr);
}
