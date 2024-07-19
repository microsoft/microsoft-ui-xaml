// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GridViewItem.g.h"
#include "GridViewItemAutomationPeer.g.h"
#include "GridViewItemTemplateSettings.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Create GridViewItemAutomationPeer to represent the GridViewItem.
IFACEMETHODIMP GridViewItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IGridViewItemAutomationPeer> spGridViewItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IGridViewItemAutomationPeerFactory> spGridViewItemAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::GridViewItemAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As<xaml_automation_peers::IGridViewItemAutomationPeerFactory>(&spGridViewItemAPFactory));

    IFC(spGridViewItemAPFactory.Cast<GridViewItemAutomationPeerFactory>()->CreateInstanceWithOwner(this, 
        NULL, 
        &spInner, 
        &spGridViewItemAutomationPeer));
    IFC(spGridViewItemAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Sets up instances that are expected on this type.
_Check_return_ HRESULT GridViewItem::PrepareState()
{
    HRESULT hr = S_OK;
    
    // Create our TemplateSettings.
    ctl::ComPtr<GridViewItemTemplateSettings> spTemplateSettings;

    IFC(GridViewItemGenerated::PrepareState());
    IFC(ctl::make<GridViewItemTemplateSettings>(&spTemplateSettings));
    IFC(put_TemplateSettings(spTemplateSettings.Get()));
    
Cleanup:
    RRETURN(hr);
}

// Sets the value to display as the dragged items count.
_Check_return_ HRESULT GridViewItem::SetDragItemsCountDisplay(
    _In_ UINT dragItemsCount)
{
    HRESULT hr = S_OK;
    
    ctl::ComPtr<IGridViewItemTemplateSettings> spSettings;
    
    IFC(ListViewBaseItem::SetDragItemsCountDisplay(dragItemsCount));
    IFC(get_TemplateSettings(&spSettings));
    IFC(spSettings.Cast<GridViewItemTemplateSettings>()->put_DragItemsCount(dragItemsCount));
    
Cleanup:
    RRETURN(hr);
}
