// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TreeView.h"
#include "TreeViewItem.h"
#include "TreeViewItemDataAutomationPeer.h"
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>

#include "TreeViewItemDataAutomationPeer.properties.cpp"

TreeViewItemDataAutomationPeer::TreeViewItemDataAutomationPeer(winrt::IInspectable const& item, winrt::TreeViewListAutomationPeer const& parent) :
    ReferenceTracker(item, parent)
{
}

// IExpandCollapseProvider 
winrt::ExpandCollapseState TreeViewItemDataAutomationPeer::ExpandCollapseState()
{
    if (auto peer = GetTreeViewItemAutomationPeer())
    {
        return peer.ExpandCollapseState();
    }
    throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
}

void TreeViewItemDataAutomationPeer::Collapse()
{
    if (auto peer = GetTreeViewItemAutomationPeer())
    {
        return peer.Collapse();
    }
    throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
}

void TreeViewItemDataAutomationPeer::Expand()
{
    if (auto peer = GetTreeViewItemAutomationPeer())
    {
        return peer.Expand();
    }
    throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
}

// IAutomationPeerOverrides
winrt::IInspectable TreeViewItemDataAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::ExpandCollapse)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

winrt::TreeViewItemAutomationPeer TreeViewItemDataAutomationPeer::GetTreeViewItemAutomationPeer()
{
    // ItemsAutomationPeer hold ItemsControlAutomationPeer and Item properties.
    // ItemsControlAutomationPeer -> ItemsControl by ItemsControlAutomationPeer.Owner -> ItemsControl Look up Item to get TreeViewItem -> Get TreeViewItemAutomationPeer 
    if (auto itemsControlAutomationPeer = ItemsControlAutomationPeer())
    {
        if (auto itemsControl = itemsControlAutomationPeer.Owner().try_as<winrt::ItemsControl>())
        {
            if (auto item = itemsControl.ContainerFromItem(Item()).try_as<winrt::UIElement>())
            {
                if (auto treeViewItemAutomationPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(item).try_as<winrt::TreeViewItemAutomationPeer>())
                {
                    return treeViewItemAutomationPeer;
                }
            }
        }
    }
    throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
}
