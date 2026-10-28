// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewSource1a.h"
#include "TableViewTip.h"
#include "RowMetadataProvider1a.h"
#include "GroupedSourceAdapter.h"
#include "HierarchicalSourceAdapter1a.h"
#include "ShapedItemsSource1a.h"
#include "ShapingHelpers.h"
#include "SortMemberPathResolver.h"
#include "SharedHelpers.h"

namespace
{
    namespace tabularPrimitives = winrt::Microsoft::UI::Xaml::Controls::Tabular::Primitives::implementation;
}

struct TableViewSource1a::ShapingTip
{
    ShapingTip(TableViewSource1a& source, TableViewTip::Operation kind, bool meaningful = true) :
        source(source), kind(kind), handlers(source.m_shapingTipHandlers),
        previous(std::exchange(source.m_shapingTip, this)), ownerGeneration(source.m_ownerGeneration)
    {
        observation.unproven = previous || source.m_notificationDepth;
        if (previous) { previous->observation.unproven = true; }
        if (handlers)
        {
            generation = handlers->begin(kind, meaningful);
        }
    }

    ~ShapingTip()
    {
        source.m_shapingTip = previous;
        if (handlers)
        {
            auto const stale = ownerGeneration != source.m_ownerGeneration;
            handlers->complete(kind, generation, !stale && observation.Failed(),
                stale ? std::optional{ TableViewTip::IgnoreReason::Stale } : observation.Ignored(),
                observation.error, observation.appFaultKind, observation.contained);
        }
    }

    ShapingTip(ShapingTip const&) = delete;
    ShapingTip& operator=(ShapingTip const&) = delete;

    template<class Callback>
    void Apply(Callback&& callback)
    {
        // The verb counts its own contained delegate faults, however it ends. A tally already running belongs to the verb
        // a delegate started this one in, and goes back to it; with no such verb it is live shaping's or a source
        // change's, not this verb's.
        auto const enclosing = source.m_engine->ExchangeSwallowedFaults({});
        auto takeContained = gsl::finally([this, enclosing]() noexcept
        {
            auto const own = source.m_engine->ExchangeSwallowedFaults(previous ? enclosing : ShapingHelpers::SwallowedFaults{});
            observation.contained = { own.count, own.firstError };
        });
        try
        {
            std::forward<Callback>(callback)();
            observation.completed = true;
        }
        catch (...)
        {
            // The app's collection, its broken contract, or a callout arrive tagged as the app's; TableView's own
            // invalid arguments stay its own.
            auto const fault = TableViewTip::CurrentFault();
            if (!observation.error) { observation.error = fault.error; }
            if (observation.appFaultKind == TableViewTip::AppFaultKind::None) { observation.appFaultKind = fault.appFault; }
            throw;
        }
    }

    TableViewSource1a& source;
    TableViewTip::Operation kind;
    std::shared_ptr<ShapingTipHandlers> handlers;
    ShapingTip* previous{};
    uint64_t ownerGeneration{};
    uint64_t generation{};
    TableViewTip::ShapingObservation observation;
};

void TableViewSource1a::SetShapingTipHandlers(
    winrt::IInspectable const& owner,
    std::function<uint64_t(TableViewTip::Operation, bool)> begin,
    ShapingTipCompleted complete)
{
    if (m_owningTableView.get() != owner) { return; }
    try
    {
        m_shapingTipHandlers = std::make_shared<ShapingTipHandlers>(
            ShapingTipHandlers{ std::move(begin), std::move(complete) });
    }
    catch (...) { OutputDebugStringW(L"TableView TiP: source observation unavailable.\n"); }
}

TableViewSource1a::~TableViewSource1a() = default;

TableViewSource1a::TableViewSource1a(winrt::IInspectable const& items) :
    m_engine(std::make_shared<::pb1a::ShapedItemsSource>(items))
{
    // Diagnostics raised by the engine are contractual for apps written against this type, so
    // keep naming it even though the engine itself must not know about it.
    m_engine->DiagnosticName(L"TableViewSource1a");
    // Handlers are installed before Start() so the very first projection is observed like any
    // later one -- the control never has to special-case its own construction.
    m_engine->SetProjectionRebuiltHandler([this]() { OnProjectionRebuilt(); });
    m_engine->SetShapingChangedHandler([this](bool reorderOnly) { NotifyOwnerShapingChanged(reorderOnly); });
    m_engine->Start();
}

winrt::TableViewSource1a TableViewSource1a::From(winrt::IInspectable const& items)
{
    if (!items)
    {
        throw winrt::hresult_invalid_argument(L"items cannot be null.");
    }

    return winrt::make<::TableViewSource1a>(items);
}

winrt::TableViewSource1a TableViewSource1a::From(winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector)
{
    if (keySelector)
    {
        throw winrt::hresult_not_implemented(L"This TableViewSource1a variant does not support a row key.");
    }
    return From(items);
}

winrt::ITableViewShapingSource TableViewSource1a::Filter(winrt::TableViewPredicate const& predicate)
{
    if (!predicate)
    {
        throw winrt::hresult_invalid_argument(L"predicate cannot be null.");
    }
    ShapingTip tip{ *this, TableViewTip::Operation::Filter };
    tip.Apply([&]()
    {
        m_engine->SetFilter([predicate](winrt::IInspectable const& item) { return predicate(item); });
    });
    return *this;
}

winrt::ITableViewShapingSource TableViewSource1a::GroupBy(winrt::TableViewKeySelector const& key)
{
    return GroupBy(key, nullptr);
}

winrt::ITableViewShapingSource TableViewSource1a::GroupBy(winrt::TableViewKeySelector const& key, winrt::TableViewIdentitySelector const& groupIdentitySelector)
{
    if (!key)
    {
        throw winrt::hresult_invalid_argument(L"key cannot be null.");
    }

    ShapingTip tip{ *this, TableViewTip::Operation::GroupConfiguration };
    RowIdentity::IdentitySelector groupIdentity{ nullptr };
    if (groupIdentitySelector)
    {
        groupIdentity = [groupIdentitySelector](winrt::IInspectable const& item) { return groupIdentitySelector(item); };
    }

    tip.Apply([&]()
    {
        m_engine->SetGroup([key](winrt::IInspectable const& item) { return key(item); }, groupIdentity);
    });
    return *this;
}

// A clear is measured only when the engine holds something to clear. The engine keeps a declaration
// even when re-projecting it throws, so it, not a flag set around the call, is the answer.
winrt::ITableViewShapingSource TableViewSource1a::ClearFilter()
{
    ShapingTip tip{ *this, TableViewTip::Operation::Filter, m_engine->HasFilter() };
    tip.Apply([&]() { m_engine->ClearFilter(); });
    return *this;
}

winrt::ITableViewShapingSource TableViewSource1a::ClearGroupBy()
{
    ShapingTip tip{ *this, TableViewTip::Operation::GroupConfiguration, m_engine->HasGroup() };
    tip.Apply([&]() { m_engine->ClearGroup(); });
    return *this;
}

winrt::ITableViewShapingSource TableViewSource1a::ParentBy(winrt::TableViewKeySelector const& keySelector, winrt::TableViewKeySelector const& parentKeySelector)
{
    if (!keySelector || !parentKeySelector)
    {
        throw winrt::hresult_invalid_argument(L"ParentBy: keySelector and parentKeySelector are required.");
    }

    m_engine->SetParent(
        [keySelector](winrt::IInspectable const& item) { return keySelector(item); },
        [parentKeySelector](winrt::IInspectable const& item) { return parentKeySelector(item); });
    return *this;
}

winrt::ITableViewShapingSource TableViewSource1a::ClearParentBy()
{
    m_engine->ClearParentBy();
    return *this;
}

winrt::ITableViewShapingSource TableViewSource1a::ClearSort()
{
    ShapingTip tip{ *this, TableViewTip::Operation::Sort, m_engine->HasSortAxis({}) };
    tip.Apply([&]() { m_engine->ClearSorts(); });
    return *this;
}

winrt::TableViewSource1a TableViewSource1a::ClearSort(winrt::hstring const& sortAxisToken)
{
    ShapingTip tip{ *this, TableViewTip::Operation::Sort, m_engine->HasSortAxis(sortAxisToken) };
    tip.Apply([&]() { m_engine->ClearSort(sortAxisToken); });
    return *this;
}

winrt::TableViewSource1a TableViewSource1a::ClearSortsExcept(winrt::hstring const& sortAxisToken)
{
    ShapingTip tip{ *this, TableViewTip::Operation::Sort, m_engine->HasSortOtherThan(sortAxisToken) };
    tip.Apply([&]() { m_engine->ClearSortsExcept(sortAxisToken); });
    return *this;
}

bool TableViewSource1a::IsLiveShaping() const
{
    return m_engine->IsLiveShapingEnabled();
}

void TableViewSource1a::IsLiveShaping(bool value)
{
    m_engine->SetLiveShaping(value);
}

std::vector<TableViewSource1a::ActiveSortAxisInfo> TableViewSource1a::ActiveSortAxisInfos() const
{
    std::vector<ActiveSortAxisInfo> infos;
    for (auto const& axis : m_engine->ActiveSortAxisInfos())
    {
        infos.push_back({ axis.AxisToken, axis.SortMemberPath, axis.Direction });
    }
    return infos;
}

winrt::hstring TableViewSource1a::SortAxisTokenForPath(winrt::hstring const& sortMemberPath)
{
    // Distinct from the control's "column:<address>" tokens, so a path-declared axis and a
    // column-declared axis are never mistaken for one another even when they sort the same
    // property. Tokenizing at all is what makes re-sorting the same path REPLACE its axis: an
    // untokenized axis is matched by delegate identity, and every call would synthesize a fresh
    // delegate, so Sort("Name", Asc) followed by Sort("Name", Desc) would stack two axes.
    return L"path:" + sortMemberPath;
}

winrt::ITableViewShapingSource TableViewSource1a::Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction)
{
    return SortCore({}, {}, key, {}, direction);
}

winrt::ITableViewShapingSource TableViewSource1a::Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    if (sortMemberPath.empty())
    {
        throw winrt::hresult_invalid_argument(L"sortMemberPath cannot be empty.");
    }

    // One resolver per axis, captured by the selector and reused across every comparison: the
    // binding is built once per path, so steady-state cost is a DataContext write plus a property
    // read rather than a fresh binding per item.
    auto resolver = std::make_shared<SortMemberPathResolver>(sortMemberPath);
    winrt::TableViewKeySelector key{
        [resolver](winrt::IInspectable const& item) -> winrt::IInspectable
        {
            return resolver->Resolve(item);
        }
    };

    return SortCore({}, SortAxisTokenForPath(sortMemberPath), key, sortMemberPath, direction);
}

winrt::TableViewSource1a TableViewSource1a::SortReplacing(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken, winrt::TableViewKeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    return SortCore(previousSortAxisToken, sortAxisToken, key, sortMemberPath, direction);
}

winrt::TableViewSource1a TableViewSource1a::SortCore(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken, winrt::TableViewKeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    if (!key)
    {
        throw winrt::hresult_invalid_argument(L"key cannot be null.");
    }

    // A projected enum is just an int32 across the ABI, so an out-of-range value would otherwise
    // reach the comparator and be treated as Descending (anything that is not Ascending). None is
    // a defined value and removes this key's sort axis, per the IDL contract.
    if (direction != winrt::SortDirection::None &&
        direction != winrt::SortDirection::Ascending &&
        direction != winrt::SortDirection::Descending)
    {
        throw winrt::hresult_invalid_argument(L"direction must be a defined SortDirection value.");
    }

    // The engine stores a std::function, which cannot be compared, so the delegate itself is
    // handed over as the axis identity. The wrapping lambda holds a strong ref to that same
    // delegate, so the identity stays valid for as long as the axis lives.
    auto const keyIdentity = key.as<winrt::Windows::Foundation::IUnknown>();
    // Removing an axis that is not there is a no-op, measured no more than a clear with nothing to clear.
    bool const meaningful = direction != winrt::SortDirection::None ||
        m_engine->HasMatchingSort(sortAxisToken, keyIdentity) ||
        (!previousSortAxisToken.empty() && previousSortAxisToken != sortAxisToken && m_engine->HasSortAxis(previousSortAxisToken));
    ShapingTip tip{ *this, TableViewTip::Operation::Sort, meaningful };
    tip.Apply([&]()
    {
        m_engine->SetSort(
            previousSortAxisToken,
            sortAxisToken,
            [key](winrt::IInspectable const& item) { return key(item); },
            keyIdentity,
            sortMemberPath,
            direction);
    });
    return *this;
}

bool TableViewSource1a::IsGrouped() const
{
    return m_engine->IsProjectedAsGrouped();
}

bool TableViewSource1a::HasFilter() const
{
    return m_engine->HasFilter();
}

bool TableViewSource1a::HasGroupBy() const
{
    return m_engine->HasGroup();
}

bool TableViewSource1a::HasParentBy() const
{
    return m_engine->HasParentBy();
}

uint32_t TableViewSource1a::SourceRowCount() const
{
    return m_engine->SourceRowCount();
}

uint32_t TableViewSource1a::DataRowCount() const
{
    // Rows() is only a snapshot of a tree's visible rows; the published index holds every node the filter keeps.
    if (auto const adapter = m_engine->HierarchicalAdapter(); adapter && adapter->HasIndex())
    {
        return adapter->KeptNodeCount();
    }
    // Rows() is the shaped data without headers and ignores group expansion.
    auto const rows = m_engine->Rows();
    return rows ? rows.Size() : 0;
}

bool TableViewSource1a::ContainsShapedItem(winrt::IInspectable const& item) const
{
    if (!item)
    {
        return false;
    }

    // Decided by the published index, not Kind(): a rebuild publishes the index (or releases it)
    // before raising the Reset that closes the edit, while Kind() is only updated afterwards.
    if (auto const adapter = m_engine->HierarchicalAdapter(); adapter && adapter->HasIndex())
    {
        // Rows() is only a snapshot of the visible rows here; the index holds every filtered node.
        return adapter->IndexContainsItem(item);
    }

    // Rows() is the shaped data without headers and ignores group expansion.
    auto const rows = m_engine->Rows();
    if (!rows)
    {
        return false;
    }
    auto const identity = item.as<winrt::Windows::Foundation::IUnknown>();
    for (auto const& row : rows)
    {
        if (row && row.as<winrt::Windows::Foundation::IUnknown>() == identity)
        {
            return true;
        }
    }
    return false;
}

winrt::ItemsSourceView TableViewSource1a::GetItemsSourceView()
{
    return m_itemsSourceView.get();
}

TableViewRowMetadataProvider TableViewSource1a::GetRowMetadata()
{
    return m_rowMetadata;
}

void TableViewSource1a::OnProjectionRebuilt()
{
    // The control-level reading of the projection: what an ItemsRepeater consumes, and how a row
    // at an index is described. The engine reports which shape it produced; deciding what that
    // shape means for a TableView is this class's only remaining projection responsibility.
    switch (m_engine->Kind())
    {
    case ::pb1a::ShapedItemsSource::ProjectionKind::Grouped:
    {
        auto const adapter = m_engine->GroupedAdapter();
        if (!adapter)
        {
            // A projection kind without its adapter is never published coherently; keep the
            // previous projection until the engine publishes again.
            MUX_ASSERT_MSG(false, L"TableViewSource1a: Grouped projection published without a grouped adapter.");
            return;
        }
        // No wrap: the grouped view IS an ItemsSourceView, so ItemsRepeater consumes it directly.
        m_itemsSourceView.set(adapter->Entries());
        m_rowMetadata = tabularPrimitives::RowMetadataProvider1a::CreateForGroupedRows(adapter, MakeIdentitySelector());
        break;
    }
    case ::pb1a::ShapedItemsSource::ProjectionKind::Hierarchical:
    {
        auto const adapter = m_engine->HierarchicalAdapter();
        if (!adapter)
        {
            MUX_ASSERT_MSG(false, L"TableViewSource1a: Hierarchical projection published without a hierarchical adapter.");
            return;
        }
        // Same as grouped: consumed directly.
        m_itemsSourceView.set(adapter->Entries());
        m_rowMetadata = tabularPrimitives::RowMetadataProvider1a::CreateForHierarchicalRows(adapter, MakeIdentitySelector());
        break;
    }
    case ::pb1a::ShapedItemsSource::ProjectionKind::GroupedHierarchical:
    {
        // Grouped adapter supplies the presented axis (header rows); hierarchy adapter supplies
        // level and expansion.
        auto const groupedAdapter = m_engine->GroupedAdapter();
        auto const hierarchicalAdapter = m_engine->HierarchicalAdapter();
        if (!groupedAdapter || !hierarchicalAdapter)
        {
            MUX_ASSERT_MSG(false, L"TableViewSource1a: GroupedHierarchical projection published without both adapters.");
            return;
        }
        m_itemsSourceView.set(groupedAdapter->Entries());
        m_rowMetadata = tabularPrimitives::RowMetadataProvider1a::CreateForGroupedHierarchicalRows(
            groupedAdapter,
            hierarchicalAdapter,
            MakeIdentitySelector());
        break;
    }
    case ::pb1a::ShapedItemsSource::ProjectionKind::Flat:
    {
        // tracker_ref, so set through .set(); read the local back into the metadata provider since
        // CreateForFlatRows takes the concrete ItemsSourceView, not the tracked slot.
        auto const flatView = winrt::ItemsSourceView{ m_engine->Rows() };
        m_itemsSourceView.set(flatView);
        m_rowMetadata = tabularPrimitives::RowMetadataProvider1a::CreateForFlatRows(flatView, MakeIdentitySelector());
        break;
    }
    case ::pb1a::ShapedItemsSource::ProjectionKind::Unshaped:
        // An unshaped mirror carries no shaped identity, so there is deliberately no row
        // metadata: consumers must not read sorted/identity semantics off a degraded projection.
        m_itemsSourceView.set(winrt::ItemsSourceView{ m_engine->Rows() });
        m_rowMetadata = nullptr;
        break;
    case ::pb1a::ShapedItemsSource::ProjectionKind::None:
        m_itemsSourceView.set(nullptr);
        m_rowMetadata = nullptr;
        break;
    }

    // Every rebuild mints a fresh row-metadata provider and can swap the view, so anything the
    // owner cached from the previous projection is stale from here - whether or not grouped-ness
    // changed.
    if (m_shapingTip) { m_shapingTip->observation.published = true; }
    NotifyOwnerProjectionChanged();
}

TableViewRowItemKeySelector TableViewSource1a::MakeIdentitySelector() const
{
    // Bridge the engine's Object-returning identity selector to the String-returning selector the
    // row-metadata provider consumes. The engine derives identity from each item's object identity
    // and already stringifies it, so this only unwraps the box.
    auto const keySelector = m_engine->IdentitySelector();
    if (!keySelector)
    {
        return {};
    }

    return [keySelector](winrt::IInspectable const& item) -> winrt::hstring
    {
        winrt::IInspectable key{ nullptr };
        try
        {
            key = keySelector(item);
        }
        catch (...)
        {
            return L"";
        }
        if (auto propertyValue = key.try_as<winrt::IPropertyValue>();
            propertyValue && propertyValue.Type() == winrt::PropertyType::String)
        {
            return propertyValue.GetString();
        }
        return {};
    };
}

bool TableViewSource1a::SetOwningTableView(winrt::IInspectable const& owner)
{
    // Sharing one shaped TableViewSource1a across two TableView controls has no coherent
    // semantics: shaping verbs (Filter/Sort/GroupBy) mutate a single projection, and the two
    // controls would compete for it (last-writer-wins) while both cache the ItemsSourceView /
    // row-metadata / grouped-ness the first bind produced. Since only a single owner slot
    // exists, silently overwriting m_owningTableView here left the previous owner rendering
    // against stale shape metadata and never receiving projection-shape notifications.
    //
    // Contract: a second TableView cannot bind a source another TableView already owns; it is
    // refused and renders empty until the owner releases it (assigns a different ItemsSource or
    // is unloaded). Not an assert: a navigation loads the new page before it unloads the old one,
    // so a refusal is a normal transient state.
    if (auto existingOwner = m_owningTableView.get())
    {
        if (existingOwner != owner)
        {
            return false;
        }
        // Same TableView re-registering itself (e.g. reclaiming on Loaded). Idempotent.
    }

    if (m_owningTableView.get() != owner) { ++m_ownerGeneration; }
    m_owningTableView = winrt::weak_ref<winrt::IInspectable>{ owner };
    RemoveOwnerReleasedHandler(owner);
    return true;
}

void TableViewSource1a::AddOwnerReleasedHandler(winrt::IInspectable const& waiter, std::function<void()> handler)
{
    RemoveOwnerReleasedHandler(waiter);
    m_ownerReleasedWaiters.push_back({ winrt::weak_ref<winrt::IInspectable>{ waiter }, std::move(handler) });
}

void TableViewSource1a::RemoveOwnerReleasedHandler(winrt::IInspectable const& waiter)
{
    // Also prunes waiters that have been collected.
    std::erase_if(m_ownerReleasedWaiters, [&waiter](OwnerReleasedWaiter const& entry)
    {
        auto const entryWaiter = entry.Waiter.get();
        return !entryWaiter || entryWaiter == waiter;
    });
}

void TableViewSource1a::ReleaseOwningTableView(winrt::IInspectable const& owner)
{
    // Only the owner can release; a table whose bind was refused must not detach the real owner.
    if (auto existingOwner = m_owningTableView.get(); existingOwner && existingOwner != owner)
    {
        return;
    }

    if (m_owningTableView.get()) { ++m_ownerGeneration; }
    m_owningTableView = winrt::weak_ref<winrt::IInspectable>{ nullptr };

    // Drop the owner's handlers with the owner itself, so a source that has been swapped out of
    // ItemsSource cannot drive the control it used to belong to.
    m_projectionChanged = nullptr;
    m_shapingChanged = nullptr;
    m_shapingTipHandlers.reset();
    // A restore held for the former owner's editor must not wait on an edit it will no longer
    // hear the end of.
    m_engine->SetLiveShapingHold(nullptr);
    m_engine->SetLiveItemChangedHook(nullptr);
    m_engine->SetRowsReleasedHook(nullptr);
    m_engine->SetLiveItemReshapeHandler(nullptr);
    m_engine->ResumeHeldLiveShaping();

    // Last, after the former owner's handlers are dropped. Each waiter is popped before it runs:
    // it re-enters SetOwningTableView, and one that does not claim must not be retried.
    while (!m_owningTableView.get() && !m_ownerReleasedWaiters.empty())
    {
        auto waiter = std::move(m_ownerReleasedWaiters.front());
        m_ownerReleasedWaiters.erase(m_ownerReleasedWaiters.begin());
        if (waiter.Waiter.get() && waiter.Handler)
        {
            waiter.Handler();
        }
    }
}

void TableViewSource1a::SetLiveShapingHold(std::function<bool()> isHeld)
{
    m_engine->SetLiveShapingHold(std::move(isHeld));
}

void TableViewSource1a::SetLiveItemChangedHook(std::function<bool(winrt::IInspectable const&, bool)> hook)
{
    m_engine->SetLiveItemChangedHook(std::move(hook));
}

void TableViewSource1a::SetLiveItemReshapeHandler(std::function<void()> handler)
{
    m_engine->SetLiveItemReshapeHandler(std::move(handler));
}

void TableViewSource1a::SetRowsReleasedHook(std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> hook)
{
    m_engine->SetRowsReleasedHook(std::move(hook));
}

void TableViewSource1a::ResumeHeldLiveShaping()
{
    m_engine->ResumeHeldLiveShaping();
}

bool TableViewSource1a::CanApplySort() const
{
    return m_engine->CanProjectWithRowIdentity();
}

void TableViewSource1a::NotifyOwnerProjectionChanged()
{
    if (m_projectionChanged)
    {
        // The owner can run app code that releases the last reference to this source.
        auto const strongThis = get_strong();
        ++m_notificationDepth;
        auto restore = gsl::finally([this]() noexcept { --m_notificationDepth; });
        m_projectionChanged();
    }
}

void TableViewSource1a::NotifyOwnerShapingChanged(bool reorderOnly)
{
    // Only the in-place reorder notification proves computation; Refresh can merely defer.
    if (reorderOnly && m_shapingTip) { m_shapingTip->observation.published = true; }
    if (m_shapingChanged)
    {
        auto const strongThis = get_strong();
        ++m_notificationDepth;
        auto restore = gsl::finally([this]() noexcept { --m_notificationDepth; });
        m_shapingChanged(reorderOnly);
    }
}
