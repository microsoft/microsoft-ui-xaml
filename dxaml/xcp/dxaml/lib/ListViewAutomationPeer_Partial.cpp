// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewAutomationPeer.g.h"
#include "ListView.g.h"
#include "ListViewItemDataAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ListViewAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IListView* pOwner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IListViewAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IListViewAutomationPeer> spInstance;
    ctl::ComPtr<IInspectable> spInner;
    ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(pOwner);
    
    IFC(ctl::do_query_interface(spOwnerAsUIE, pOwner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<ListView*>(pOwner)->GetHandle(),
            &spInner));
    IFC(spInner.As<xaml_automation_peers::IListViewAutomationPeer>(&spInstance));
    IFC(spInstance.Cast<ListViewAutomationPeer>()->put_Owner(spOwnerAsUIE.Get()));

    if (ppInner)
    {
        IFC(spInner.CopyTo(ppInner));
    }

    IFC(spInstance.CopyTo(ppInstance));

Cleanup:
    RRETURN(hr);
}

// Initializes a new instance of the ListViewAutomationPeer class.
ListViewAutomationPeer::ListViewAutomationPeer()
{
}

// Deconstructor
ListViewAutomationPeer::~ListViewAutomationPeer()
{
}

IFACEMETHODIMP ListViewAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ListView")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListViewAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_List;
    RRETURN(S_OK);
}

_Check_return_ HRESULT ListViewAutomationPeer::OnCreateItemAutomationPeerImpl(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IListViewItemDataAutomationPeer> spListViewItemDataAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IListViewItemDataAutomationPeerFactory> spListViewItemDataAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ListViewItemDataAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As<xaml_automation_peers::IListViewItemDataAutomationPeerFactory>(&spListViewItemDataAPFactory));

    IFC(spListViewItemDataAPFactory.Cast<ListViewItemDataAutomationPeerFactory>()->CreateInstanceWithParentAndItem(item,
        this,
        NULL, 
        &spInner, 
        &spListViewItemDataAutomationPeer));
    IFC(spListViewItemDataAutomationPeer.CopyTo<xaml_automation_peers::IItemAutomationPeer>(returnValue));

Cleanup:
    RRETURN(hr);
}
