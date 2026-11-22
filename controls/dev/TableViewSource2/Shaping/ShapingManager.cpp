// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Shaping/ShapingManager.h"

#include <algorithm>
#include <exception>
#include <utility>

// RAII bracket for one commit (M17). EndCommit runs from the destructor on both normal exit and
// unwind; std::uncaught_exceptions tells the two apart.
class ShapingManager::CommitScope
{
public:
    CommitScope(ShapingManager& manager, std::span<SourceRecord const* const> retired)
        : m_manager(manager)
        , m_retired(retired)
        , m_uncaught(std::uncaught_exceptions())
    {
        m_manager.BeginCommit();
    }

    ~CommitScope()
    {
        m_manager.EndCommit(m_retired, std::uncaught_exceptions() > m_uncaught);
    }

    CommitScope(CommitScope const&) = delete;
    CommitScope& operator=(CommitScope const&) = delete;

private:
    ShapingManager& m_manager;
    std::span<SourceRecord const* const> m_retired;
    int m_uncaught{ 0 };
};

ShapingManager::ShapingManager(ITrackerHandleManager const* trackerOwner, winrt::IInspectable const& items)
    : m_trackerOwner(trackerOwner)
    , m_sourceTable(SourceTable::Create(trackerOwner, items))
    , m_delegates(trackerOwner)
    , m_engine(trackerOwner, *m_sourceTable, m_delegates, m_paths)
    , m_rows(m_engine, *m_sourceTable)
{
    try
    {
        m_sourceTable->SetSourceChangedHandler([this](SourceChange const& change)
        {
            OnSourceChanged(change);
        });
        m_sourceTable->SetItemChangedHandler([this](SourceRecord* record, winrt::hstring const& propertyName)
        {
            OnItemChanged(record, propertyName);
        });

        // The initial load's Reset was raised inside Create, before this handler existed. Place
        // the records now; there is no listener or owner yet, so nothing is raised.
        m_engine.Reapply(m_desired, false /* clearOverlay */);
        m_engine.ReleaseRetired();

        // From here on every commit reaches the listener, which forwards to the owner and the
        // view as each appears (both may be null). An owner may claim before View() is called.
        m_engine.Tree().SetListener(static_cast<IShapedRowsListener*>(this));
        m_liveDrainToken = std::make_shared<ShapingManager*>(this);
    }
    catch (...)
    {
        // The destructor does not run for a throwing constructor; detach while the owner is alive.
        Clear();
        throw;
    }
}

ShapingManager::~ShapingManager()
{
    MUX_ASSERT(m_sourceTable->IsDetached());
    MUX_ASSERT(!m_view && !m_viewImpl && !m_rowMetadata);
}

// ---- Spec verbs ---------------------------------------------------------------------------------

namespace
{
    // Sort(level, None) removes the matching level; otherwise it replaces it in place (keeping its
    // priority) or appends a new lowest-priority level.
    void UpsertSortLevel(std::vector<SortSpec>& sorts, SortSpec level)
    {
        auto const match = std::find_if(sorts.begin(), sorts.end(),
            [&](SortSpec const& existing) { return existing.Matches(level); });
        if (level.SortKind != SortSpec::Kind::Comparer && level.Direction == winrt::SortDirection::None)
        {
            if (match != sorts.end())
            {
                sorts.erase(match);
            }
        }
        else if (match != sorts.end())
        {
            *match = std::move(level);
        }
        else
        {
            sorts.push_back(std::move(level));
        }
    }
}

void ShapingManager::Filter(winrt::TableViewPredicate const& predicate)
{
    if (!BeginVerb(L"Filter"))
    {
        return;
    }

    m_desired.Filters = { FilterSpec{ m_delegates.Register(predicate) } };
    if (CanReshape())
    {
        ApplyDesiredSpec();
    }
}

void ShapingManager::GroupBy(winrt::TableViewKeySelector const& key, winrt::SortDirection groupDirection)
{
    if (!BeginVerb(L"GroupBy"))
    {
        return;
    }

    m_desired.Groups = { GroupSpec{ m_delegates.Register(key), groupDirection } };
    if (CanReshape())
    {
        ApplyDesiredSpec();
    }
}

void ShapingManager::Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    if (!BeginVerb(L"Sort"))
    {
        return;
    }

    SortSpec level;
    level.SortKind = SortSpec::Kind::Path;
    level.Path = sortMemberPath;
    level.Direction = direction;
    UpsertSortLevel(m_desired.Sorts, std::move(level));
    if (CanReshape())
    {
        ApplyDesiredSpec();
    }
}

void ShapingManager::Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction)
{
    if (!BeginVerb(L"Sort"))
    {
        return;
    }

    SortSpec level;
    level.SortKind = SortSpec::Kind::Key;
    level.Delegate = m_delegates.Register(key);
    level.Direction = direction;
    UpsertSortLevel(m_desired.Sorts, std::move(level));
    if (CanReshape())
    {
        ApplyDesiredSpec();
    }
}

void ShapingManager::Sort(winrt::TableViewComparer2 const& comparer)
{
    if (!BeginVerb(L"Sort"))
    {
        return;
    }

    SortSpec level;
    level.SortKind = SortSpec::Kind::Comparer;
    level.Delegate = m_delegates.Register(comparer);
    UpsertSortLevel(m_desired.Sorts, std::move(level));
    if (CanReshape())
    {
        ApplyDesiredSpec();
    }
}

void ShapingManager::ClearFilter()
{
    if (!BeginVerb(L"ClearFilter"))
    {
        return;
    }

    m_desired.Filters.clear();
    if (CanReshape())
    {
        ApplyDesiredSpec();
    }
}

void ShapingManager::ClearGroupBy()
{
    if (!BeginVerb(L"ClearGroupBy"))
    {
        return;
    }

    m_desired.Groups.clear();
    if (CanReshape())
    {
        ApplyDesiredSpec();
    }
}

void ShapingManager::ClearSort()
{
    if (!BeginVerb(L"ClearSort"))
    {
        return;
    }

    m_desired.Sorts.clear();
    if (CanReshape())
    {
        ApplyDesiredSpec();
    }
}

// ---- Hierarchy verbs ----------------------------------------------------------------------------

void ShapingManager::ParentBy(winrt::TableViewKeySelector const& key, winrt::TableViewKeySelector const& parentKey)
{
    if (!BeginVerb(L"ParentBy"))
    {
        return;
    }

    // The verb's Reset comes back through OnSourceChanged, which opens the commit scope.
    m_sourceTable->SetParentKeyHierarchy(key, parentKey);
}

void ShapingManager::ChildrenBy(winrt::TableViewChildrenSelector2 const& childrenSelector)
{
    if (!BeginVerb(L"ChildrenBy"))
    {
        return;
    }

    m_sourceTable->SetChildCollectionHierarchy(childrenSelector);
}

void ShapingManager::ClearHierarchy()
{
    if (!BeginVerb(L"ClearHierarchy"))
    {
        return;
    }

    m_sourceTable->ClearHierarchy();
}

// ---- Source -------------------------------------------------------------------------------------

void ShapingManager::Refresh()
{
    if (!BeginVerb(L"Refresh"))
    {
        return;
    }

    if (!CanReshape())
    {
        // Owed at the outermost EndDeferRefresh; the table is not marked diverged, so incremental
        // changes keep showing until then.
        m_sourceTable->ScheduleRefresh();
        return;
    }

    // The re-read's Reset comes back through OnSourceChanged, which opens the commit scope.
    m_sourceTable->Refresh();
}

// ---- DeferRefresh -------------------------------------------------------------------------------

void ShapingManager::BeginDeferRefresh()
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2.DeferRefresh cannot be called while the source is being shaped.");
    }

    if (m_sourceTable->IsDetached())
    {
        return;
    }

    if (m_deferDepth++ == 0)
    {
        m_sourceTable->BeginHoldResets();
    }
}

void ShapingManager::EndDeferRefresh()
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"A DeferRefresh handle cannot be closed while the source is being shaped.");
    }

    MUX_ASSERT(m_deferDepth > 0 || m_sourceTable->IsDetached());
    if (m_deferDepth == 0 || --m_deferDepth > 0)
    {
        return;
    }

    bool const rereadOwed = m_sourceTable->EndHoldResets();
    if (m_sourceTable->IsDetached())
    {
        return;
    }

    try
    {
        if (rereadOwed)
        {
            // One re-read (it also builds a parked hierarchy); its Reset reapplies m_desired. All
            // or nothing (M11): a throw propagates and m_desired waits for the next verb or Refresh.
            m_sourceTable->Refresh();
        }

        // Nothing when the spec did not change (or the re-read applied it) and the engine is not
        // stale; releases dropped delegates either way.
        ApplyDesiredSpec();
    }
    catch (...)
    {
        PostLiveDrain();
        throw;
    }

    PostLiveDrain();
}

bool ShapingManager::IsRefreshDeferred() const noexcept
{
    return m_deferDepth > 0;
}

// ---- Live shaping setting and applied spec ------------------------------------------------------

bool ShapingManager::IsLiveShaping() const
{
    return m_sourceTable->IsLiveShaping();
}

void ShapingManager::IsLiveShaping(bool value)
{
    if (!BeginVerb(L"IsLiveShaping"))
    {
        return;
    }

    // Gates item changes from now on (M10); changes already queued stay for the posted drain.
    m_sourceTable->SetLiveShaping(value);
}

ShapeSpec const& ShapingManager::AppliedSpec() const noexcept
{
    return m_engine.Applied();
}

// ---- Owner sink ---------------------------------------------------------------------------------

void ShapingManager::SetOwner(IShapingOwner* sink)
{
    // A release always succeeds (M1): the listener re-reads m_owner at each callback, so a release
    // from inside a commit only silences the rest of that commit.
    if (sink && !CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2 cannot change its owner while the source is being shaped.");
    }

    if (sink == m_owner)
    {
        return;
    }

    // Holds belong to the owner that took them; changes they deferred drain on the next turn.
    m_liveHoldCount = 0;
    ++m_ownerGeneration;
    m_owner = sink;

    if (sink)
    {
        m_sourceTable->BindOwnerThread();
    }

    PostLiveDrain();
}

void ShapingManager::SetCommitEndedHandler(std::function<void()> handler) noexcept
{
    m_commitEnded = std::move(handler);
}

bool ShapingManager::IsApplying() const noexcept
{
    return m_isApplying;
}

uint64_t ShapingManager::BeginLiveShapingHold() noexcept
{
    ++m_liveHoldCount;
    return m_ownerGeneration;
}

void ShapingManager::EndLiveShapingHold(uint64_t generation) noexcept
{
    if (generation != m_ownerGeneration || m_liveHoldCount == 0)
    {
        // Taken before SetOwner or Clear dropped the holds: it is not one of the current count.
        return;
    }

    if (--m_liveHoldCount == 0)
    {
        PostLiveDrain();
    }
}

void ShapingManager::SetWeakOwnerResolver(std::function<winrt::weak_ref<winrt::IInspectable>()> resolver) noexcept
{
    m_weakOwnerResolver = std::move(resolver);
}

// ---- Output and queries -------------------------------------------------------------------------

winrt::ItemsSourceView ShapingManager::View()
{
    if (m_sourceTable->IsDetached())
    {
        return nullptr;
    }

    if (!m_view)
    {
        auto const view = winrt::make_self<ShapedCollectionView>(m_rows);
        // ShapedCollectionView implements ItemsSourceView's default interface (M16), so the
        // projected class is a QueryInterface for it.
        m_view.emplace(m_trackerOwner, view.as<winrt::ItemsSourceView>());
        m_viewImpl = view.get();
    }

    return m_view->get();
}

bool ShapingManager::IsItemInShapedTree(winrt::IInspectable const& item) const
{
    return m_rows.IsItemInShapedTree(item);
}

std::shared_ptr<ITableViewRowMetadataProvider> ShapingManager::RowMetadata()
{
    if (m_sourceTable->IsDetached())
    {
        return nullptr;
    }

    if (!m_rowMetadata)
    {
        // Materialize the view first so TryGetGroupMembership can compare its rows argument with
        // this source's view by identity.
        View();
        m_rowMetadata = std::make_shared<ShapedRowMetadata>(
            m_rows, static_cast<IShapedExpansion&>(*this), *m_viewImpl);
    }

    return m_rowMetadata;
}

// ---- Lifetime -----------------------------------------------------------------------------------

void ShapingManager::Clear()
{
    // No callbacks during teardown; nothing below calls get() on a tracker_ref.
    m_owner = nullptr;
    m_commitEnded = nullptr;
    m_weakOwnerResolver = nullptr;
    m_liveHoldCount = 0;
    ++m_ownerGeneration;
    m_liveDrainToken.reset();
    ClearLiveChanges();
    m_liveDrainPosted = false;
    m_deferDepth = 0;

    if (m_viewImpl)
    {
        m_viewImpl->Detach();
    }

    if (m_rowMetadata)
    {
        m_rowMetadata->Detach();
    }

    m_engine.Clear();
    m_sourceTable->Clear();
    m_delegates.Clear();
    m_paths.Clear();

    m_viewImpl = nullptr;
    m_view.reset();
    m_rowMetadata.reset();
}

// ---- SourceTable callbacks ----------------------------------------------------------------------

void ShapingManager::OnSourceChanged(SourceChange const& change)
{
    // A delegate throw during the re-shape: the engine has already dropped the departed records
    // (UnlinkOnly) and raised a Reset, so the commit ends normally and the owner hears it; the
    // error is rethrown after, into the app's mutation.
    std::exception_ptr fault;
    {
        CommitScope scope{ *this, change.Removed };
        try
        {
            if (change.Kind == SourceChangeKind::Reset)
            {
                Reapply(change.Added);
            }
            else
            {
                // Removed holds the departed subtree, so no entry outlives its record (M6).
                for (SourceRecord const* const record : change.Removed)
                {
                    auto const it = m_liveDirtyIndex.find(const_cast<SourceRecord*>(record));
                    if (it != m_liveDirtyIndex.end())
                    {
                        m_liveDirty[it->second].Record = nullptr;
                        m_liveDirtyIndex.erase(it);
                    }
                }

                m_commit.chromeDirty = true;
                m_engine.ApplySourceChange(change);
            }
        }
        catch (...)
        {
            fault = std::current_exception();
        }

        // A Reset, a Replace, or a change the engine could not apply surgically rebuilt the list:
        // the owner hears Reset | Membership.
        if ((m_commit.changes & ShapingChangeFlags::Reset) == ShapingChangeFlags::Reset)
        {
            m_commit.changes |= ShapingChangeFlags::Membership;
        }
    }

    if (fault)
    {
        AfterCommit(true /* faulted */);
        std::rethrow_exception(fault);
    }
    AfterCommit();
}

void ShapingManager::OnItemChanged(SourceRecord* record, winrt::hstring const& propertyName)
{
    if (m_sourceTable->IsDetached() || !m_sourceTable->IsLiveShaping() || m_engine.Applied() == ShapeSpec{})
    {
        // Nothing is shaped, or a ParentBy watch (not live shaping) raised it: no row can move (M10).
        return;
    }

    QueueLiveChange(record, propertyName);
    PostLiveDrain();
}

void ShapingManager::QueueLiveChange(SourceRecord* record, winrt::hstring const& propertyName)
{
    auto const [it, added] = m_liveDirtyIndex.try_emplace(record, static_cast<uint32_t>(m_liveDirty.size()));
    if (added)
    {
        try
        {
            m_liveDirty.push_back(LiveChange{ record, propertyName });
        }
        catch (...)
        {
            m_liveDirtyIndex.erase(it);
            throw;
        }
    }
    else
    {
        LiveChange& entry = m_liveDirty[it->second];
        if (entry.PropertyName != propertyName)
        {
            entry.PropertyName = {};
        }
    }
}

void ShapingManager::ClearLiveChanges() noexcept
{
    m_liveDirty.clear();
    m_liveDirtyIndex.clear();
}

void ShapingManager::PostLiveDrain() noexcept
{
    if (m_liveDrainPosted || m_liveDirtyIndex.empty() || !m_liveDrainToken || !m_weakOwnerResolver ||
        m_sourceTable->IsDetached() || !CanDrain())
    {
        return;
    }

    try
    {
        auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
        if (!queue)
        {
            return;
        }

        winrt::weak_ref<winrt::IInspectable> owner = m_weakOwnerResolver();
        std::weak_ptr<ShapingManager*> const token = m_liveDrainToken;
        m_liveDrainPosted = queue.TryEnqueue([owner = std::move(owner), token]()
        {
            // M5: the strong reference keeps TableViewSource2 (and so this object) alive until the
            // drain returns, whatever app code it runs; the token says Clear() has not run.
            // Declared first, so it is released last.
            auto const strongOwner = owner.get();
            if (!strongOwner)
            {
                return;
            }

            auto const alive = token.lock();
            if (!alive)
            {
                return;
            }

            // Not caught (M3): nothing on the stack can receive the error, so it reaches the
            // dispatcher (Application.UnhandledException), as WPF's posted live-shaping work does.
            (*alive)->DrainLiveChanges();
        });
    }
    catch (...)
    {
        m_liveDrainPosted = false;
    }
}

void ShapingManager::DrainLiveChanges()
{
    m_liveDrainPosted = false;
    if (!CanDrain() || m_liveDirtyIndex.empty() || m_sourceTable->IsDetached())
    {
        return;
    }

    // Arrival order, tombstones skipped (M6). Moved out first, so a re-entrant OnItemChanged lands
    // in the fresh list and posts the next drain.
    std::vector<LiveChange> batch;
    batch.reserve(m_liveDirtyIndex.size());
    for (LiveChange& entry : m_liveDirty)
    {
        if (entry.Record)
        {
            batch.push_back(std::move(entry));
        }
    }
    ClearLiveChanges();

    std::exception_ptr fault;
    {
        CommitScope scope{ *this, {} };
        if (batch.size() > c_liveBulkThreshold)
        {
            // A throw wrote nothing and left the engine stale (M3): it unwinds through the scope.
            m_engine.ApplyItemsChanged(batch);
        }
        else
        {
            // Stops at the first throw (M3): entries before it stay committed, the rest are
            // dropped; the engine is stale, so the next rebuild re-reads them.
            try
            {
                for (LiveChange const& entry : batch)
                {
                    m_engine.ApplyItemChanged(entry.Record, entry.PropertyName);
                }
            }
            catch (...)
            {
                fault = std::current_exception();
            }
        }

        if ((m_commit.changes & ShapingChangeFlags::Reset) == ShapingChangeFlags::Reset)
        {
            m_commit.changes |= ShapingChangeFlags::Membership;
        }
    }

    if (fault)
    {
        AfterCommit(true /* faulted */);
        std::rethrow_exception(fault);
    }
    AfterCommit();
}

// ---- Reshapes -----------------------------------------------------------------------------------

void ShapingManager::ApplyDesiredSpec()
{
    ShapeDelta const verbDelta = Diff(m_engine.Applied(), m_desired);
    ShapeDelta delta = verbDelta;
    if (delta == ShapeDelta::None && m_engine.IsStale())
    {
        // A failed source change or live change left cached state the next apply must re-read (M15).
        delta = ShapeDelta::All;
    }

    if (delta != ShapeDelta::None)
    {
        // Pending live changes fold into this rebuild (M4): it re-reads every record, so the verb
        // raises one Reset and no drain runs over keys the rebuild replaced. They move to a local
        // first, so a change queued by app code during the apply is not lost.
        std::vector<LiveChange> folded;
        folded.reserve(m_liveDirtyIndex.size());
        for (LiveChange& entry : m_liveDirty)
        {
            if (entry.Record)
            {
                folded.push_back(std::move(entry));
            }
        }
        ClearLiveChanges();

        bool const foldsLive = !folded.empty();
        if (foldsLive)
        {
            delta |= ShapeDelta::Filter | ShapeDelta::GroupKey | ShapeDelta::Sort;
        }

        // D6, M15: only a filter verb clears the overlay; the folded live delta never does.
        bool const clearOverlay = (verbDelta & ShapeDelta::Filter) != ShapeDelta::None;

        {
            CommitScope scope{ *this, {} };
            try
            {
                m_engine.Apply(m_desired, delta, clearOverlay);
            }
            catch (...)
            {
                // Nothing was applied: the folded changes are still owed, in front of any queued
                // during the apply.
                std::vector<LiveChange> queued = std::move(m_liveDirty);
                ClearLiveChanges();
                for (LiveChange const& entry : folded)
                {
                    QueueLiveChange(entry.Record, entry.PropertyName);
                }
                for (LiveChange const& entry : queued)
                {
                    if (entry.Record)
                    {
                        QueueLiveChange(entry.Record, entry.PropertyName);
                    }
                }
                throw;
            }

            // #18 O5: only the order changed (sort or group direction) -> Order; else Membership.
            // Folded live changes can move rows between groups or in and out of the filter.
            if ((m_commit.changes & ShapingChangeFlags::Reset) == ShapingChangeFlags::Reset)
            {
                bool const orderOnly = !foldsLive && verbDelta != ShapeDelta::None &&
                    (verbDelta & ~(ShapeDelta::Sort | ShapeDelta::GroupOrder)) == ShapeDelta::None;
                m_commit.changes |= orderOnly ? ShapingChangeFlags::Order : ShapingChangeFlags::Membership;
            }
        }
        AfterCommit();
    }

    // The desired spec may hold delegates a DeferRefresh will apply; keep both specs' references.
    ShapeSpec const* const specs[] = { &m_engine.Applied(), &m_desired };
    m_delegates.Retain(specs);
    m_paths.Retain(specs);
}

void ShapingManager::Reapply(std::span<SourceRecord* const> arrivals)
{
    // Every record is re-read.
    ClearLiveChanges();

    if (CanReshape())
    {
        m_engine.ReapplyForSource(m_desired, arrivals);
    }
    else
    {
        // A Reset that reaches here inside a DeferRefresh (a ParentBy adoption raises one directly,
        // bypassing the hold) keeps the applied spec; the verbs in the scope wait for the close
        // (M13). Copied: the engine replaces Applied() while it rebuilds.
        ShapeSpec const applied = m_engine.Applied();
        m_engine.ReapplyForSource(applied, arrivals);
    }
}

// ---- IShapedRowsListener ------------------------------------------------------------------------

void ShapingManager::OnRowsRemoved(uint32_t index, uint32_t count, bool isMove) noexcept
{
    if (isMove)
    {
        m_commit.changes |= ShapingChangeFlags::Order;
        m_commit.moveNet -= static_cast<int64_t>(count);
    }
    else
    {
        m_commit.changes |= ShapingChangeFlags::Membership;
    }

    if (m_commit.listenerFault || !m_viewImpl)
    {
        return;
    }

    try
    {
        m_viewImpl->NotifyRemoved(index, count);
    }
    catch (...)
    {
        m_commit.listenerFault = std::current_exception();
    }
}

void ShapingManager::OnRowsInserted(uint32_t index, uint32_t count, bool isMove) noexcept
{
    if (isMove)
    {
        m_commit.changes |= ShapingChangeFlags::Order;
        m_commit.moveNet += static_cast<int64_t>(count);
    }
    else
    {
        m_commit.changes |= ShapingChangeFlags::Membership;
    }

    if (m_commit.listenerFault || !m_viewImpl)
    {
        return;
    }

    try
    {
        m_viewImpl->NotifyAdded(index, count);
    }
    catch (...)
    {
        m_commit.listenerFault = std::current_exception();
    }
}

void ShapingManager::OnRowsResetting() noexcept
{
    if (m_commit.listenerFault || !m_owner)
    {
        return;
    }

    try
    {
        m_owner->OnResetting();
    }
    catch (...)
    {
        m_commit.listenerFault = std::current_exception();
    }
}

void ShapingManager::OnRowsReset() noexcept
{
    m_commit.changes |= ShapingChangeFlags::Reset;

    if (m_commit.listenerFault || !m_viewImpl)
    {
        return;
    }

    try
    {
        m_viewImpl->NotifyReset();
    }
    catch (...)
    {
        m_commit.listenerFault = std::current_exception();
    }
}

// ---- IShapedExpansion ---------------------------------------------------------------------------

bool ShapingManager::SetExpanded(ShapedNode* node, std::optional<bool> desired)
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2: rows cannot be expanded or collapsed while the source is being shaped.");
    }

    // A filtered-out record has no place to count its rows in.
    if (!node || m_sourceTable->IsDetached() || node->ShapedChildCount() == 0 || !ShapingEngine::InShapedTree(node))
    {
        return false;
    }

    ShapedTree& tree = m_engine.Tree();
    bool const expand = desired.value_or(!tree.IsExpanded(node));
    if (expand == tree.IsExpanded(node))
    {
        return false;
    }

    bool changed = false;
    {
        CommitScope scope{ *this, {} };
        changed = expand ? tree.Expand(node) : tree.Collapse(node);
    }
    AfterCommit();
    return changed;
}

void ShapingManager::SetAllExpanded(ExpansionAxis axis, bool expanded)
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2: rows cannot be expanded or collapsed while the source is being shaped.");
    }

    if (m_sourceTable->IsDetached())
    {
        return;
    }

    {
        CommitScope scope{ *this, {} };
        if (axis == ExpansionAxis::Groups)
        {
            m_engine.Tree().SetAllGroupsExpanded(expanded);
        }
        else
        {
            m_engine.Tree().SetAllRowsExpanded(expanded);
        }
    }
    AfterCommit();
}

void ShapingManager::ExpandSubtree(ShapedNode* node)
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2: rows cannot be expanded or collapsed while the source is being shaped.");
    }

    if (!node || node->IsGroup() || m_sourceTable->IsDetached() || !m_sourceTable->HasHierarchy() ||
        !ShapingEngine::InShapedTree(node))
    {
        return;
    }

    {
        CommitScope scope{ *this, {} };
        m_engine.Tree().ExpandSubtree(node);
    }
    AfterCommit();
}

std::optional<uint32_t> ShapingManager::Reveal(ShapedNode* node)
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2: a row cannot be revealed while the source is being shaped.");
    }

    if (!node || m_sourceTable->IsDetached() || !ShapingEngine::InShapedTree(node))
    {
        return std::nullopt;
    }

    ShapedTree& tree = m_engine.Tree();
    if (!tree.IsShown(node))
    {
        {
            CommitScope scope{ *this, {} };
            tree.Reveal(node);
        }
        AfterCommit();
    }

    // Not linked (a ParentBy-excluded record): nothing was changed, and IndexOf is nullopt.
    return tree.IndexOf(node);
}

// ---- Commit scope -------------------------------------------------------------------------------

void ShapingManager::BeginCommit()
{
    MUX_ASSERT(CanRunVerb());
    m_isApplying = true;
    m_sourceTable->BeginFreeze();
    m_commit = {};
}

void ShapingManager::EndCommit(std::span<SourceRecord const* const> retired, bool unwinding) noexcept
{
    if (m_commit.moveNet != 0)
    {
        m_commit.changes |= ShapingChangeFlags::Membership;
    }

    if (m_commit.listenerFault && m_viewImpl)
    {
        // The view and the repeater may disagree after a suppressed event; one Reset realigns them.
        try
        {
            m_viewImpl->NotifyReset();
        }
        catch (...)
        {
        }
        m_commit.changes |= ShapingChangeFlags::Reset;
    }

    // Also on unwind (M17): rows written before the throw must reach the owner. An engine throw
    // that wrote nothing leaves changes None and retired empty.
    if (m_owner && (m_commit.changes != ShapingChangeFlags::None || !retired.empty() || m_commit.chromeDirty))
    {
        try
        {
            m_owner->OnShapingApplied(ShapingAppliedArgs{ m_commit.changes, retired });
        }
        catch (...)
        {
            if (!m_commit.listenerFault)
            {
                m_commit.listenerFault = std::current_exception();
            }
        }
    }

    m_engine.ReleaseRetired();
    bool const changePending = m_sourceTable->EndFreeze();
    m_isApplying = false;

    // A live change queued during the apply, or a drain skipped because one ran, goes next turn (M9).
    PostLiveDrain();

    // From AfterCommit normally; posted when the scope ends by an exception (never from a
    // destructor path).
    m_commit.resyncDue = changePending && !unwinding;

    if (unwinding)
    {
        // The engine's exception propagates; a handler fault recorded on the way is dropped.
        m_commit.listenerFault = nullptr;
        if (changePending)
        {
            try
            {
                ResyncSource(true /* post */);
            }
            catch (...)
            {
            }
        }
    }
}

void ShapingManager::AfterCommit(bool faulted)
{
    CommitState commit = std::exchange(m_commit, CommitState{});

    // A handler fault or a caught engine fault: post rather than re-read before the error reaches
    // the app.
    std::exception_ptr resyncFault;
    if (commit.resyncDue)
    {
        try
        {
            ResyncSource(faulted || commit.listenerFault != nullptr /* post */);
        }
        catch (...)
        {
            resyncFault = std::current_exception();
        }
    }

    std::exception_ptr endedFault;
    if (m_commitEnded)
    {
        try
        {
            m_commitEnded();
        }
        catch (...)
        {
            endedFault = std::current_exception();
        }
    }

    if (faulted)
    {
        // The caller rethrows its own error.
        return;
    }

    for (std::exception_ptr const& error : { commit.listenerFault, resyncFault, endedFault })
    {
        if (error)
        {
            std::rethrow_exception(error);
        }
    }
}

void ShapingManager::ResyncSource(bool post)
{
    if (post)
    {
        m_sourceTable->PostReread();
        return;
    }

    // Its Reset comes back through OnSourceChanged; a change during it is posted by Refresh. A
    // throw propagates (M14); the table is then diverged with nothing owed (M18).
    m_sourceTable->Refresh();
}

// ---- Gates --------------------------------------------------------------------------------------

bool ShapingManager::BeginVerb(wchar_t const* verb) const
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(
            winrt::hstring{ L"TableViewSource2." } + verb + L" cannot be called while the source is being shaped.");
    }

    return !m_sourceTable->IsDetached();
}

bool ShapingManager::CanRunVerb() const noexcept
{
    return !m_isApplying;
}

bool ShapingManager::CanReshape() const noexcept
{
    return m_deferDepth == 0;
}

bool ShapingManager::CanDrain() const noexcept
{
    return !m_isApplying && m_deferDepth == 0 && m_liveHoldCount == 0;
}
