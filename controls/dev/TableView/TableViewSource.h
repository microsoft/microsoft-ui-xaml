// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <memory>

#include "TableViewSource.g.h"

class TableViewProjectionView;
class ShapedItemsSource;

// The control-facing face of the shaping stack: a fluent verb surface, and the one place that
// decides what a projected row MEANS to a TableView.
//
// It owns no projection. Filtering, sorting, source subscriptions, incremental change
// application, and identity tracking all live in ShapedItemsSource below it; this class
// translates public WinRT delegates into that engine's vocabulary, turns the shape the engine
// produced into an ItemsSourceView plus a row-metadata provider, and tells the owning TableView
// when the projection it cached is no longer the one being shown.
//
// Reference-tracked (like ItemsRepeater's ItemsSourceView) rather than a plain projected class.
// This is the standard WinUI mechanism for a WinRT type that holds references to app-supplied
// (possibly managed) objects: ReferenceTracker gives two things this type needs --
//   1. final_release marshals the delete to the DispatcherQueue captured at construction, so the
//      whole teardown cascade (~TableViewSource -> m_engine drop -> ~ShapedItemsSource -> revoke)
//      always runs on the owning UI thread even when the CLR finalizes this source off-thread.
//      That is why ShapedItemsSource needs no thread guard of its own.
//   2. tracker_ref members let the GC walk this native object's strong refs to managed objects and
//      collect reference cycles.
// Costs: abi_enter enforces UI-thread affinity on every projected call, and each instance composes
// a DependencyObject inner. Both are acceptable for a UI-thread-affine items source.
class TableViewSource :
    public ReferenceTracker<TableViewSource, winrt::implementation::TableViewSourceT, winrt::composable, winrt::composing>
{
public:
    // No parameterless constructor: TableViewSource is not activatable in IDL (only the static
    // From(items) factory is projected). Instances are created via winrt::make with the items ctor.
    ~TableViewSource();
    explicit TableViewSource(winrt::IInspectable const& items);

    static winrt::TableViewSource From(winrt::IInspectable const& items);

    // Single, always-first predicate: each Filter() replaces the previous predicate and runs
    // before sort shaping.
    winrt::TableViewSource Filter(winrt::TableViewPredicate const& predicate);
    winrt::TableViewSource Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction);
    winrt::TableViewSource SortReplacing(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken, winrt::TableViewKeySelector const& key, winrt::SortDirection direction);
    winrt::TableViewSource ClearFilter();
    winrt::TableViewSource ClearSort();
    winrt::TableViewSource ClearSort(winrt::hstring const& sortAxisToken);

    // UI-thread affine after construction/binding: shaping verbs and projection mutation must
    // run on the owning UI thread. Only source change notifications are marshaled back here.
    winrt::IObservableVector<winrt::IInspectable> View();
    winrt::ItemsSourceView GetItemsSourceView();

    // Internal: the owning TableView caches the projection (its ItemsSourceView)
    // when it binds, so it must be told when a shaping verb rewrites that projection. Held
    // weakly - the owner holds this source via ItemsSource.
    //
    // The owner is taken as IInspectable because only its IDENTITY matters here, for the
    // single-owner check below. Naming TableView would invert the dependency (rule 5): this type
    // sits below the control in the shaping stack. For the same reason the owner INSTALLS the
    // handler below rather than being called back into by name - the same shape as the handlers
    // this class installs on its own engine.
    void SetOwningTableView(winrt::IInspectable const& owner);
    // Raised when a shaping verb rewrote the projection. `reorderOnly` is true when membership is
    // unchanged and only the order moved.
    void SetShapingChangedHandler(std::function<void(bool)> handler) { m_shapingChanged = std::move(handler); }

private:
    winrt::TableViewSource SortCore(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken, winrt::TableViewKeySelector const& key, winrt::SortDirection direction);
    // Re-derives everything this class caches from the shape the engine just produced.
    void OnProjectionRebuilt();
    void RefreshViewWrapper();
    // Tells the owner a shaping verb rewrote the projection, so it can raise the UIA
    // structure-changed event that a programmatic reshape has no other trigger for.
    void NotifyOwnerShapingChanged(bool reorderOnly);

    std::shared_ptr<ShapedItemsSource> m_engine{};
    // Weak, and typed as IInspectable rather than TableView: this slot exists only to detect a
    // second owner binding, and the owning TableView references this source through its
    // ItemsSource property, so a strong back-pointer would be a cycle.
    winrt::weak_ref<winrt::IInspectable> m_owningTableView{ nullptr };
    // Installed by the owner in SetOwningTableView's caller and cleared when it detaches, so a
    // source that has been swapped out cannot drive its former owner.
    std::function<void(bool)> m_shapingChanged{};
    winrt::com_ptr<TableViewProjectionView> m_viewWrapper{ nullptr };
    // tracker_ref so the GC can walk this strong reference into the (possibly managed) projected
    // rows and collect any cycle; also swaps to null safely during finalization.
    tracker_ref<winrt::ItemsSourceView> m_itemsSourceView{ this };
};
