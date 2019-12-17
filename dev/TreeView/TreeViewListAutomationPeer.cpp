// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TreeViewList.h"
#include "TreeViewListAutomationPeer.h"
#include "TreeViewItemDataAutomationPeer.h"

#include "TreeViewListAutomationPeer.properties.cpp"

TreeViewListAutomationPeer::TreeViewListAutomationPeer(winrt::TreeViewList const& owner)
    : ReferenceTracker(owner)
{
}

//IItemsControlAutomationPeerOverrides2
winrt::ItemAutomationPeer TreeViewListAutomationPeer::OnCreateItemAutomationPeer(winrt::IInspectable const& item)
{
    winrt::TreeViewItemDataAutomationPeer itemPeer{ item, *this };
    return itemPeer;
}

//DropTargetProvider
winrt::hstring TreeViewListAutomationPeer::DropEffect()
{
    return winrt::get_self<TreeViewList>(Owner().as<winrt::TreeViewList>())->GetDropTargetDropEffect();
}

winrt::com_array<winrt::hstring> TreeViewListAutomationPeer::DropEffects()
{
    throw winrt::hresult_not_implemented();
}

winrt::IInspectable TreeViewListAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::DropTarget ||
       (patternInterface == winrt::PatternInterface::Selection && IsMultiselect()))
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

winrt::AutomationControlType TreeViewListAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Tree;
}

// ISelectionProvider
bool TreeViewListAutomationPeer::CanSelectMultiple()
{
    return IsMultiselect()? true: __super::CanSelectMultiple();
}

bool TreeViewListAutomationPeer::IsSelectionRequried()
{
    return IsMultiselect()? false : __super::CanSelectMultiple();
}

winrt::com_array<winrt::Windows::UI::Xaml::Automation::Provider::IRawElementProviderSimple> TreeViewListAutomationPeer::GetSelection()
{
    // The selected items might be collapsed, virtualized, so getting an accurate list of selected items is not possible.
    return {};
}

bool TreeViewListAutomationPeer::IsMultiselect()
{
    return winrt::get_self<TreeViewList>(Owner().as<winrt::TreeViewList>())->IsMultiselect();
}
