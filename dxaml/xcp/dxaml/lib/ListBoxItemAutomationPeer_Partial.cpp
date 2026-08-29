// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListBoxItemAutomationPeer.g.h"
#include "ListBoxItem.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ListBoxItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IListBoxItem* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IListBoxItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IListBoxItemAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<ListBoxItem*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ListBoxItemAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the ListBoxItemAutomationPeer class.
ListBoxItemAutomationPeer::ListBoxItemAutomationPeer()
{
}

// Deconstructor
ListBoxItemAutomationPeer::~ListBoxItemAutomationPeer()
{
}

IFACEMETHODIMP ListBoxItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ListBoxItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListBoxItemAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_ListItem;
    RRETURN(S_OK);
}
