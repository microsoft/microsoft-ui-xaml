// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Source/SourceTable.h"
#include "Source/Hierarchy/ChildCollectionsHierarchy.h"
#include "Source/Hierarchy/ParentKeyHierarchy.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

#include <winrt/Microsoft.UI.Xaml.Data.h>

namespace
{
    using Inspectable = winrt::Windows::Foundation::IInspectable;
    namespace wfc = winrt::Windows::Foundation::Collections;
    namespace interop = winrt::Microsoft::UI::Xaml::Interop;
    namespace dispatching = winrt::Microsoft::UI::Dispatching;
    namespace data = winrt::Microsoft::UI::Xaml::Data;

    // Makes room for one more element the way push_back would (geometric growth), so the insert
    // that follows cannot throw.
    template <typename T>
    void ReserveForInsert(std::vector<T>& values)
    {
        if (values.size() == values.capacity())
        {
            values.reserve(values.size() + (std::max)(values.size() / 2, size_t{ 1 }));
        }
    }

    // Runs body where only allocation can still fail. Unwinding there would leave the table half
    // rebuilt while downstream stages still point at records it has released, so a failure is
    // fatal instead.
    template <typename Body>
    void RunOrFailFast(Body&& body) noexcept
    {
        try
        {
            body();
        }
        catch (...)
        {
            MUX_FAIL_FAST_MSG("TableViewSource2: SourceTable failed after the point of no return.");
        }
    }
}

// ---- Construction and threading ------------------------------------------------------------

std::shared_ptr<SourceTable> SourceTable::Create(
    ITrackerHandleManager const* owner,
    Inspectable const& source)
{
    // The constructor is private, so make_shared cannot reach it.
    auto table = std::shared_ptr<SourceTable>(new SourceTable(owner));

    if (auto const queue = dispatching::DispatcherQueue::GetForCurrentThread())
    {
        table->m_binding->queue = queue;
        table->m_binding->threadId.store(::GetCurrentThreadId(), std::memory_order_release);
    }

    try
    {
        table->AttachSource(source);
    }
    catch (...)
    {
        // Detach while the owner is alive: every tracker_ref the attach created goes now, and the
        // destructor asserts a detached table.
        table->Clear();
        throw;
    }

    return table;
}

SourceTable::SourceTable(ITrackerHandleManager const* owner)
    : m_trackerOwner(owner)
{
    MUX_ASSERT(owner);
}

SourceTable::~SourceTable()
{
    MUX_ASSERT(IsDetached());
}

void SourceTable::BindOwnerThread() noexcept
{
    if (IsDetached())
    {
        return;
    }

    DWORD const current = ::GetCurrentThreadId();
    DWORD const bound = m_binding->threadId.load(std::memory_order_acquire);
    if (bound != 0)
    {
        MUX_ASSERT_MSG(bound == current, "SourceTable is already bound to another thread.");
        return;
    }

    if (auto const queue = dispatching::DispatcherQueue::GetForCurrentThread())
    {
        // Queue first, thread id last with release order: a reader that sees the id sees the queue.
        m_binding->queue = queue;
        m_binding->threadId.store(current, std::memory_order_release);
    }
}

// ---- Source lifecycle ----------------------------------------------------------------------

void SourceTable::AttachSource(Inspectable const& source)
{
    m_sourceAccessor.emplace(m_trackerOwner, source);
    SubscribeToSourceChanges();
    Refresh();
}

void SourceTable::SubscribeToSourceChanges()
{
    // The thread is checked before the table is locked: a lock taken off the bound thread could
    // become the last strong reference and destroy the table there.
    auto const weak = weak_from_this();
    auto const binding = m_binding;

    m_sourceEventToken = m_sourceAccessor->Subscribe(
        [weak, binding](Inspectable const&, interop::NotifyCollectionChangedEventArgs const& args)
        {
            ThrowIfOffBoundThread(*binding);
            if (auto const self = weak.lock(); self && !self->IsDetached())
            {
                self->OnSourceCollectionChanged(args);
            }
        },
        [weak, binding](wfc::IObservableVector<Inspectable> const&, wfc::IVectorChangedEventArgs const& args)
        {
            ThrowIfOffBoundThread(*binding);
            if (auto const self = weak.lock(); self && !self->IsDetached())
            {
                self->OnSourceVectorChanged(args);
            }
        },
        [weak, binding](interop::IBindableObservableVector const&, Inspectable const&)
        {
            ThrowIfOffBoundThread(*binding);
            if (auto const self = weak.lock(); self && !self->IsDetached())
            {
                self->OnSourceBindableVectorChanged();
            }
        });
}

void SourceTable::UnsubscribeFromSourceChanges() noexcept
{
    if (m_sourceAccessor)
    {
        m_sourceAccessor->Unsubscribe(m_sourceEventToken);
    }
}

void SourceTable::OnSourceCollectionChanged(interop::NotifyCollectionChangedEventArgs const& args)
{
    using Action = interop::NotifyCollectionChangedAction;

    auto const action = args.Action();
    auto const newItems = args.NewItems();
    auto const oldItems = args.OldItems();
    int32_t const newIndex = args.NewStartingIndex();
    int32_t const oldIndex = args.OldStartingIndex();
    int32_t const size = static_cast<int32_t>(m_records.size());

    // Applied positionally only when it carries one item and its positions (and, for an item that
    // leaves or moves, the item at that position) agree with the table. Anything else, including
    // a multi-item change, is handled as a Reset.
    auto const isSingle = [](interop::IBindableVector const& items) { return items && items.Size() == 1; };
    auto const isInTable = [size](int32_t index) { return index >= 0 && index < size; };
    auto const holdsOldItem = [&](int32_t index) { return TryFindByItem(oldItems.GetAt(0)) == m_records[index].get(); };

    bool applicable = false;
    switch (action)
    {
    case Action::Add:
        applicable = isSingle(newItems) && newIndex >= 0 && newIndex <= size;
        break;

    case Action::Remove:
        applicable = isSingle(oldItems) && isInTable(oldIndex) && holdsOldItem(oldIndex);
        break;

    case Action::Replace:
        applicable = isSingle(oldItems) && isSingle(newItems) && isInTable(oldIndex) && newIndex == oldIndex &&
            holdsOldItem(oldIndex);
        break;

    case Action::Move:
        applicable = isSingle(oldItems) && isInTable(oldIndex) && isInTable(newIndex) && holdsOldItem(oldIndex);
        break;

    default:
        break;
    }

    if (!AdmitChange(!applicable))
    {
        return;
    }

    switch (action)
    {
    case Action::Add:
        InsertAt(static_cast<uint32_t>(newIndex), newItems.GetAt(0));
        break;

    case Action::Remove:
        RemoveAt(static_cast<uint32_t>(oldIndex));
        break;

    case Action::Replace:
        ReplaceAt(static_cast<uint32_t>(oldIndex), newItems.GetAt(0));
        break;

    case Action::Move:
        Move(static_cast<uint32_t>(oldIndex), static_cast<uint32_t>(newIndex));
        break;

    default:
        break;
    }

    DrainPendingChanges();
}

void SourceTable::OnSourceVectorChanged(wfc::IVectorChangedEventArgs const& args)
{
    using Change = wfc::CollectionChange;

    auto const change = args.CollectionChange();
    uint32_t const size = static_cast<uint32_t>(m_records.size());

    // Vector args carry only an index; the incoming item is read from the source. Index() is not
    // read for a Reset: some implementations throw from it.
    uint32_t const index = change == Change::Reset ? 0u : args.Index();
    Inspectable item{ nullptr };
    bool applicable = false;
    switch (change)
    {
        case Change::ItemInserted:
            applicable = index <= size && m_sourceAccessor->TryGetAt(index, item);
            break;

        case Change::ItemRemoved:
            applicable = index < size;
            break;

        case Change::ItemChanged:
            applicable = index < size && m_sourceAccessor->TryGetAt(index, item);
            break;

        default:
            break;
    }

    if (!AdmitChange(!applicable))
    {
        return;
    }

    switch (change)
    {
    case Change::ItemInserted:
        InsertAt(index, item);
        break;

    case Change::ItemRemoved:
        RemoveAt(index);
        break;

    case Change::ItemChanged:
        ReplaceAt(index, item);
        break;

    default:
        break;
    }

    DrainPendingChanges();
}

void SourceTable::OnSourceBindableVectorChanged()
{
    // IBindableObservableVector carries no usable detail: always a re-read, or skipped while
    // frozen or held. AdmitChange(true) never asks the caller to apply anything.
    (void)AdmitChange(true);
}

void SourceTable::Refresh()
{
    if (IsDetached())
    {
        return;
    }

    MUX_ASSERT(!IsFrozen());

    // A parked verb (DeferRefresh) replaces the current strategy in this re-read.
    bool const swapping = m_pendingHierarchy.has_value();
    SourceHierarchy* const strategy = swapping ? m_pendingHierarchy->get() : m_hierarchy.get();

    // 1. Read: every app call and check. A throw leaves the records untouched (decided, M18):
    // the table is marked diverged with no re-read owed, so the error reaches the app once and
    // the next source notification or Refresh() re-reads (WPF: a Refresh that throws is not
    // retried). A parked verb (DeferRefresh) is dropped, as a hierarchy verb that throws outside
    // a DeferRefresh leaves the old hierarchy; otherwise it would be reported, and re-read, at
    // every report point.
    std::vector<winrt::IInspectable> items;
    try
    {
        items = m_sourceAccessor->Enumerate(true /* throwIfUnsupported */);
        ValidateItems(items);
        if (strategy && !swapping)
        {
            // The committed strategy is tolerant: only a selector throw gets here.
            strategy->Prepare(*this, items);
        }
    }
    catch (...)
    {
        m_diverged = true;
        m_rereadDue = false;
        m_pendingHierarchy.reset();
        throw;
    }

    if (strategy && swapping)
    {
        try
        {
            strategy->Prepare(*this, items);
        }
        catch (...)
        {
            // Decided (D2): the parked verb is dropped and the re-read completes with the current
            // strategy, so the changes made during the deferral still show; then the verb's error.
            auto const error = std::current_exception();
            m_pendingHierarchy.reset();
            Refresh();
            std::rethrow_exception(error);
        }
    }

    // 2. Match and 3. link. Nothing below calls app code.
    std::vector<SourceRecordPtr> removed;
    std::vector<SourceRecord*> added;
    RunOrFailFast([&]
    {
        auto const matches = MatchRecords(items, removed);
        if (swapping)
        {
            if (m_hierarchy)
            {
                m_hierarchy->TakeChildRecords(*this, removed);
                m_hierarchy->Release(*this);
            }
            m_hierarchy = std::move(*m_pendingHierarchy);
            m_pendingHierarchy.reset();
        }
        else if (m_hierarchy)
        {
            m_hierarchy->MatchForRefresh(*this, matches, removed);
        }

        // Parked records leave with this Reset, unless still in the table or already reported.
        for (auto& zombie : m_zombies)
        {
            bool const inTable = TryFindByAddress(zombie->Address) == zombie.get();
            bool const reported = std::any_of(removed.begin(), removed.end(), [&zombie](SourceRecordPtr const& record) { return record == zombie; });
            if (!inTable && !reported)
            {
                removed.push_back(std::move(zombie));
            }
        }
        m_zombies.clear();

        CommitRecords(items, matches, added);
        ResetTreeToFlat();
        if (m_hierarchy)
        {
            m_hierarchy->Commit(*this, added);
        }
    });

    // Cleared before the Reset is raised: the table matches the source now, so a change a Reset
    // handler makes is skipped and owed again rather than lost.
    m_diverged = false;
    m_rereadDue = false;

    // 4. One Reset. Anti-loop (one synchronous re-read per turn): a re-read owed by this dispatch
    // is posted, never run here. A handler throw posts it on its way out of RaiseSourceChanged.
    RaiseSourceChanged(SourceChangeKind::Reset, nullptr, nullptr, 0, 0, added, removed);
    PostRereadIfOwed();
}

void SourceTable::ValidateItems(std::vector<Inspectable> const& items)
{
    std::unordered_set<ObjectAddress> seen;
    seen.reserve(items.size());

    for (auto const& item : items)
    {
        if (!item)
        {
            throw winrt::hresult_invalid_argument(L"TableViewSource2: A row could not be given a stable identity: null item");
        }

        if (!seen.insert(SourceRecord::AddressOf(item)).second)
        {
            throw winrt::hresult_invalid_argument(
                L"TableViewSource2: The same item object appears in the source more than once. Rows are identified by object identity, "
                L"so two rows backed by one object cannot be told apart. Use a distinct object per row.");
        }
    }
}

std::vector<SourceRecord*> SourceTable::MatchRecords(
    std::vector<Inspectable> const& items,
    std::vector<SourceRecordPtr>& removed)
{
    // Items are validated distinct and each lookup key maps to one record, so no record is matched
    // twice.
    std::vector<SourceRecord*> matches;
    matches.reserve(items.size());

    std::unordered_set<SourceRecord*> kept;
    kept.reserve(items.size());

    for (auto const& item : items)
    {
        SourceRecord* const match = TryFindByItem(item);
        matches.push_back(match);
        if (match)
        {
            kept.insert(match);
        }
    }

    // Compacts the kept records in place; the rest leave the lookups and move to removed, which
    // keeps them alive until the Reset handler returns.
    size_t keep = 0;
    for (auto& record : m_records)
    {
        if (kept.contains(record.get()))
        {
            if (&m_records[keep] != &record)
            {
                m_records[keep] = std::move(record);
            }
            ++keep;
        }
        else
        {
            RemoveFromLookups(record.get());
            removed.push_back(std::move(record));
        }
    }
    m_records.resize(keep);

    return matches;
}

void SourceTable::CommitRecords(
    std::vector<Inspectable> const& items,
    std::vector<SourceRecord*> const& matches,
    std::vector<SourceRecord*>& added)
{
    // The kept records' owning pointers, by raw pointer, so they can be placed in items order.
    std::unordered_map<SourceRecord*, SourceRecordPtr> kept;
    kept.reserve(m_records.size());
    for (auto& record : m_records)
    {
        SourceRecord* const raw = record.get();
        kept.emplace(raw, std::move(record));
    }

    std::vector<SourceRecordPtr> ordered;
    ordered.reserve(items.size());
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (SourceRecord* const match = matches[i])
        {
            ordered.push_back(std::move(kept.at(match)));
        }
        else
        {
            ordered.push_back(CreateRecord(items[i]));
            added.push_back(ordered.back().get());
        }
    }

    m_records = std::move(ordered);
}

void SourceTable::Clear()
{
    if (IsDetached())
    {
        return;
    }

    // Handlers first: nothing is raised from here on, so no consumer reads Item or Identity().
    m_sourceChanged = nullptr;
    m_itemChanged = nullptr;

    // The strategies hold tracker_refs (selectors, keys, child feeds): release them while the
    // owner is alive.
    m_pendingHierarchy.reset();
    if (m_hierarchy)
    {
        m_hierarchy->Release(*this);
        m_hierarchy.reset();
    }

    UnsubscribeFromSourceChanges();

    // Wholesale: teardown never reads Item.get(), since the GC may already have collected the
    // items. ~SourceRecord revokes through safe_get().
    m_rawLookup.clear();
    m_addressLookup.clear();
    m_identityIndex.clear();
    m_identityIndexEnabled = false;
    m_topLevel.clear();
    m_removedView.clear();
    m_records.clear();
    m_zombies.clear();
    m_sourceAccessor.reset();

    m_trackerOwner = nullptr;
}

bool SourceTable::IsDetached() const noexcept
{
    return m_trackerOwner == nullptr;
}

uint32_t SourceTable::Size() const noexcept
{
    return static_cast<uint32_t>(m_records.size());
}

SourceRecord* SourceTable::At(uint32_t index) const
{
    MUX_ASSERT(index < m_records.size());
    return m_records[index].get();
}

std::vector<SourceRecordPtr> const& SourceTable::Records() const noexcept
{
    return m_records;
}

void SourceTable::SetSourceChangedHandler(SourceChangedHandler handler)
{
    if (!IsDetached())
    {
        m_sourceChanged = std::move(handler);
    }
}

void SourceTable::SetItemChangedHandler(ItemChangedHandler handler)
{
    if (!IsDetached())
    {
        m_itemChanged = std::move(handler);
    }
}

// ---- Freeze, reset hold, divergence --------------------------------------------------------

void SourceTable::BeginFreeze() noexcept
{
    ++m_freezeDepth;
}

bool SourceTable::EndFreeze() noexcept
{
    MUX_ASSERT(m_freezeDepth > 0);
    --m_freezeDepth;

    // A rejection alone (m_diverged without m_rereadDue) is not reported: re-reading would fail
    // validation again at every commit.
    return m_freezeDepth == 0 && m_resetHoldDepth == 0 && !IsDetached() &&
        (m_rereadDue || m_pendingHierarchy.has_value());
}

bool SourceTable::IsFrozen() const noexcept
{
    return m_freezeDepth > 0;
}

void SourceTable::BeginHoldResets() noexcept
{
    ++m_resetHoldDepth;
}

bool SourceTable::EndHoldResets() noexcept
{
    MUX_ASSERT(m_resetHoldDepth > 0);
    --m_resetHoldDepth;

    // Inside a freeze, the outermost EndFreeze reports instead.
    return m_resetHoldDepth == 0 && m_freezeDepth == 0 && !IsDetached() &&
        (m_rereadDue || m_pendingHierarchy.has_value());
}

void SourceTable::ScheduleRefresh() noexcept
{
    MUX_ASSERT(m_resetHoldDepth > 0);
    m_rereadDue = true;
}

bool SourceTable::IsRereadDue() const noexcept
{
    return m_rereadDue;
}

bool SourceTable::AdmitChange(bool isReset)
{
    if (IsFrozen() || (m_resetHoldDepth > 0 && (m_diverged || isReset)))
    {
        // Skipped: the table stays as it was and the re-read is owed to the report point.
        m_diverged = true;
        m_rereadDue = true;
        return false;
    }

    if (m_diverged || isReset)
    {
        // Positional deltas are never applied to a diverged table. A validation throw propagates
        // into the app's Add / Insert / setter.
        Refresh();
        return false;
    }

    return true;
}

void SourceTable::DrainPendingChanges()
{
    // The dispatch's own report point: a change its handler made was skipped. This is the turn's
    // one synchronous re-read; a change made during its Reset is posted by Refresh().
    if (!IsDetached() && m_rereadDue && !IsFrozen() && m_resetHoldDepth == 0)
    {
        Refresh();
    }
}

void SourceTable::PostRereadIfOwed() noexcept
{
    if (m_rereadDue && !IsFrozen() && m_resetHoldDepth == 0)
    {
        PostReread();
    }
}

void SourceTable::PostReread() noexcept
{
    if (IsDetached() || m_rereadPosted)
    {
        return;
    }

    auto const queue = dispatching::DispatcherQueue::GetForCurrentThread();
    if (!queue)
    {
        // Headless or test: the re-read stays owed until the next report point or Refresh().
        return;
    }

    m_rereadPosted = true;

    bool posted = false;
    try
    {
        posted = queue.TryEnqueue([weak = weak_from_this()]()
        {
            auto const self = weak.lock();
            if (!self || self->IsDetached())
            {
                return;
            }

            self->m_rereadPosted = false;

            // Frozen or held: that freeze's or hold's report point takes the owed re-read.
            if (self->IsRereadDue() && !self->IsFrozen() && self->m_resetHoldDepth == 0)
            {
                // Decided (#7, revised): a throw is reported through Application.UnhandledException
                // (ReportPostedError), as WPF lets its posted work fail. The flags follow
                // Refresh()'s failure rules; nothing re-posts.
                try
                {
                    self->Refresh();
                }
                catch (...)
                {
                    ReportPostedError();
                }
            }
        });
    }
    catch (...)
    {
    }

    if (!posted)
    {
        // The queue is shutting down: nothing will run, so a later report point may post again.
        m_rereadPosted = false;
    }
}

// ---- Source edits (root collection) --------------------------------------------------------

void SourceTable::InsertAt(uint32_t index, Inspectable const& item)
{
    MUX_ASSERT(index <= m_records.size());

    try
    {
        ValidateIncoming(CollectIncomingItems(item), nullptr);
    }
    catch (...)
    {
        // The source already holds the item: the table has diverged until a re-read succeeds.
        // The strategy's read of item is dropped (H5).
        if (m_hierarchy)
        {
            m_hierarchy->DiscardIncoming();
        }
        m_diverged = true;
        throw;
    }

    InsertCollected(index, item);
}

void SourceTable::InsertCollected(uint32_t index, Inspectable const& item)
{
    MUX_ASSERT(index <= m_records.size());

    std::vector<SourceRecord*> added;
    std::vector<SourceRecordPtr> removed;
    SourceRecordPtr record;
    try
    {
        // Room first, so nothing after the record exists can throw.
        ReserveForInsert(m_records);
        ReserveForInsert(m_topLevel);
        added.reserve(1);
        record = CreateRecord(item);
    }
    catch (...)
    {
        if (m_hierarchy)
        {
            m_hierarchy->DiscardIncoming();
        }
        m_diverged = true;
        throw;
    }

    SourceRecord* const raw = record.get();
    m_records.insert(m_records.begin() + index, std::move(record));

    if (m_hierarchy)
    {
        // The strategy links the record and raises the Add and what follows (decided, A1).
        try
        {
            m_hierarchy->OnInserted(*this, raw);
        }
        catch (...)
        {
            PostRereadIfOwed();
            throw;
        }
        return;
    }

    m_topLevel.insert(m_topLevel.begin() + index, raw);
    added.push_back(raw);

    RaiseSourceChanged(SourceChangeKind::Add, nullptr, nullptr, 0, index, added, removed);
}

void SourceTable::RemoveAt(uint32_t index)
{
    MUX_ASSERT(index < m_records.size());

    std::vector<SourceRecordPtr> removed;
    try
    {
        removed.reserve(1);
    }
    catch (...)
    {
        m_diverged = true;
        throw;
    }

    // removed keeps the record alive (Item, Identity()) until the handler returns.
    removed.push_back(std::move(m_records[index]));
    m_records.erase(m_records.begin() + index);
    SourceRecord* const raw = removed.front().get();
    RemoveFromLookups(raw);

    if (m_hierarchy)
    {
        // The strategy unlinks the record and raises the Remove and what precedes it (decided, A1).
        try
        {
            m_hierarchy->OnRemoving(*this, raw, index, removed);
        }
        catch (...)
        {
            // A fault part way: whatever is still linked or alive is parked until the re-read
            // (requested by the strategy) reports it.
            RunOrFailFast([&]
            {
                auto& siblings = SiblingsOf(raw->Parent);
                auto const it = std::find(siblings.begin(), siblings.end(), raw);
                if (it != siblings.end())
                {
                    siblings.erase(it);
                }
                raw->Parent = nullptr;

                m_zombies.reserve(m_zombies.size() + removed.size());
                for (auto& record : removed)
                {
                    m_zombies.push_back(std::move(record));
                }
                removed.clear();
            });
            RequestReset();
            PostRereadIfOwed();
            throw;
        }
        return;
    }

    m_topLevel.erase(m_topLevel.begin() + index);
    RaiseSourceChanged(SourceChangeKind::Remove, nullptr, nullptr, index, 0, {}, removed);
}

void SourceTable::ReplaceAt(uint32_t index, Inspectable const& item)
{
    MUX_ASSERT(index < m_records.size());

    SourceRecord* const old = m_records[index].get();

    if (m_hierarchy)
    {
        // Decided (D1): validate against the leaving record and subtree, then remove and insert.
        // H5: item is read once; the insert is built from that read.
        try
        {
            ValidateIncoming(CollectIncomingItems(item), old);
        }
        catch (...)
        {
            m_hierarchy->DiscardIncoming();
            m_diverged = true;
            throw;
        }

        try
        {
            RemoveAt(index);
        }
        catch (...)
        {
            // The item is not inserted: the table has diverged.
            if (m_hierarchy)
            {
                m_hierarchy->DiscardIncoming();
            }
            RequestReset();
            throw;
        }

        InsertCollected(index, item);
        return;
    }

    std::vector<SourceRecord*> added;
    std::vector<SourceRecordPtr> removed;
    SourceRecordPtr record;
    try
    {
        added.reserve(1);
        removed.reserve(1);
        ValidateIncoming({ item }, old);

        // Always a new record, even for the same object. It takes over any lookup key it shares
        // with the old record, which then releases only the keys it still holds.
        record = CreateRecord(item);
    }
    catch (...)
    {
        m_diverged = true;
        throw;
    }

    RemoveFromLookups(old);

    SourceRecord* const raw = record.get();
    removed.push_back(std::exchange(m_records[index], std::move(record)));
    m_topLevel[index] = raw;
    added.push_back(raw);

    RaiseSourceChanged(SourceChangeKind::Replace, nullptr, nullptr, index, index, added, removed);
}

void SourceTable::Move(uint32_t fromIndex, uint32_t toIndex)
{
    MUX_ASSERT(fromIndex < m_records.size() && toIndex < m_records.size());

    if (fromIndex == toIndex)
    {
        return;
    }

    // A rotation moves one element without allocating, so the edit cannot fail.
    auto const moveOne = [fromIndex, toIndex](auto& values)
    {
        auto const begin = values.begin();
        if (fromIndex < toIndex)
        {
            std::rotate(begin + fromIndex, begin + fromIndex + 1, begin + toIndex + 1);
        }
        else
        {
            std::rotate(begin + toIndex, begin + fromIndex, begin + fromIndex + 1);
        }
    };
    moveOne(m_records);

    std::vector<SourceRecordPtr> removed;
    if (m_hierarchy)
    {
        // The record keeps its parent; only its place among its siblings can change.
        SourceRecord* const record = m_records[toIndex].get();
        SourceRecord* const parent = record->Parent;
        auto const& siblings = SiblingsOf(parent);
        uint32_t const oldIndex = IndexInSourceSiblings(siblings, record);
        bool const linked = oldIndex < siblings.size();
        try
        {
            m_hierarchy->OnMoved(*this, record, fromIndex, toIndex);
            uint32_t const newIndex = IndexInSourceSiblings(siblings, record);
            if (linked && newIndex != oldIndex)
            {
                RaiseSourceChanged(SourceChangeKind::Move, parent, parent, oldIndex, newIndex, {}, removed);
            }
        }
        catch (...)
        {
            PostRereadIfOwed();
            throw;
        }
        return;
    }

    moveOne(m_topLevel);
    RaiseSourceChanged(SourceChangeKind::Move, nullptr, nullptr, fromIndex, toIndex, {}, removed);
}

// ---- Source tree (hierarchy) ---------------------------------------------------------------

std::vector<SourceRecord*> const& SourceTable::TopLevel() const noexcept
{
    return m_topLevel;
}

bool SourceTable::HasHierarchy() const noexcept
{
    return m_hierarchy != nullptr;
}

void SourceTable::SetParentKeyHierarchy(
    winrt::TableViewKeySelector const& keySelector,
    winrt::TableViewKeySelector const& parentKeySelector)
{
    if (!keySelector || !parentKeySelector)
    {
        throw winrt::hresult_invalid_argument(L"TableViewSource2.ParentBy: keySelector and parentKeySelector must not be null.");
    }

    if (IsDetached())
    {
        return;
    }

    ReplaceHierarchy(std::make_unique<ParentKeyHierarchy>(m_trackerOwner, keySelector, parentKeySelector));
}

void SourceTable::SetChildCollectionHierarchy(winrt::TableViewChildrenSelector2 const& childrenSelector)
{
    if (!childrenSelector)
    {
        throw winrt::hresult_invalid_argument(L"TableViewSource2.ChildrenBy: childrenSelector must not be null.");
    }

    if (IsDetached())
    {
        return;
    }

    ReplaceHierarchy(std::make_unique<ChildCollectionsHierarchy>(m_trackerOwner, childrenSelector));
}

void SourceTable::ClearHierarchy()
{
    ReplaceHierarchy(nullptr);
}

void SourceTable::ReplaceHierarchy(std::unique_ptr<SourceHierarchy> next)
{
    if (IsDetached())
    {
        return;
    }

    if (m_resetHoldDepth > 0)
    {
        // DeferRefresh: park the unprepared strategy; the re-read at the outermost Close prepares
        // and commits it (Refresh). The current tree keeps receiving changes meanwhile.
        if (!next && !m_hierarchy && !m_pendingHierarchy.has_value())
        {
            return;
        }

        m_pendingHierarchy = std::move(next);
        return;
    }

    // Not held: a verb applies now, superseding anything parked.
    m_pendingHierarchy.reset();

    if (!next && !m_hierarchy)
    {
        return;
    }

    // Compute before commit: every app call and check writes only next's scratch state.
    if (next)
    {
        std::vector<Inspectable> items;
        items.reserve(m_records.size());
        for (auto const& record : m_records)
        {
            items.push_back(record->Item.get());
        }

        next->Prepare(*this, items);
    }

    std::vector<SourceRecordPtr> removed;
    std::vector<SourceRecord*> added;
    RunOrFailFast([&]
    {
        if (m_hierarchy)
        {
            m_hierarchy->TakeChildRecords(*this, removed);
            m_hierarchy->Release(*this);
        }
        m_hierarchy = std::move(next);

        ResetTreeToFlat();
        if (m_hierarchy)
        {
            m_hierarchy->Commit(*this, added);
        }
    });

    RaiseSourceChanged(SourceChangeKind::Reset, nullptr, nullptr, 0, 0, added, removed);
    PostRereadIfOwed();
}

void SourceTable::ResetTreeToFlat() noexcept
{
    m_topLevel.clear();
    m_topLevel.reserve(m_records.size());
    for (auto const& record : m_records)
    {
        record->Parent = nullptr;
        record->Children.clear();
        record->Depth = 0;
        m_topLevel.push_back(record.get());
    }
}

bool SourceTable::ReevaluateHierarchy(SourceRecord* record, winrt::hstring const& propertyName)
{
    // The same gate as a single-item change: frozen or held, it is skipped and a re-read is owed;
    // diverged, it re-reads now.
    if (!m_hierarchy || !AdmitChange(false))
    {
        return false;
    }

    bool changed = false;
    try
    {
        changed = m_hierarchy->Reevaluate(*this, record, propertyName);
    }
    catch (...)
    {
        // A selector or key error (after the tree changes were raised) or a listener fault.
        PostRereadIfOwed();
        throw;
    }

    DrainPendingChanges();
    return changed;
}

// ---- Live shaping --------------------------------------------------------------------------

void SourceTable::SetLiveShaping(bool enabled)
{
    if (IsDetached() || enabled == m_liveShapingEnabled)
    {
        return;
    }

    m_liveShapingEnabled = enabled;
    auto const apply = [this, enabled](SourceRecordPtr const& record)
    {
        if (enabled)
        {
            SubscribeRecordPropertyChanged(record);
        }
        else
        {
            UnsubscribeRecordPropertyChanged(record.get());
        }
    };

    for (auto const& record : m_records)
    {
        apply(record);
    }

    if (m_hierarchy)
    {
        m_hierarchy->ForEachChildRecord(apply);
    }
}

bool SourceTable::IsLiveShaping() const noexcept
{
    return m_liveShapingEnabled;
}

void SourceTable::SubscribeRecordPropertyChanged(SourceRecordPtr const& record) noexcept
{
    // Subscribed already, or probed once and found not observable (kept across toggles).
    if (record->Watch != WatchState::Unchecked)
    {
        return;
    }

    try
    {
        auto const notifier = record->Item.get().try_as<data::INotifyPropertyChanged>();
        if (!notifier)
        {
            record->Watch = WatchState::NotObservable;
            return;
        }

        record->PropertyChangedToken = notifier.PropertyChanged(
            [weakTable = weak_from_this(), weakRecord = SourceRecordWeakPtr{ record }, binding = m_binding](
                Inspectable const&, data::PropertyChangedEventArgs const& args)
            {
                auto const propertyName = args.PropertyName();

                DWORD const bound = binding->threadId.load(std::memory_order_acquire);
                if (bound != 0 && bound != ::GetCurrentThreadId())
                {
                    // Only the weak pointers and the name cross threads; they are locked on the
                    // bound thread. A failed post means the queue is shutting down: ignored.
                    (void)binding->queue.TryEnqueue([weakTable, weakRecord, propertyName]()
                    {
                        try
                        {
                            if (auto const table = weakTable.lock(); table && !table->IsDetached())
                            {
                                table->OnRecordPropertyChanged(weakRecord, propertyName);
                            }
                        }
                        catch (...)
                        {
                            ReportPostedError();
                        }
                    });
                    return;
                }

                if (auto const table = weakTable.lock(); table && !table->IsDetached())
                {
                    table->OnRecordPropertyChanged(weakRecord, propertyName);
                }
            });
        record->Watch = WatchState::Subscribed;
    }
    catch (...)
    {
        // Decided (#5): no retry; this item's property edits are not live-shaped.
        record->Watch = WatchState::NotObservable;
    }
}

void SourceTable::UnsubscribeRecordPropertyChanged(SourceRecord* record) noexcept
{
    if (record->Watch != WatchState::Subscribed)
    {
        return;
    }

    auto const token = std::exchange(record->PropertyChangedToken, winrt::event_token{});
    record->Watch = WatchState::Unchecked;
    try
    {
        if (auto const notifier = record->Item.safe_get().try_as<data::INotifyPropertyChanged>())
        {
            notifier.PropertyChanged(token);
        }
    }
    catch (...)
    {
    }
}

void SourceTable::OnRecordPropertyChanged(SourceRecordWeakPtr const& weakRecord, winrt::hstring const& propertyName)
{
    // A record that has left the table may still be alive for a Remove or Reset dispatch; it is
    // no longer a row.
    auto const record = weakRecord.lock();
    if (!record || TryFindByAddress(record->Address) != record.get())
    {
        return;
    }

    std::exception_ptr error;
    if (m_hierarchy)
    {
        try
        {
            (void)ReevaluateHierarchy(record.get(), propertyName);
        }
        catch (...)
        {
            // The tree change is already applied; the row's value change is still reported.
            error = std::current_exception();
        }
    }

    if (m_itemChanged && TryFindByAddress(record->Address) == record.get())
    {
        m_itemChanged(record.get(), propertyName);
    }

    if (error)
    {
        std::rethrow_exception(error);
    }
}

// ---- Lookups -------------------------------------------------------------------------------

SourceRecord* SourceTable::TryFindByRawPointer(Inspectable const& item) const noexcept
{
    if (!item)
    {
        return nullptr;
    }

    auto const it = m_rawLookup.find(winrt::get_abi(item));
    return it != m_rawLookup.end() ? it->second : nullptr;
}

SourceRecord* SourceTable::TryFindByAddress(ObjectAddress address) const noexcept
{
    auto const it = m_addressLookup.find(address);
    return it != m_addressLookup.end() ? it->second : nullptr;
}

SourceRecord* SourceTable::TryFindByItem(Inspectable const& item) const
{
    if (!item)
    {
        return nullptr;
    }

    if (auto const record = TryFindByRawPointer(item))
    {
        return record;
    }

    return TryFindByAddress(SourceRecord::AddressOf(item));
}

void SourceTable::EnableIdentityIndex()
{
    if (IsDetached() || m_identityIndexEnabled)
    {
        return;
    }

    // The address lookup covers every record in the tree. Built aside, so a throw leaves the
    // index off.
    std::unordered_map<winrt::hstring, SourceRecord*> index;
    index.reserve(m_addressLookup.size());
    for (auto const& entry : m_addressLookup)
    {
        index.emplace(entry.second->Identity(), entry.second);
    }

    m_identityIndex = std::move(index);
    m_identityIndexEnabled = true;
}

void SourceTable::DisableIdentityIndex() noexcept
{
    m_identityIndexEnabled = false;
    m_identityIndex.clear();
}

bool SourceTable::HasIdentityIndex() const noexcept
{
    return m_identityIndexEnabled;
}

SourceRecord* SourceTable::TryFindByIdentity(winrt::hstring const& identity) const
{
    if (!m_identityIndexEnabled)
    {
        return nullptr;
    }

    auto const it = m_identityIndex.find(identity);
    return it != m_identityIndex.end() ? it->second : nullptr;
}

void SourceTable::AddToLookups(SourceRecord* record)
{
    // Assigns rather than inserts: a Replace by the same object creates the new record while the
    // old one still holds the keys (see ReplaceAt).
    m_rawLookup.insert_or_assign(winrt::get_abi(record->Item.get()), record);
    m_addressLookup.insert_or_assign(record->Address, record);
    if (m_identityIndexEnabled)
    {
        m_identityIndex.insert_or_assign(record->Identity(), record);
    }
}

void SourceTable::RemoveFromLookups(SourceRecord* record) noexcept
{
    // Erases a key only while it still maps to this record.
    auto const eraseIfOwned = [record](auto& map, auto const& key)
    {
        auto const it = map.find(key);
        if (it != map.end() && it->second == record)
        {
            map.erase(it);
        }
    };

    eraseIfOwned(m_rawLookup, winrt::get_abi(record->Item.get()));
    eraseIfOwned(m_addressLookup, record->Address);
    if (m_identityIndexEnabled)
    {
        eraseIfOwned(m_identityIndex, record->Identity());
    }
}

// ---- ISourceHierarchyHost ------------------------------------------------------------------

ITrackerHandleManager const* SourceTable::TrackerOwner() const noexcept
{
    return m_trackerOwner;
}

std::weak_ptr<ISourceHierarchyHost> SourceTable::WeakHost() noexcept
{
    auto const self = weak_from_this().lock();
    if (!self)
    {
        return {};
    }

    // Aliasing constructor: shares the table's control block. The cast to the private base is
    // made here, where it is accessible.
    return std::shared_ptr<ISourceHierarchyHost>(self, static_cast<ISourceHierarchyHost*>(this));
}

SourceHierarchy* SourceTable::CurrentHierarchy() noexcept
{
    return m_hierarchy.get();
}

std::vector<SourceRecordPtr> const& SourceTable::RootRecords() const noexcept
{
    return m_records;
}

std::shared_ptr<ThreadBinding const> SourceTable::Binding() const noexcept
{
    return m_binding;
}

std::vector<Inspectable> SourceTable::CollectIncomingItems(Inspectable const& item)
{
    std::vector<Inspectable> items{ item };
    if (m_hierarchy && item)
    {
        m_hierarchy->CollectIncoming(item, items);
    }
    return items;
}

bool SourceTable::IsLeavingWith(SourceRecord const* existing, SourceRecord const* replacing) const noexcept
{
    if (!replacing || existing == replacing)
    {
        return existing == replacing;
    }

    bool underReplacing = false;
    for (SourceRecord const* ancestor = existing->Parent; ancestor; ancestor = ancestor->Parent)
    {
        if (ancestor == replacing)
        {
            underReplacing = true;
            break;
        }
    }

    if (!underReplacing)
    {
        return false;
    }

    // A ParentBy descendant is another root record and stays; a ChildrenBy child record leaves.
    return std::none_of(m_records.begin(), m_records.end(),
        [existing](SourceRecordPtr const& record) { return record.get() == existing; });
}

SourceRecordPtr SourceTable::CreateRecord(Inspectable const& item)
{
    auto record = std::make_shared<SourceRecord>(m_trackerOwner, item);
    try
    {
        AddToLookups(record.get());
    }
    catch (...)
    {
        // No lookup may outlive the record.
        RemoveFromLookups(record.get());
        throw;
    }

    if (m_liveShapingEnabled)
    {
        SubscribeRecordPropertyChanged(record);
    }

    return record;
}

void SourceTable::ReleaseRecord(SourceRecord* record) noexcept
{
    RemoveFromLookups(record);
}

void SourceTable::ValidateIncoming(std::vector<Inspectable> const& items, SourceRecord const* replacing) const
{
    // Duplicates within items need a set only when there is more than one.
    std::unordered_set<ObjectAddress> seen;

    for (auto const& item : items)
    {
        if (!item)
        {
            throw winrt::hresult_invalid_argument(L"TableViewSource2: A row could not be given a stable identity: null item");
        }

        SourceRecord const* const existing = TryFindByItem(item);
        if (existing && existing != replacing && !IsLeavingWith(existing, replacing))
        {
            throw winrt::hresult_invalid_argument(
                L"TableViewSource2: The same item object appears in the source more than once. Rows are identified by object identity, "
                L"so two rows backed by one object cannot be told apart. Use a distinct object per row.");
        }

        if (items.size() > 1 && !seen.insert(SourceRecord::AddressOf(item)).second)
        {
            throw winrt::hresult_invalid_argument(
                L"TableViewSource2: The same item object appears in the source more than once. Rows are identified by object identity, "
                L"so two rows backed by one object cannot be told apart. Use a distinct object per row.");
        }
    }
}

std::vector<SourceRecord*>& SourceTable::SiblingsOf(SourceRecord* parent) noexcept
{
    return parent ? parent->Children : m_topLevel;
}

void SourceTable::Reparent(SourceRecord* record, SourceRecord* newParent, uint32_t index)
{
    // A record that is not linked yet (a new root record) is simply linked.
    auto& oldSiblings = SiblingsOf(record->Parent);
    auto const it = std::find(oldSiblings.begin(), oldSiblings.end(), record);
    if (it != oldSiblings.end())
    {
        oldSiblings.erase(it);
    }

    auto& newSiblings = SiblingsOf(newParent);
    newSiblings.insert(newSiblings.begin() + std::min<size_t>(index, newSiblings.size()), record);
    record->Parent = newParent;
    UpdateDepths(record);
}

void SourceTable::UpdateDepths(SourceRecord* record)
{
    uint16_t const depth = record->Parent ? static_cast<uint16_t>(record->Parent->Depth + 1) : uint16_t{ 0 };
    if (record->Depth == depth)
    {
        // The subtree was consistent under the old position at this depth already.
        return;
    }

    record->Depth = depth;

    // Pre-order, linear: each stack entry is a record whose children still need their depth.
    std::vector<SourceRecord*> pending{ record };
    while (!pending.empty())
    {
        SourceRecord* const parent = pending.back();
        pending.pop_back();
        for (SourceRecord* const child : parent->Children)
        {
            child->Depth = static_cast<uint16_t>(parent->Depth + 1);
            pending.push_back(child);
        }
    }
}

void SourceTable::RaiseSourceChanged(
    SourceChangeKind kind,
    SourceRecord* oldParent,
    SourceRecord* newParent,
    uint32_t oldIndex,
    uint32_t newIndex,
    std::vector<SourceRecord*> const& added,
    std::vector<SourceRecordPtr>& removed)
{
    // The table is already edited: failing to raise would leave downstream stages on records it
    // has released.
    RunOrFailFast([&]
    {
        m_removedView.clear();
        m_removedView.reserve(removed.size());
        for (auto const& record : removed)
        {
            m_removedView.push_back(record.get());
        }
    });

    SourceChange const change{ kind, oldParent, newParent, oldIndex, newIndex, added, m_removedView };

    // Frozen around the handler, so a change it causes is skipped rather than nested. The report
    // is taken by the caller's report point (DrainPendingChanges, Refresh's PostRereadIfOwed).
    BeginFreeze();
    try
    {
        if (m_sourceChanged)
        {
            m_sourceChanged(change);
        }
    }
    catch (...)
    {
        (void)EndFreeze();
        m_removedView.clear();

        // The removed records may still be referenced downstream: parked until the next Reset
        // reports them.
        RunOrFailFast([&]
        {
            m_zombies.reserve(m_zombies.size() + removed.size());
            for (auto& record : removed)
            {
                m_zombies.push_back(std::move(record));
            }
            removed.clear();
        });

        // The throw skips the caller's report point: post a re-read this dispatch owes.
        PostRereadIfOwed();
        throw;
    }

    (void)EndFreeze();
    m_removedView.clear();

    // The records that left are destroyed now, after every handler has dropped them.
    removed.clear();
}

void SourceTable::RequestReset() noexcept
{
    m_diverged = true;
}
