// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GridViewHeaderItem.g.h"
#include "GridViewHeaderItemAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Create GridViewHeaderItemAutomationPeer to represent the GridViewHeaderItem.
IFACEMETHODIMP GridViewHeaderItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IGridViewHeaderItemAutomationPeer> spGridViewHeaderItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IGridViewHeaderItemAutomationPeerFactory> spGridViewHeaderItemAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    IFCPTR(ppAutomationPeer);
    *ppAutomationPeer = nullptr;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::GridViewHeaderItemAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spGridViewHeaderItemAPFactory));

    IFC(spGridViewHeaderItemAPFactory.Cast<GridViewHeaderItemAutomationPeerFactory>()->CreateInstanceWithOwner(this, 
        nullptr, 
        &spInner, 
        &spGridViewHeaderItemAutomationPeer));
    IFC(spGridViewHeaderItemAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}
