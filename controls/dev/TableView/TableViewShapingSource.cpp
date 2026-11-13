// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewShapingSource.h"
#include "TableViewTip.h"
#include "RowMetadataProvider.h"
#include "GroupedSourceAdapter.h"
#include "HierarchicalSourceAdapter.h"
#include "ShapedItemsSource.h"
#include "ShapingHelpers.h"
#include "SortMemberPathResolver.h"
#include "SharedHelpers.h"
#include "TVDiag.h"

namespace
{
    namespace tabularPrimitives = winrt::Microsoft::UI::Xaml::Controls::Tabular::Primitives::implementation;

    void ThrowIfInvalidDirection(winrt::SortDirection direction, bool allowNone)
    {
        // A projected enum is just an int32 across the ABI, so an out-of-range value would otherwise
        // reach the comparator and be treated as Descending (anything that is not Ascending).
        if (direction == winrt::SortDirection::None)
        {
            if (!allowNone)
            {
                throw winrt::hresult_invalid_argument(L"direction cannot be None here; use Sort or ClearSort to remove sort levels.");
            }
            return;
        }
        if (direction != winrt::SortDirection::Ascending && direction != winrt::SortDirection::Descending)
        {
            throw winrt::hresult_invalid_argument(L"direction must be a defined SortDirection value.");
        }
    }

    // Ends one deferral of source on a later turn of queue. An end refused as re-entrant leaves the deferral open
    // and is posted again, a bounded number of times; a failure of the reshape itself has already ended it, so it is
    // not retried.
    constexpr uint32_t c_maxEndDeferRefreshRetries = 100;

    void PostEndDeferRefresh(winrt::Microsoft::UI::Dispatching::DispatcherQueue const& queue, winrt::com_ptr<::TableViewShapingSource> const& source, uint32_t attempt = 0) noexcept
    {
        try
        {
            queue.TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Low, [queue, source, attempt]()
            {
                try
                {
                    source->ThrowIfEndDeferRefreshReentrant();
                }
                catch (...)
                {
                    if (attempt + 1 < c_maxEndDeferRefreshRetries)
                    {
                        PostEndDeferRefresh(queue, source, attempt + 1);
                    }
                    else
                    {
                        TVDiag::LogRetailF(L"[TableView] A refresh deferral could not be ended: every attempt ran inside an app delegate.");
                    }
                    return;
                }
                try { source->EndDeferRefresh(); } catch (...) {}
            });
        }
        catch (...) {}
    }
}

struct TableViewShapingSource::ShapingTip
{
    ShapingTip(TableViewShapingSource& source, TableViewTip::Operation kind, bool meaningful = true) :
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

    TableViewShapingSource& source;
    TableViewTip::Operation kind;
    std::shared_ptr<ShapingTipHandlers> handlers;
    ShapingTip* previous{};
    uint64_t ownerGeneration{};
    uint64_t generation{};
    TableViewTip::ShapingObservation observation;
};

void TableViewShapingSource::SetShapingTipHandlers(
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

TableViewShapingSource::~TableViewShapingSource()
{
    // An engine pass still on the stack keeps the engine alive; it must stop and not call back here.
    *m_obsolete = true;
    m_engine->SetProjectionRebuiltHandler(nullptr);
    m_engine->SetShapingChangedHandler(nullptr);
}

TableViewShapingSource::TableViewShapingSource(winrt::IInspectable const& items) :
    m_engine(std::make_shared<::ShapedItemsSource>(items))
{
    m_engine->DiagnosticName(L"TableView");
    // Handlers are installed before Start() so the very first projection is observed like any
    // later one -- the control never has to special-case its own construction.
    m_engine->SetProjectionRebuiltHandler([this]() { OnProjectionRebuilt(); });
    m_engine->SetShapingChangedHandler([this](bool reorderOnly) { NotifyOwnerShapingChanged(reorderOnly); });
    m_engine->SetObsoleteFlag(m_obsolete);
    m_engine->Start();
}

winrt::com_ptr<TableViewShapingSource> TableViewShapingSource::Create(winrt::IInspectable const& items)
{
    if (!items)
    {
        throw winrt::hresult_invalid_argument(L"items cannot be null.");
    }

    return winrt::make_self<::TableViewShapingSource>(items);
}

winrt::IInspectable TableViewShapingSource::Source() const
{
    return m_engine->Source();
}

void TableViewShapingSource::GroupBy(ShapingHelpers::KeySelector const& key, RowIdentity::IdentitySelector const& groupIdentitySelector)
{
    if (!key)
    {
        throw winrt::hresult_invalid_argument(L"key cannot be null.");
    }

    m_engine->ThrowIfReentrant();
    ShapingTip tip{ *this, TableViewTip::Operation::GroupConfiguration };
    tip.Apply([&]()
    {
        m_engine->SetGroup(key, groupIdentitySelector);
    });
}

void TableViewShapingSource::ParentBy(ShapingHelpers::KeySelector const& keySelector, ShapingHelpers::KeySelector const& parentKeySelector)
{
    if (!keySelector || !parentKeySelector)
    {
        throw winrt::hresult_invalid_argument(L"ParentBy: keySelector and parentKeySelector are required.");
    }

    m_engine->ThrowIfReentrant();
    m_engine->SetParent(keySelector, parentKeySelector);
}

void TableViewShapingSource::ClearParentBy()
{
    m_engine->ThrowIfReentrant();
    m_engine->ClearParentBy();
}

void TableViewShapingSource::ClearSort()
{
    m_engine->ThrowIfReentrant();
    ShapingTip tip{ *this, TableViewTip::Operation::Sort, m_engine->HasSortAxis({}) };
    tip.Apply([&]() { m_engine->ClearSorts(); });
}

void TableViewShapingSource::ClearSort(winrt::hstring const& sortAxisToken)
{
    m_engine->ThrowIfReentrant();
    ShapingTip tip{ *this, TableViewTip::Operation::Sort, m_engine->HasSortAxis(sortAxisToken) };
    tip.Apply([&]() { m_engine->ClearSort(sortAxisToken); });
}

void TableViewShapingSource::ThrowIfWrongThread() const
{
    if (!m_engine->HasOwnerThreadAccess())
    {
        throw winrt::hresult_wrong_thread();
    }
}

void TableViewShapingSource::Refresh()
{
    ThrowIfWrongThread();
    m_engine->RefreshShaping();
}

void TableViewShapingSource::BeginDeferRefresh()
{
    m_engine->BeginDeferRefresh();
}

void TableViewShapingSource::EndDeferRefresh()
{
    m_engine->EndDeferRefresh();
}

void TableViewShapingSource::EndDeferRefreshLater()
{
    auto queue = m_engine->OwnerQueue();
    if (!queue)
    {
        if (!m_deferralQueue)
        {
            m_deferralQueue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
        }
        queue = m_deferralQueue;
    }
    if (!queue)
    {
        // Nothing to post to: best effort now.
        m_engine->EndDeferRefresh();
        return;
    }

    PostEndDeferRefresh(queue, get_strong());
}

std::optional<winrt::hstring> TableViewShapingSource::TryGetGroupIdentityForKey(winrt::IInspectable const& key) const
{
    winrt::hstring identity;
    if (m_engine->TryGetGroupIdentityForKey(key, identity))
    {
        return identity;
    }
    return std::nullopt;
}

std::vector<TableViewShapingSource::GroupInfo> TableViewShapingSource::CurrentGroups() const
{
    std::vector<GroupInfo> groups;
    for (auto const& group : m_engine->CurrentGroups())
    {
        groups.push_back({ group.Identity, group.Key, group.Group });
    }
    return groups;
}

void TableViewShapingSource::ThrowIfEndDeferRefreshReentrant() const
{
    m_engine->ThrowIfEndDeferRefreshReentrant();
}

bool TableViewShapingSource::HasItemSort() const
{
    return m_engine->HasSortAxis({});
}

bool TableViewShapingSource::IsLiveShaping() const
{
    return m_engine->IsLiveShapingEnabled();
}

std::vector<TableViewShapingSource::ActiveSortAxisInfo> TableViewShapingSource::ActiveSortAxisInfos() const
{
    std::vector<ActiveSortAxisInfo> infos;
    for (auto const& axis : m_engine->ActiveSortAxisInfos())
    {
        infos.push_back({ axis.AxisToken, axis.SortMemberPath, axis.Direction });
    }
    return infos;
}

winrt::hstring TableViewShapingSource::SortAxisTokenForPath(winrt::hstring const& sortMemberPath)
{
    // Distinct from the control's "column:<address>" tokens, so a path-declared axis and a
    // column-declared axis are never mistaken for one another even when they sort the same
    // property.
    return L"path:" + sortMemberPath;
}

ShapingHelpers::KeySelector TableViewShapingSource::MakePathKeySelector(winrt::hstring const& path)
{
    if (path.empty())
    {
        throw winrt::hresult_invalid_argument(L"path cannot be empty.");
    }

    // One resolver per axis, captured by the selector and reused across every comparison: the
    // binding is built once per path, so steady-state cost is a DataContext write plus a property
    // read rather than a fresh binding per item.
    auto resolver = std::make_shared<SortMemberPathResolver>(path);
    return [resolver](winrt::IInspectable const& item) -> winrt::IInspectable
    {
        return resolver->Resolve(item);
    };
}

void TableViewShapingSource::SortExclusive(winrt::hstring const& sortAxisToken, ShapingHelpers::KeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    if (direction == winrt::SortDirection::None)
    {
        ClearSort();
        return;
    }
    if (!key)
    {
        throw winrt::hresult_invalid_argument(L"key cannot be null.");
    }
    ReplaceSortsCore(sortAxisToken, key, sortMemberPath, direction);
}

void TableViewShapingSource::ReplaceSortsCore(winrt::hstring const& sortAxisToken, ShapingHelpers::KeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    ThrowIfInvalidDirection(direction, true /* allowNone */);
    m_engine->ThrowIfReentrant();
    // Clearing nothing is a no-op, measured no more than a clear with nothing to clear.
    ShapingTip tip{ *this, TableViewTip::Operation::Sort, direction != winrt::SortDirection::None || m_engine->HasSortAxis({}) };
    tip.Apply([&]()
    {
        m_engine->ReplaceSorts(
            sortAxisToken,
            key,
            sortMemberPath,
            direction);
    });
}

bool TableViewShapingSource::IsGrouped() const
{
    return m_engine->IsProjectedAsGrouped();
}

bool TableViewShapingSource::HasFilter() const
{
    return m_engine->HasFilter();
}

bool TableViewShapingSource::HasGroupBy() const
{
    return m_engine->HasGroup();
}

bool TableViewShapingSource::HasParentBy() const
{
    return m_engine->HasParentBy();
}

uint32_t TableViewShapingSource::SourceRowCount() const
{
    return m_engine->SourceRowCount();
}

uint32_t TableViewShapingSource::DataRowCount() const
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

winrt::ItemsSourceView TableViewShapingSource::GetItemsSourceView()
{
    return m_itemsSourceView.get();
}

TableViewRowMetadataProvider TableViewShapingSource::GetRowMetadata()
{
    return m_rowMetadata;
}

void TableViewShapingSource::OnProjectionRebuilt()
{
    // The control-level reading of the projection: what an ItemsRepeater consumes, and how a row
    // at an index is described. The engine reports which shape it produced; deciding what that
    // shape means for a TableView is this class's only remaining projection responsibility.
    switch (m_engine->Kind())
    {
    case ::ShapedItemsSource::ProjectionKind::Grouped:
    {
        auto const adapter = m_engine->GroupedAdapter();
        if (!adapter)
        {
            // A projection kind without its adapter is never published coherently; keep the
            // previous projection until the engine publishes again.
            MUX_ASSERT_MSG(false, L"TableViewShapingSource: Grouped projection published without a grouped adapter.");
            return;
        }
        // No wrap: the grouped view IS an ItemsSourceView, so ItemsRepeater consumes it directly.
        m_itemsSourceView.set(adapter->Entries());
        m_rowMetadata = tabularPrimitives::RowMetadataProvider::CreateForGroupedRows(adapter, MakeIdentitySelector());
        break;
    }
    case ::ShapedItemsSource::ProjectionKind::Hierarchical:
    {
        auto const adapter = m_engine->HierarchicalAdapter();
        if (!adapter)
        {
            MUX_ASSERT_MSG(false, L"TableViewShapingSource: Hierarchical projection published without a hierarchical adapter.");
            return;
        }
        // Same as grouped: consumed directly.
        m_itemsSourceView.set(adapter->Entries());
        m_rowMetadata = tabularPrimitives::RowMetadataProvider::CreateForHierarchicalRows(adapter, MakeIdentitySelector());
        break;
    }
    case ::ShapedItemsSource::ProjectionKind::GroupedHierarchical:
    {
        // Grouped adapter supplies the presented axis (header rows); hierarchy adapter supplies
        // level and expansion.
        auto const groupedAdapter = m_engine->GroupedAdapter();
        auto const hierarchicalAdapter = m_engine->HierarchicalAdapter();
        if (!groupedAdapter || !hierarchicalAdapter)
        {
            MUX_ASSERT_MSG(false, L"TableViewShapingSource: GroupedHierarchical projection published without both adapters.");
            return;
        }
        m_itemsSourceView.set(groupedAdapter->Entries());
        m_rowMetadata = tabularPrimitives::RowMetadataProvider::CreateForGroupedHierarchicalRows(
            groupedAdapter,
            hierarchicalAdapter,
            MakeIdentitySelector());
        break;
    }
    case ::ShapedItemsSource::ProjectionKind::Flat:
    {
        // tracker_ref, so set through .set(); read the local back into the metadata provider since
        // CreateForFlatRows takes the concrete ItemsSourceView, not the tracked slot.
        auto const flatView = winrt::ItemsSourceView{ m_engine->Rows() };
        m_itemsSourceView.set(flatView);
        m_rowMetadata = tabularPrimitives::RowMetadataProvider::CreateForFlatRows(flatView, MakeIdentitySelector());
        break;
    }
    case ::ShapedItemsSource::ProjectionKind::Unshaped:
        // An unshaped mirror carries no shaped identity, so there is deliberately no row
        // metadata: consumers must not read sorted/identity semantics off a degraded projection.
        m_itemsSourceView.set(winrt::ItemsSourceView{ m_engine->Rows() });
        m_rowMetadata = nullptr;
        break;
    case ::ShapedItemsSource::ProjectionKind::None:
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

TableViewRowItemKeySelector TableViewShapingSource::MakeIdentitySelector() const
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

bool TableViewShapingSource::SetOwningTableView(winrt::IInspectable const& owner)
{
    // Sharing one shaped TableViewShapingSource across two TableView controls has no coherent
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
    return true;
}

void TableViewShapingSource::ReleaseOwningTableView(winrt::IInspectable const& owner)
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
    m_engine->SetLiveItemChangedHook(nullptr);
    m_engine->SetRowsReleasedHook(nullptr);
    m_engine->SetLiveItemReshapeHandler(nullptr);

}

void TableViewShapingSource::SetLiveItemChangedHook(std::function<bool(winrt::IInspectable const&, bool)> hook)
{
    m_engine->SetLiveItemChangedHook(std::move(hook));
}

void TableViewShapingSource::SetLiveItemReshapeHandler(std::function<void()> handler)
{
    m_engine->SetLiveItemReshapeHandler(std::move(handler));
}

void TableViewShapingSource::SetRowsReleasedHook(std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> hook)
{
    m_engine->SetRowsReleasedHook(std::move(hook));
}

bool TableViewShapingSource::CanApplySort() const
{
    return m_engine->CanProjectWithRowIdentity();
}

void TableViewShapingSource::NotifyOwnerProjectionChanged()
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

void TableViewShapingSource::NotifyOwnerShapingChanged(bool reorderOnly)
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
