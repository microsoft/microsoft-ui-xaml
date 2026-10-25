// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewSource2.g.h"
#include "Shaping/ShapingManager.h"

// Scaffold for the shaping redesign. Exposes the same fluent verb surface as TableViewSource;
// From() constructs a working instance, and every shaping verb throws hresult_not_implemented
// until the new engine is wired in. Shaping state lives in ShapingManager (the coordinator);
// this class validates arguments and forwards.
//
// Reference-tracked for the same reasons as TableViewSource: final_release runs on the owning
// UI thread, and tracker_ref members let the GC walk references to app-supplied objects.
class TableViewSource2 :
    public ReferenceTracker<TableViewSource2, winrt::implementation::TableViewSource2T, winrt::composable, winrt::composing>
{
public:
    // Not activatable in IDL: instances are created only through From(items).
    explicit TableViewSource2(winrt::IInspectable const& items);
    ~TableViewSource2();

    static winrt::TableViewSource2 From(winrt::IInspectable const& items);

    winrt::TableViewSource2 Filter(winrt::TableViewPredicate2 const& predicate);
    winrt::TableViewSource2 GroupBy(winrt::TableViewKeySelector2 const& key, winrt::SortDirection2 groupDirection);
    winrt::TableViewSource2 Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection2 direction);
    winrt::TableViewSource2 Sort(winrt::TableViewKeySelector2 const& key, winrt::SortDirection2 direction);
    winrt::TableViewSource2 Sort(winrt::TableViewComparer2 const& comparer);
    winrt::TableViewSource2 ParentBy(winrt::TableViewKeySelector2 const& key, winrt::TableViewKeySelector2 const& parentKey);
    winrt::TableViewSource2 ChildrenBy(winrt::TableViewChildrenSelector2 const& childrenSelector);
    winrt::TableViewSource2 ClearFilter();
    winrt::TableViewSource2 ClearGroupBy();
    winrt::TableViewSource2 ClearSort();
    winrt::TableViewSource2 ClearHierarchy();

    void Refresh();

    // Forwards to ShapingManager, which forwards to SourceTable (owner of the flag and the
    // per-item subscriptions).
    bool IsLiveShaping() const;
    void IsLiveShaping(bool value);

private:
    [[noreturn]] static void ThrowNotImplemented(wchar_t const* verb);

    // The app's source collection, kept for the engine that will consume it.
    tracker_ref<winrt::IInspectable> m_items{ this };

    // The coordinator: owns SourceTable, the desired spec, the delegate registry, and later the
    // engine, flattener and View. Its tracker_refs are bound to this, so ~TableViewSource2() calls
    // m_shapingManager->Clear() while this object is still alive.
    std::unique_ptr<ShapingManager> m_shapingManager;
};
