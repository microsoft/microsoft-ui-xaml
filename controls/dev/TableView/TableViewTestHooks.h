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
    static void SetRowCellRebuildHandler(winrt::Windows::Foundation::EventHandler<winrt::IInspectable> const& handler);

    // Called by TableViewRow at the end of each cell rebuild.
    static void NoteRowCellRebuild(winrt::IInspectable const& row);
};
