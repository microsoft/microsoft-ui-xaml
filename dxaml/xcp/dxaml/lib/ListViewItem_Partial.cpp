// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewItem.g.h"
#include "ListViewItemAutomationPeer.g.h"
#include "ListViewItemTemplateSettings.g.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Create ListViewItemAutomationPeer to represent the ListViewItem.
IFACEMETHODIMP ListViewItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IListViewItemAutomationPeer> spListViewItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IListViewItemAutomationPeerFactory> spListViewItemAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ListViewItemAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As<xaml_automation_peers::IListViewItemAutomationPeerFactory>(&spListViewItemAPFactory));

    IFC(spListViewItemAPFactory.Cast<ListViewItemAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spListViewItemAutomationPeer));
    IFC(spListViewItemAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Sets up instances that are expected on this type.
_Check_return_ HRESULT ListViewItem::PrepareState()
{
    HRESULT hr = S_OK;

    // Create our TemplateSettings.
    ctl::ComPtr<ListViewItemTemplateSettings> spTemplateSettings;

    IFC(ListViewItemGenerated::PrepareState());
    IFC(ctl::make<ListViewItemTemplateSettings>(&spTemplateSettings));
    IFC(put_TemplateSettings(spTemplateSettings.Get()));

Cleanup:
    RRETURN(hr);
}

// Sets the value to display as the dragged items count.
_Check_return_ HRESULT ListViewItem::SetDragItemsCountDisplay(
    _In_ UINT dragItemsCount)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IListViewItemTemplateSettings> spSettings;

    IFC(ListViewBaseItem::SetDragItemsCountDisplay(dragItemsCount));
    IFC(get_TemplateSettings(&spSettings));
    IFC(spSettings.Cast<ListViewItemTemplateSettings>()->put_DragItemsCount(dragItemsCount));

Cleanup:
    RRETURN(hr);
}
