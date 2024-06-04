// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListBoxAutomationPeer.g.h"
#include "ListBox.g.h"
#include "ListBoxItemDataAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ListBoxAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IListBox* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IListBoxAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IListBoxAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<ListBox*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ListBoxAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the ListBoxAutomationPeer class.
ListBoxAutomationPeer::ListBoxAutomationPeer()
{
}

// Deconstructor
ListBoxAutomationPeer::~ListBoxAutomationPeer()
{
}

IFACEMETHODIMP ListBoxAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ListBox")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListBoxAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_List;
    RRETURN(S_OK);
}

_Check_return_ HRESULT ListBoxAutomationPeer::OnCreateItemAutomationPeerImpl(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IListBoxItemDataAutomationPeer* pListBoxItemDataAutomationPeer = NULL;
    xaml_automation_peers::IListBoxItemDataAutomationPeerFactory* pListBoxItemDataAPFactory = NULL;
    IActivationFactory* pActivationFactory = NULL;
    IInspectable* inner = NULL;
    
    pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::ListBoxItemDataAutomationPeerFactory>::CreateActivationFactory();
    IFC(ctl::do_query_interface(pListBoxItemDataAPFactory, pActivationFactory));

    IFC(static_cast<ListBoxItemDataAutomationPeerFactory*>(pListBoxItemDataAPFactory)->CreateInstanceWithParentAndItem(item,
        this,
        NULL, 
        &inner, 
        &pListBoxItemDataAutomationPeer));
    IFC(ctl::do_query_interface(*returnValue, pListBoxItemDataAutomationPeer));

Cleanup:
    ReleaseInterface(pListBoxItemDataAutomationPeer);
    ReleaseInterface(pListBoxItemDataAPFactory);
    ReleaseInterface(pActivationFactory);
    ReleaseInterface(inner);
    RRETURN(hr);
}
