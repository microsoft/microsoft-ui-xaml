// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewSource2.g.h"

// Scaffold for the shaping redesign. Exposes the same fluent verb surface as TableViewSource;
// From() constructs a working instance, and every shaping verb throws hresult_not_implemented
// until the new engine (ShapeSpec / ShapingEngine / ShapingCoordinator) is wired in.
//
// Reference-tracked for the same reasons as TableViewSource: final_release runs on the owning
// UI thread, and tracker_ref members let the GC walk references to app-supplied objects.
class TableViewSource2 :
    public ReferenceTracker<TableViewSource2, winrt::implementation::TableViewSource2T, winrt::composable, winrt::composing>
{
public:
    // Not activatable in IDL: instances are created only through From(items).
    explicit TableViewSource2(winrt::IInspectable const& items);

    static winrt::TableViewSource2 From(winrt::IInspectable const& items);

    winrt::TableViewSource2 Filter(winrt::TableViewPredicate const& predicate);
    winrt::TableViewSource2 GroupBy(winrt::TableViewKeySelector const& key);
    winrt::TableViewSource2 GroupBy(winrt::TableViewKeySelector const& key, winrt::TableViewIdentitySelector const& groupIdentitySelector);
    winrt::TableViewSource2 Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction);
    winrt::TableViewSource2 Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction);
    winrt::TableViewSource2 ClearFilter();
    winrt::TableViewSource2 ClearGroupBy();
    winrt::TableViewSource2 ClearSort();

private:
    [[noreturn]] static void ThrowNotImplemented(wchar_t const* verb);

    // The app's source collection, kept for the engine that will consume it.
    tracker_ref<winrt::IInspectable> m_items{ this };
};
