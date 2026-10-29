// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Shaping/ShapingManager.h"

#include <algorithm>
#include <exception>
#include <iterator>
#include <utility>

// RAII bracket for one commit. EndCommit runs from the destructor on both normal exit and
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
        m_engine.Reapply(m_desired);
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

void ShapingManager::Refresh()
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2.Refresh cannot be called while the source is being shaped.");
    }

    if (m_sourceTable->IsDetached())
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

void ShapingManager::BeginDeferRefresh()
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2.DeferRefresh cannot be called while the source is being shaped.");
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

    MUX_ASSERT(m_deferDepth > 0);
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
            try
            {
                // One re-read (it also builds a parked hierarchy); its Reset reapplies m_desired.
                m_sourceTable->Refresh();
            }
            catch (...)
            {
                // The records are as they were (compute before commit), so the verbs made in the
                // scope still apply to them (drift #59); the re-read's error is the one surfaced.
                auto const error = std::current_exception();
                try
                {
                    ApplyDesiredSpec();
                }
                catch (...)
                {
                }
                std::rethrow_exception(error);
            }
        }

        // Nothing when the spec did not change (or the re-read applied it); releases dropped
        // delegates either way.
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

void ShapingManager::ApplyDesiredSpec()
{
    ShapeDelta delta = Diff(m_engine.Applied(), m_desired);
    ShapeDelta const verbDelta = delta;
    if (delta == ShapeDelta::None && m_engine.IsStale())
    {
        // A failed source change left cached state the next apply must re-read.
        delta = ShapeDelta::All;
    }
    if (delta != ShapeDelta::None)
    {
        // Pending live changes fold into this rebuild (drift #41): it re-reads every record, so
        // the verb raises one Reset and no drain runs over keys the rebuild replaced. They move to
        // a local first, so a change queued by app code during the apply is not lost.
        decltype(m_liveDirty) folded;
        folded.swap(m_liveDirty);
        bool const foldsLive = !folded.empty();
        if (foldsLive)
        {
            delta |= ShapeDelta::Filter | ShapeDelta::GroupKey | ShapeDelta::Sort;
        }

        {
            CommitScope scope{ *this, {} };
            try
            {
                m_engine.Apply(m_desired, delta);
            }
            catch (...)
            {
                // Nothing was applied: the folded changes are still owed. An entry queued during
                // the apply keeps its (later) sequence; a second name means "re-read everything".
                for (auto& [record, dirty] : folded)
                {
                    auto const [it, added] = m_liveDirty.try_emplace(record, dirty);
                    if (!added)
                    {
                        if (it->second.PropertyName != dirty.PropertyName)
                        {
                            it->second.PropertyName = {};
                        }
                        it->second.Sequence = dirty.Sequence;
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

bool ShapingManager::IsLiveShaping() const
{
    return m_sourceTable->IsLiveShaping();
}

void ShapingManager::IsLiveShaping(bool value)
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2.IsLiveShaping cannot be set while the source is being shaped.");
    }

    if (!m_sourceTable->IsDetached())
    {
        // Turning it off keeps changes already queued: the posted drain still applies them.
        m_sourceTable->SetLiveShaping(value);
    }
}

ShapeSpec const& ShapingManager::AppliedSpec() const noexcept
{
    return m_engine.Applied();
}

void ShapingManager::SetOwner(IShapingOwner* sink)
{
    // A release always succeeds: the listener re-reads m_owner at each callback, so a release from
    // inside a commit only silences the rest of that commit.
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

uint64_t ShapingManager::BeginLiveShapingHold() noexcept
{
    ++m_liveHoldCount;
    return m_ownerGeneration;
}

void ShapingManager::EndLiveShapingHold(uint64_t generation)
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

winrt::Microsoft::UI::Xaml::Interop::IBindableVector ShapingManager::View()
{
    if (m_sourceTable->IsDetached())
    {
        return nullptr;
    }

    if (!m_view)
    {
        auto const view = winrt::make_self<ShapedCollectionView>(m_rows);
        m_view.emplace(m_trackerOwner, view.as<winrt::Microsoft::UI::Xaml::Interop::IBindableVector>());
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
        m_rowMetadata = std::make_shared<ShapedRowMetadata>(m_rows, static_cast<IShapedExpansion&>(*this));
    }

    return m_rowMetadata;
}

void ShapingManager::Clear()
{
    // No callbacks during teardown; nothing below calls get() on a tracker_ref.
    m_owner = nullptr;
    m_commitEnded = nullptr;
    m_liveHoldCount = 0;
    ++m_ownerGeneration;
    m_engine.Tree().SetListener(nullptr);

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

    m_liveDirty.clear();
    m_liveDrainToken.reset();
    m_weakOwnerResolver = nullptr;
    m_liveDrainPosted = false;
    m_deferDepth = 0;
}

void ShapingManager::OnItemChanged(SourceRecord* record, winrt::hstring const& propertyName)
{
    if (m_sourceTable->IsDetached() || !m_sourceTable->IsLiveShaping() || m_engine.Applied() == ShapeSpec{})
    {
        // Nothing is shaped (or a ParentBy watch, not live shaping, raised it): no row can move.
        return;
    }

    auto const [it, added] = m_liveDirty.try_emplace(record, LiveDirty{ propertyName, m_liveSequence });
    if (added)
    {
        ++m_liveSequence;
    }
    else if (it->second.PropertyName != propertyName)
    {
        it->second.PropertyName = {};
    }

    PostLiveDrain();
}

void ShapingManager::PostLiveDrain()
{
    if (m_liveDrainPosted || m_liveDirty.empty() || !m_liveDrainToken || !m_weakOwnerResolver ||
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
            // Drift #42: the strong reference keeps TableViewSource2 (and so this object) alive
            // until the drain returns, whatever app code it runs; the token says Clear() has not
            // run. Declared first, so it is released last.
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

            // Drift #40: nothing on the stack can receive the error of a posted drain, so it is
            // dropped; the tree is consistent (compute before commit) and the failing records
            // keep their old rows until their next change.
            try
            {
                (*alive)->DrainLiveChanges();
            }
            catch (...)
            {
            }
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
    if (!CanDrain() || m_liveDirty.empty() || m_sourceTable->IsDetached())
    {
        return;
    }

    // Arrival order (drift #44), so the events a batch raises do not depend on hashing.
    std::vector<std::pair<SourceRecord*, LiveDirty>> batch(
        std::make_move_iterator(m_liveDirty.begin()), std::make_move_iterator(m_liveDirty.end()));
    m_liveDirty.clear();
    std::sort(batch.begin(), batch.end(), [](auto const& a, auto const& b)
    {
        return a.second.Sequence < b.second.Sequence;
    });

    std::exception_ptr fault;
    {
        CommitScope scope{ *this, {} };
        bool applied = false;
        if (batch.size() > c_liveBulkThreshold)
        {
            std::vector<LiveChange> changes;
            changes.reserve(batch.size());
            for (auto const& [record, dirty] : batch)
            {
                changes.push_back({ record, dirty.PropertyName });
            }

            try
            {
                m_engine.ApplyItemsChanged(changes);
                applied = true;
            }
            catch (...)
            {
                // The rebuild wrote nothing; fall back to one entry at a time below, so only the
                // entries that throw are dropped (drift #40).
            }
        }

        if (!applied)
        {
            // Each change is computed before it is committed, so one that throws leaves the tree
            // as it was and the rest still apply (drift #40).
            for (auto const& [record, dirty] : batch)
            {
                try
                {
                    m_engine.ApplyItemChanged(record, dirty.PropertyName);
                }
                catch (...)
                {
                    if (!fault)
                    {
                        fault = std::current_exception();
                    }
                }
            }
        }

        if ((m_commit.changes & ShapingChangeFlags::Reset) == ShapingChangeFlags::Reset)
        {
            m_commit.changes |= ShapingChangeFlags::Membership;
        }
    }

    if (fault)
    {
        try
        {
            AfterCommit();
        }
        catch (...)
        {
        }
        std::rethrow_exception(fault);
    }
    AfterCommit();
}

void ShapingManager::OnSourceChanged(SourceChange const& change)
{
    // A delegate throw during the re-shape: the engine has already dropped the departed records
    // (ReapplyForSource) and raised a Reset, so the commit ends normally and the owner hears it;
    // the error is rethrown after, into the app's mutation. Nothing is reposted.
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
                // Removed holds the departed subtree, so no stale key survives the change.
                for (SourceRecord const* const record : change.Removed)
                {
                    m_liveDirty.erase(const_cast<SourceRecord*>(record));
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
        try
        {
            AfterCommit();
        }
        catch (...)
        {
        }
        std::rethrow_exception(fault);
    }
    AfterCommit();
}

void ShapingManager::Reapply(std::span<SourceRecord* const> arrivals)
{
    m_liveDirty.clear();
    if (CanReshape())
    {
        m_engine.ReapplyForSource(m_desired, arrivals);
    }
    else
    {
        // A Reset that reaches here inside a DeferRefresh (a ParentBy adoption raises one directly,
        // bypassing the hold) keeps the applied spec; the verbs in the scope wait for the close
        // (drift #62). Copied: the engine replaces Applied() while it rebuilds.
        ShapeSpec const applied = m_engine.Applied();
        m_engine.ReapplyForSource(applied, arrivals);
    }
}

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

    if (!unwinding && m_owner &&
        (m_commit.changes != ShapingChangeFlags::None || !retired.empty() || m_commit.chromeDirty))
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

    // A live change queued during the apply, or a drain skipped because one ran, goes next turn.
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
            ResyncSource(true /* post */);
        }
    }
}

void ShapingManager::AfterCommit()
{
    CommitState commit = std::exchange(m_commit, CommitState{});

    // A handler fault means the owner is in an unknown state: post rather than re-enter it now.
    if (commit.resyncDue)
    {
        ResyncSource(commit.listenerFault != nullptr /* post */);
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

    if (commit.listenerFault)
    {
        std::rethrow_exception(commit.listenerFault);
    }

    if (endedFault)
    {
        std::rethrow_exception(endedFault);
    }
}

void ShapingManager::ResyncSource(bool post) noexcept
{
    if (!post)
    {
        try
        {
            // Its Reset comes back through OnSourceChanged; a change during it is posted by Refresh.
            m_sourceTable->Refresh();
            return;
        }
        catch (...)
        {
            // The verb that opened the scope already succeeded; retry once, posted.
        }
    }

    m_sourceTable->PostReread();
}

void ShapingManager::SetCommitEndedHandler(std::function<void()> handler) noexcept
{
    m_commitEnded = std::move(handler);
}

bool ShapingManager::IsApplying() const noexcept
{
    return m_isApplying;
}

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
