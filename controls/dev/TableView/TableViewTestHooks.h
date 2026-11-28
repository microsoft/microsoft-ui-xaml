// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewTestHooks.g.h"

class TableViewTestHooks :
    public winrt::implementation::TableViewTestHooksT<TableViewTestHooks>
{
public:
    static bool BeginColumnReorderDrag(winrt::IInspectable const& tableView, winrt::IInspectable const& column);
    static bool UpdateColumnReorderDrag(winrt::IInspectable const& tableView, double translationX);
    static bool CompleteColumnReorderDrag(winrt::IInspectable const& tableView, double translationX);
    static bool IsColumnReorderDragging(winrt::IInspectable const& tableView);
    static winrt::event_token RowCellRebuilt(winrt::Windows::Foundation::EventHandler<winrt::IInspectable> const& handler);
    static void RowCellRebuilt(winrt::event_token const& token);

    // Called by TableViewRow at the end of each cell rebuild.
    static void NoteRowCellRebuild(winrt::IInspectable const& row);

    static void SetAutomationListenerForced(bool isForced);
    static winrt::event_token RowAutomationPropertyChanged(winrt::Windows::Foundation::EventHandler<winrt::IInspectable> const& handler);
    static void RowAutomationPropertyChanged(winrt::event_token const& token);

    // AutomationPeer::ListenerExists, or true while a test forces it.
    static bool AutomationListenerExists(winrt::AutomationEvents eventId);
    // Called by the row peer for each property change it raises.
    static void NoteRowAutomationPropertyChanged(winrt::IInspectable const& row, wchar_t const* property);

    static uint32_t GetRowHierarchyRestampCount();
    // Called by TableView each time it restamps its realized rows.
    static void NoteRowHierarchyRestamp();
};
