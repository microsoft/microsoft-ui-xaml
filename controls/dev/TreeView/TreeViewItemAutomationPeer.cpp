// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TreeView.h"
#include "TreeViewItem.h"
#include "TreeViewItemAutomationPeer.h"
#include "SharedHelpers.h"

#include "TreeViewItemAutomationPeer.properties.cpp"

TreeViewItemAutomationPeer::TreeViewItemAutomationPeer(winrt::TreeViewItem const& owner) :
    ReferenceTracker(owner)
{
}

// IExpandCollapseProvider 
winrt::ExpandCollapseState TreeViewItemAutomationPeer::ExpandCollapseState()
{
    auto targetNode = GetTreeViewNode();
    if (targetNode && targetNode.HasChildren())
    {
        if (targetNode.IsExpanded())
        {
            return winrt::ExpandCollapseState::Expanded;
        }
        return winrt::ExpandCollapseState::Collapsed;
    }   
    return winrt::ExpandCollapseState::LeafNode;
}

void TreeViewItemAutomationPeer::Collapse()
{
    if (auto ancestorTreeView = GetParentTreeView())
    {
        if (auto targetNode = GetTreeViewNode())
        {
            ancestorTreeView.Collapse(targetNode);
            RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState::Collapsed);
        }
    }
}

void TreeViewItemAutomationPeer::Expand()
{
    if (auto ancestorTreeView = GetParentTreeView())
    {
        if (auto targetNode = GetTreeViewNode())
        {
            ancestorTreeView.Expand(targetNode);
            RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState::Expanded);
        }
    }
}

// IAutomationPeerOverrides
winrt::IInspectable TreeViewItemAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::ExpandCollapse)
    {
        return *this;
    }

    if (auto treeView = GetParentTreeView())
    {
        if (patternInterface == winrt::PatternInterface::SelectionItem && treeView.SelectionMode() != winrt::TreeViewSelectionMode::None)
        {
            return *this;
        }
    }

    return __super::GetPatternCore(patternInterface);
}


winrt::AutomationControlType TreeViewItemAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::TreeItem;
}

winrt::hstring TreeViewItemAutomationPeer::GetNameCore()
{
    //Check to see if the item has a defined Automation Name
    winrt::hstring nameHString = __super::GetNameCore();

    if (nameHString.empty())
    {
        auto treeViewNode = GetTreeViewNode();
        if (treeViewNode)
        {
            nameHString = SharedHelpers::TryGetStringRepresentationFromObject(treeViewNode.Content());
        }

        if (nameHString.empty())
        {
            nameHString = L"TreeViewNode";
        }
    }

    return nameHString;
}

winrt::hstring TreeViewItemAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::TreeViewItem>();
}

// IAutomationPeerOverrides3
int32_t TreeViewItemAutomationPeer::GetPositionInSetCore()
{
    winrt::ListView ancestorListView = GetParentListView();
    auto targetNode = GetTreeViewNode();
    int positionInSet = 0;

    if (ancestorListView && targetNode)
    {
        if (const auto targetParentNode = targetNode.Parent())
        {
            UINT32 position = 0;
            if (targetParentNode.Children().IndexOf(targetNode, position))
            {
                positionInSet = static_cast<int>(position) + 1;
            }
        }
    }

    return positionInSet;
}

int32_t TreeViewItemAutomationPeer::GetSizeOfSetCore()
{
    winrt::ListView ancestorListView = GetParentListView();
    auto targetNode = GetTreeViewNode();
    int setSize = 0;

    if (ancestorListView && targetNode)
    {
        if (const auto targetParentNode = targetNode.Parent())
        {
            UINT32 size = targetParentNode.Children().Size();
            setSize = static_cast<int>(size);
        }
    }

    return setSize;
}

int32_t TreeViewItemAutomationPeer::GetLevelCore()
{
    winrt::ListView ancestorListView = GetParentListView();
    auto targetNode = GetTreeViewNode();
    int level = -1;

    if (ancestorListView && targetNode)
    {
        level = targetNode.Depth();
        level++;
    }

    return level;
}

void TreeViewItemAutomationPeer::RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState newState)
{
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        winrt::ExpandCollapseState oldState;
        auto expandCollapseStateProperty = winrt::ExpandCollapsePatternIdentifiers::ExpandCollapseStateProperty();

        if (newState == winrt::ExpandCollapseState::Expanded)
        {
            oldState = winrt::ExpandCollapseState::Collapsed;
        }
        else
        {
            oldState = winrt::ExpandCollapseState::Expanded;
        }

        // box_value(oldState) doesn't work here, use ReferenceWithABIRuntimeClassName to make Narrator can unbox it.
        RaisePropertyChangedEvent(expandCollapseStateProperty, box_value(oldState), box_value(newState));
    }
}

// ISelectionItemProvider
bool TreeViewItemAutomationPeer::IsSelected()
{
    auto treeViewItem = winrt::get_self<TreeViewItem>(Owner().as<winrt::TreeViewItem>());
    return treeViewItem->IsSelectedInternal();
}

winrt::IRawElementProviderSimple TreeViewItemAutomationPeer::SelectionContainer()
{
    winrt::IRawElementProviderSimple provider{ nullptr };
    if (auto listView = GetParentListView())
    {
        if (auto peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(listView))
        {
            provider = ProviderFromPeer(peer);
        }
    }

    return provider;
}

void TreeViewItemAutomationPeer::AddToSelection()
{
    UpdateSelection(true);
}

void TreeViewItemAutomationPeer::RemoveFromSelection()
{
    UpdateSelection(false);
}

void TreeViewItemAutomationPeer::Select()
{
    UpdateSelection(true);
}

winrt::ListView TreeViewItemAutomationPeer::GetParentListView()
{
    auto treeViewItemAncestor = Owner().as<winrt::DependencyObject>();
    winrt::ListView ancestorListView{ nullptr };

    while (treeViewItemAncestor && !ancestorListView)
    {
        treeViewItemAncestor = winrt::VisualTreeHelper::GetParent(treeViewItemAncestor);
        if (treeViewItemAncestor != nullptr)
        {
            ancestorListView = treeViewItemAncestor.try_as<winrt::ListView>();
        }
    }

    return ancestorListView;
}

winrt::TreeView TreeViewItemAutomationPeer::GetParentTreeView()
{
    auto treeViewItemAncestor = static_cast<winrt::DependencyObject>(Owner());
    winrt::TreeView ancestorTreeView{ nullptr };

    while (treeViewItemAncestor && !ancestorTreeView)
    {
        treeViewItemAncestor = winrt::VisualTreeHelper::GetParent(treeViewItemAncestor);
        if (treeViewItemAncestor)
        {
            ancestorTreeView = treeViewItemAncestor.try_as<winrt::TreeView>();
        }
    }

    return ancestorTreeView;
}

winrt::TreeViewNode TreeViewItemAutomationPeer::GetTreeViewNode()
{
    winrt::TreeViewNode targetNode{ nullptr };
    if (auto treeview = GetParentTreeView())
    {
        targetNode = treeview.NodeFromContainer(Owner());
    }
    return targetNode;
}

void TreeViewItemAutomationPeer::UpdateSelection(bool select)
{
    if (auto treeItem = Owner().try_as<winrt::TreeViewItem>())
    {
        auto impl = winrt::get_self<TreeViewItem>(treeItem);
        impl->UpdateSelection(select);
    }
}
