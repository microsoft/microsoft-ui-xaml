// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewHeaderItem.g.h"
#include "ListViewHeaderItemAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Create ListViewHeaderItemAutomationPeer to represent the ListViewHeaderItem.
IFACEMETHODIMP ListViewHeaderItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IListViewHeaderItemAutomationPeer> spListViewHeaderItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IListViewHeaderItemAutomationPeerFactory> spListViewHeaderItemAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    IFCPTR(ppAutomationPeer);
    *ppAutomationPeer = nullptr;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ListViewHeaderItemAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spListViewHeaderItemAPFactory));

    IFC(spListViewHeaderItemAPFactory.Cast<ListViewHeaderItemAutomationPeerFactory>()->CreateInstanceWithOwner(this, 
        nullptr, 
        &spInner, 
        &spListViewHeaderItemAutomationPeer));
    IFC(spListViewHeaderItemAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}
