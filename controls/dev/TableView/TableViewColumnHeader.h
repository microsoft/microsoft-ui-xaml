// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewColumnHeader.g.h"

// The owning TableView decides sortability/resizability, wires the gripper and drives the grid
// line; this type hosts those in its template parts and maps state onto VisualStates.
class TableViewColumnHeader :
    public ReferenceTracker<TableViewColumnHeader, winrt::implementation::TableViewColumnHeaderT, winrt::composable>
{
public:
    TableViewColumnHeader();

    void OnApplyTemplate();
    winrt::AutomationPeer OnCreateAutomationPeer();
    winrt::AutomationPeer GetExistingAutomationPeerInternal() const { return m_automationPeer.get(); }

    // None of these mark the event handled, so Tapped-to-sort and focus are unaffected.
    void OnPointerEntered(winrt::PointerRoutedEventArgs const& args);
    void OnPointerExited(winrt::PointerRoutedEventArgs const& args);
    void OnPointerPressed(winrt::PointerRoutedEventArgs const& args);
    void OnPointerReleased(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCanceled(winrt::PointerRoutedEventArgs const& args);

    void SetOwnerInternal(winrt::TableView const& table, winrt::TableViewColumn const& column);
    winrt::TableView GetOwningTableView() const { return m_table.get(); }
    winrt::TableViewColumn Column() const { return m_column.get(); }

    void SetSortableInternal(bool sortable, double indicatorReserve);
    void SetSortDirectionInternal(winrt::SortDirection direction);

    // Returns the gripper only while it is hosted: an unhosted gripper cannot drag.
    void SetResizeGripperInternal(winrt::ResizeGripper const& gripper);
    winrt::ResizeGripper GetResizeGripperInternal() const;

    winrt::ContentPresenter GetContentPresenterInternal() const { return m_contentPresenter.get(); }
    winrt::Border GetGridLineInternal() const { return m_gridLine.get(); }

    // True while Content is the TextBlock the TableView made for a string Header.
    void SetOwnsContentInternal(bool ownsContent) { m_ownsContent = ownsContent; }
    bool OwnsContentInternal() const { return m_ownsContent; }

private:
    void UpdateVisualStates(bool useTransitions);
    void UpdateContentReserve();
    void HostSortIndicator();
    void SyncSortIndicatorForeground(bool followText, bool highlighted);
    void ResetPointerState(winrt::PointerRoutedEventArgs const& args);
    bool IsInsideResizeGripper(winrt::IInspectable const& source);

    winrt::weak_ref<winrt::TableView> m_table{ nullptr };
    winrt::weak_ref<winrt::TableViewColumn> m_column{ nullptr };
    winrt::weak_ref<winrt::AutomationPeer> m_automationPeer{ nullptr };

    tracker_ref<winrt::ContentPresenter> m_contentPresenter{ this };
    tracker_ref<winrt::Border> m_gridLine{ this };
    tracker_ref<winrt::Border> m_sortIndicatorHost{ this };
    tracker_ref<winrt::Border> m_resizeGripperHost{ this };
    tracker_ref<winrt::SortIndicator> m_sortIndicator{ this };
    tracker_ref<winrt::ResizeGripper> m_resizeGripper{ this };

    winrt::Control::IsEnabledChanged_revoker m_isEnabledChangedRevoker{};
    winrt::UIElement::ContextRequested_revoker m_templateRootContextRequestedRevoker{};

    winrt::Thickness m_templateContentMargin{};
    double m_indicatorReserve{ 0.0 };
    bool m_isSortable{ false };
    winrt::SortDirection m_sortDirection{ winrt::SortDirection::None };
    bool m_isPointerOver{ false };
    bool m_isPressed{ false };
    bool m_ownsContent{ false };
};
