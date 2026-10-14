// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableView.h"
#include "TableViewColumnHeaderAutomationPeer.g.h"

#include <cstdint>

// UIA peer for a TableView column header; reports header name, type, identity, and bounds.
// Cached/public peers are owned by the TableView so headers can be enumerated before the
// templates realize; realized header cells use the same per-column identity.
class TableViewColumnHeaderAutomationPeer :
    public ReferenceTracker<TableViewColumnHeaderAutomationPeer, winrt::implementation::TableViewColumnHeaderAutomationPeerT>
{
public:
    TableViewColumnHeaderAutomationPeer(winrt::TableView const& owner, winrt::TableViewColumn const& column);
    TableViewColumnHeaderAutomationPeer(winrt::FrameworkElement const& header,
        winrt::TableView const& table, winrt::TableViewColumn const& column);

    // IAutomationPeerOverrides
    hstring GetClassNameCore();
    hstring GetNameCore();
    bool IsEnabledCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    hstring GetAutomationIdCore();
    // Reports the column's current sort state, so a screen-reader user can tell a sorted header
    // from an unsorted one without relying on the chevron.
    hstring GetHelpTextCore();
    // Invoke rather than Toggle: the sort cycle is not a two-state toggle, and the number of
    // states depends on the column's SortCycle. Only offered for a column the control will
    // actually sort - see GetPatternCore.
    winrt::IInspectable GetPatternCore(winrt::PatternInterface patternInterface);

    // IInvokeProvider
    void Invoke();
    // Complements the distinct RuntimeId: AT announces "column i of n" for spatial context.
    int32_t GetPositionInSetCore();
    int32_t GetSizeOfSetCore();

    // Prevent TableView-owned header peers from exposing the whole TableView subtree.
    winrt::Windows::Foundation::Collections::IVector<winrt::AutomationPeer> GetChildrenCore();
    winrt::Windows::Foundation::Rect GetBoundingRectangleCore();
    winrt::Windows::Foundation::Point GetClickablePointCore();
    bool IsOffscreenCore();

    bool IsTableViewOwned();

private:
    winrt::FrameworkElement GetHeaderElement();
    bool IsSortableColumn();
    int32_t GetColumnIndex();

    winrt::weak_ref<winrt::TableViewColumn> m_column{ nullptr };
    winrt::weak_ref<winrt::TableView> m_table{ nullptr };
    uint32_t m_columnAutomationIdentity{ 0 };
};
