// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GridViewItemAutomationPeer.g.h"
#include "GridViewItem.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT GridViewItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IGridViewItem* pOwner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IGridViewItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IGridViewItemAutomationPeer> spInstance = NULL;
    ctl::ComPtr<IInspectable> spInner = NULL;
    ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(pOwner);
    
    IFC(ctl::do_query_interface(spOwnerAsUIE, pOwner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<GridViewItem*>(pOwner)->GetHandle(),
            &spInner));
    IFC(spInner.As<xaml_automation_peers::IGridViewItemAutomationPeer>(&spInstance));
    IFC(spInstance.Cast<GridViewItemAutomationPeer>()->put_Owner(spOwnerAsUIE.Get()));

    if (ppInner)
    {
        IFC(spInner.CopyTo(ppInner));
    }

    IFC(spInstance.CopyTo(ppInstance));

Cleanup:
    RRETURN(hr);
}

// Initializes a new instance of the GridViewItemAutomationPeer class.
GridViewItemAutomationPeer::GridViewItemAutomationPeer()
{
}

// Deconstructor
GridViewItemAutomationPeer::~GridViewItemAutomationPeer()
{
}

IFACEMETHODIMP GridViewItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GridViewItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}
