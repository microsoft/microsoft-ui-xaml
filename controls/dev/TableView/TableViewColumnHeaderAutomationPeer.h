// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableView.h"
#include "TableViewColumnHeaderAutomationPeer.g.h"

// UIA peer for a TableView column header; reports header name, type, and bounds.
// Distinct per-column RuntimeIds are not yet implemented; peers share the TableView
// owner-derived identity. TableView ownership allows pre-realization enumeration.
class TableViewColumnHeaderAutomationPeer :
    public ReferenceTracker<TableViewColumnHeaderAutomationPeer, winrt::implementation::TableViewColumnHeaderAutomationPeerT>
{
public:
    TableViewColumnHeaderAutomationPeer(winrt::TableView const& owner, winrt::TableViewColumn const& column);

    // IAutomationPeerOverrides
    hstring GetClassNameCore();
    hstring GetNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    // Header peers share the TableView owner, so RuntimeIds currently collide; expose
    // visible column position so AT can distinguish columns until distinct RuntimeIds are implemented.
    int32_t GetPositionInSetCore();
    int32_t GetSizeOfSetCore();

    // Prevent each header from exposing the whole TableView subtree.
    winrt::Windows::Foundation::Collections::IVector<winrt::AutomationPeer> GetChildrenCore();

    // Resolve bounds from this header's realized cell, not the TableView owner.
    winrt::Windows::Foundation::Rect GetBoundingRectangleCore();
    winrt::Windows::Foundation::Point GetClickablePointCore();

private:
    winrt::FrameworkElement GetHeaderElement();
    int32_t GetColumnIndex();

    winrt::weak_ref<winrt::TableViewColumn> m_column{ nullptr };
};
