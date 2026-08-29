// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GridViewAutomationPeer.g.h"
#include "GridView.g.h"
#include "GridViewItemDataAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT GridViewAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IGridView* pOwner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IGridViewAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IGridViewAutomationPeer> spInstance;
    ctl::ComPtr<IInspectable> spInner;
    ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(pOwner);
    
    IFC(ctl::do_query_interface(spOwnerAsUIE, pOwner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<GridView*>(pOwner)->GetHandle(),
            &spInner));
    IFC(spInner.As<xaml_automation_peers::IGridViewAutomationPeer>(&spInstance));
    IFC(spInstance.Cast<GridViewAutomationPeer>()->put_Owner(spOwnerAsUIE.Get()));

    if (ppInner)
    {
        IFC(spInner.CopyTo(ppInner));
    }

    IFC(spInstance.CopyTo(ppInstance));

Cleanup:
    RRETURN(hr);
}

// Initializes a new instance of the GridViewAutomationPeer class.
GridViewAutomationPeer::GridViewAutomationPeer()
{
}

// Deconstructor
GridViewAutomationPeer::~GridViewAutomationPeer()
{
}

IFACEMETHODIMP GridViewAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GridView")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP GridViewAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_List;
    RRETURN(S_OK);
}

_Check_return_ HRESULT GridViewAutomationPeer::OnCreateItemAutomationPeerImpl(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IGridViewItemDataAutomationPeer> spGridViewItemDataAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IGridViewItemDataAutomationPeerFactory> spGridViewItemDataAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::GridViewItemDataAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As<xaml_automation_peers::IGridViewItemDataAutomationPeerFactory>(&spGridViewItemDataAPFactory));

    IFC(spGridViewItemDataAPFactory.Cast<GridViewItemDataAutomationPeerFactory>()->CreateInstanceWithParentAndItem(item,
        this,
        NULL, 
        &spInner, 
        &spGridViewItemDataAutomationPeer));
    IFC(spGridViewItemDataAutomationPeer.CopyTo<xaml_automation_peers::IItemAutomationPeer>(returnValue));

Cleanup:
    RRETURN(hr);
}
