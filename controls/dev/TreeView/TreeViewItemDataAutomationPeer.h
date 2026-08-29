// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TreeViewItemDataAutomationPeer.g.h"

class TreeViewItemDataAutomationPeer :
    public ReferenceTracker<TreeViewItemDataAutomationPeer,
        winrt::implementation::TreeViewItemDataAutomationPeerT>
{
public:
    TreeViewItemDataAutomationPeer(winrt::IInspectable const& item, winrt::TreeViewListAutomationPeer const& parent);

    // IExpandCollapseProvider
    winrt::ExpandCollapseState ExpandCollapseState();
    void Collapse();
    void Expand();

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);

private:
    winrt::TreeViewItemAutomationPeer GetTreeViewItemAutomationPeer();
};

