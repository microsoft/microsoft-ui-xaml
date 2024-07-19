// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      GridView displays a a rich, interactive collection of items in a tabular
//      format.

#include "precomp.h"
#include "GridView.g.h"
#include "GridViewAutomationPeer.g.h"
#include "GridViewItem.g.h"
#include "GridViewHeaderItem.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

GridView::GridView()
{
}

// This method returns information regarding recent activities on GridView Control,
// Activities are whether the items get added/removed/reordered or content changed.
// ListViewBase does all calculation, in this class we just have to choose right enum
// based on the base class calculation. We also have to special-case when animations
// are disabled due to keyboard reordering.
_Check_return_ HRESULT GridView::GetCurrentTransitionContext(
    _In_ INT id,
    _Out_ ThemeTransitionContext* returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    IFC(GridViewGenerated::GetCurrentTransitionContext(id, returnValue));

    switch (*returnValue)
    {
    case ThemeTransitionContext::SingleAddList:
        *returnValue = ThemeTransitionContext::SingleAddGrid;
        break;

    case ThemeTransitionContext::MultipleAddList:
        *returnValue = ThemeTransitionContext::MultipleAddGrid;
        break;

    case ThemeTransitionContext::SingleDeleteList:
        *returnValue = ThemeTransitionContext::SingleDeleteGrid;
        break;

    case ThemeTransitionContext::MultipleDeleteList:
        *returnValue = ThemeTransitionContext::MultipleDeleteGrid;
        break;

    case ThemeTransitionContext::SingleReorderList:
        *returnValue = ThemeTransitionContext::SingleReorderGrid;
        break;

    case ThemeTransitionContext::MultipleReorderList:
        *returnValue = ThemeTransitionContext::MultipleReorderGrid;
        break;
    }

Cleanup:
    RRETURN(hr);
}

// Create GridViewAutomationPeer to represent the GridView.
IFACEMETHODIMP GridView::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IGridViewAutomationPeer> spGridViewAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IGridViewAutomationPeerFactory> spGridViewAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::GridViewAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As<xaml_automation_peers::IGridViewAutomationPeerFactory>(&spGridViewAPFactory));

    IFC(spGridViewAPFactory->CreateInstanceWithOwner(this, 
        NULL, 
        &spInner, 
        &spGridViewAutomationPeer));
        
    IFC(spGridViewAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Determines if the specified item is (or is eligible to be) its own container.
IFACEMETHODIMP GridView::IsItemItsOwnContainerOverride(
    _In_ IInspectable* item, 
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    
    IFCPTR(returnValue);
    
    *returnValue = !!ctl::value_is<IGridViewItem>(item);
    
Cleanup:
    RRETURN(hr);
}

// Creates or identifies the element that is used to display the given item.
IFACEMETHODIMP GridView::GetContainerForItemOverride(
    _Outptr_ IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<GridViewItem> spItem;
    
    IFCPTR(returnValue);
    *returnValue = NULL;
    
    IFC(ctl::make<GridViewItem>(&spItem));
    
    *returnValue = spItem.Detach();
    
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT GridView::GetHeaderForGroupOverrideImpl(_Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<GridViewHeaderItem> spContainer;

    IFCEXPECT(returnValue);
    IFC(ctl::make(&spContainer));

    IFC(spContainer.MoveTo(returnValue));

Cleanup:
    RRETURN(hr);
}
