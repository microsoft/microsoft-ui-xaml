// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TreeViewItemAutomationPeer.g.h"

class TreeViewItemAutomationPeer :
    public ReferenceTracker<TreeViewItemAutomationPeer,
        winrt::implementation::TreeViewItemAutomationPeerT,
        winrt::ISelectionItemProvider>
{
public:
    explicit TreeViewItemAutomationPeer(winrt::TreeViewItem const& owner);

    // IExpandCollapseProvider
    winrt::ExpandCollapseState ExpandCollapseState();
    void Collapse();
    void Expand();

    // IAutomationPeerOverrides
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetNameCore();
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    winrt::hstring GetClassNameCore();

    // IAutomationPeerOverrides3
    int32_t GetPositionInSetCore();
    int32_t GetSizeOfSetCore();
    int32_t GetLevelCore();

    void RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState newState);

    // ISelectionItemProvider
    bool IsSelected();
    winrt::IRawElementProviderSimple SelectionContainer();
    void AddToSelection();
    void RemoveFromSelection();
    void Select();

private:
    winrt::ListView GetParentListView();
    winrt::TreeView GetParentTreeView();
    winrt::TreeViewNode GetTreeViewNode();
    void UpdateSelection(bool select);
};

