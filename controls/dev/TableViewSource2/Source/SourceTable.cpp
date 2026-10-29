// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Source/SourceTable.h"

#include <unordered_set>

namespace
{
    // v1 wording, prefixed with the type name. hresult_invalid_argument surfaces as
    // System.ArgumentException in C#.
    constexpr wchar_t c_nullItemMessage[] =
        L"TableViewSource2: A row could not be given a stable identity: null item";
    constexpr wchar_t c_duplicateItemMessage[] =
        L"TableViewSource2: The same item object appears in the source more than once. Rows are identified by object identity, "
        L"so two rows backed by one object cannot be told apart. Use a distinct object per row.";
}

std::shared_ptr<SourceTable> SourceTable::Create(
    ITrackerHandleManager const* owner,
    winrt::Windows::Foundation::IInspectable const& source)
{
    auto table = std::make_shared<SourceTable>(PrivateToken{}, owner);
    table->BindOwnerThread();

    try
    {
        table->AttachSource(source);
    }
    catch (...)
    {
        // ~SourceTable() asserts it was detached; detach while the owner is alive, then rethrow.
        table->Clear();
        throw;
    }

    return table;
}

SourceTable::SourceTable(PrivateToken, ITrackerHandleManager const* owner)
    : m_trackerOwner(owner)
{
}

SourceTable::~SourceTable()
{
    MUX_ASSERT(IsDetached());
}

void SourceTable::BindOwnerThread() noexcept
{
    if (m_binding->threadId.load(std::memory_order_acquire) != 0)
    {
        return;
    }

    try
    {
        if (auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread())
        {
            m_binding->queue = queue;
            m_binding->threadId.store(::GetCurrentThreadId(), std::memory_order_release);
        }
    }
    catch (...)
    {
        // Unbound stays unbound; SetOwner(non-null) binds later.
    }
}

void SourceTable::AttachSource(winrt::Windows::Foundation::IInspectable const& source)
{
    m_sourceAccessor.emplace(m_trackerOwner, source);

    // Milestone 1 reads once and re-reads on Refresh(); SubscribeToSourceChanges() lands in
    // Milestone 2.
    Refresh();
}

void SourceTable::Refresh()
{
    if (IsDetached())
    {
        return;
    }

    auto const items = m_sourceAccessor->Enumerate(true /* throwIfUnsupported */);

    // Compute before commit: nothing below this point runs app code or throws for app data.
    try
    {
        ValidateItems(items);
    }
    catch (...)
    {
        // A failed validation turns a pending re-read into a rejection (see m_rereadDue).
        m_diverged = true;
        m_rereadDue = false;
        throw;
    }

    std::vector<SourceRecordPtr> removed;
    auto const matches = MatchRecords(items, removed);

    std::vector<SourceRecord*> added;
    CommitRecords(items, matches, added);
    ResetTreeToFlat();

    m_diverged = false;
    m_rereadDue = false;

    // Anti-loop (one synchronous re-read per turn): a change skipped during this Reset's dispatch
    // is posted, never re-read here.
    try
    {
        RaiseSourceChanged(SourceChangeKind::Reset, nullptr, nullptr, 0, 0, added, removed);
    }
    catch (...)
    {
        PostRereadIfOwed();
        throw;
    }

    PostRereadIfOwed();
}

void SourceTable::PostRereadIfOwed() noexcept
{
    if (m_rereadDue && !IsFrozen() && m_resetHoldDepth == 0)
    {
        PostReread();
    }
}

void SourceTable::ValidateItems(std::vector<winrt::Windows::Foundation::IInspectable> const& items)
{
    std::unordered_set<ObjectAddress> seen;
    seen.reserve(items.size());

    for (auto const& item : items)
    {
        if (!item)
        {
            throw winrt::hresult_invalid_argument(c_nullItemMessage);
        }

        if (!seen.insert(SourceRecord::AddressOf(item)).second)
        {
            throw winrt::hresult_invalid_argument(c_duplicateItemMessage);
        }
    }
}

std::vector<SourceRecord*> SourceTable::MatchRecords(
    std::vector<winrt::Windows::Foundation::IInspectable> const& items,
    std::vector<SourceRecordPtr>& removed)
{
    // matches[i] is the existing record for items[i], or null when items[i] is new. Records that
    // match no item move to `removed` and leave the lookups; they stay alive until the Reset
    // handler returns.
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

    std::vector<SourceRecordPtr> remaining;
    remaining.reserve(m_records.size());
    for (auto& record : m_records)
    {
        if (kept.count(record.get()))
        {
            remaining.push_back(std::move(record));
        }
        else
        {
            RemoveFromLookups(record.get());
            removed.push_back(std::move(record));
        }
    }

    m_records = std::move(remaining);
    return matches;
}

void SourceTable::CommitRecords(
    std::vector<winrt::Windows::Foundation::IInspectable> const& items,
    std::vector<SourceRecord*> const& matches,
    std::vector<SourceRecord*>& added)
{
    std::unordered_map<SourceRecord*, SourceRecordPtr> owned;
    owned.reserve(m_records.size());
    for (auto& record : m_records)
    {
        SourceRecord* const raw = record.get();
        owned.emplace(raw, std::move(record));
    }

    std::vector<SourceRecordPtr> ordered;
    ordered.reserve(items.size());
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (matches[i])
        {
            ordered.push_back(std::move(owned.at(matches[i])));
        }
        else
        {
            auto record = CreateRecord(items[i]);
            AddToLookups(record.get());
            added.push_back(record.get());
            ordered.push_back(std::move(record));
        }
    }

    m_records = std::move(ordered);
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

void SourceTable::AddToLookups(SourceRecord* record)
{
    m_rawLookup[winrt::get_abi(record->Item.get())] = record;
    m_addressLookup[record->Address] = record;
    if (m_identityIndexEnabled)
    {
        m_identityIndex[record->Identity()] = record;
    }
}

void SourceTable::RemoveFromLookups(SourceRecord* record) noexcept
{
    m_rawLookup.erase(winrt::get_abi(record->Item.get()));
    m_addressLookup.erase(record->Address);
    if (m_identityIndexEnabled)
    {
        m_identityIndex.erase(record->Identity());
    }
}

void SourceTable::Clear()
{
    // Handlers first: from here on no consumer reads Item or Identity().
    m_sourceChanged = nullptr;
    m_itemChanged = nullptr;

    if (m_hierarchy)
    {
        m_hierarchy->Release(*this);
        m_hierarchy.reset();
    }
    m_pendingHierarchy.reset();

    // Lookups are cleared wholesale, without reading any record's Item.
    m_rawLookup.clear();
    m_addressLookup.clear();
    m_identityIndex.clear();
    m_identityIndexEnabled = false;

    m_topLevel.clear();
    m_removedView.clear();

    // ~SourceRecord() revokes through Item.safe_get().
    m_records.clear();
    m_sourceAccessor.reset();

    // No tracker_ref outlives the owner.
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
    return m_records.at(index).get();
}

std::vector<SourceRecordPtr> const& SourceTable::Records() const noexcept
{
    return m_records;
}

void SourceTable::SetSourceChangedHandler(SourceChangedHandler handler)
{
    m_sourceChanged = std::move(handler);
}

void SourceTable::SetItemChangedHandler(ItemChangedHandler handler)
{
    m_itemChanged = std::move(handler);
}

void SourceTable::BeginFreeze() noexcept
{
    ++m_freezeDepth;
}

bool SourceTable::EndFreeze() noexcept
{
    MUX_ASSERT(m_freezeDepth > 0);
    --m_freezeDepth;

    // Reported at the outermost freeze with no hold open (an open hold reports at EndHoldResets).
    return m_freezeDepth == 0 && m_resetHoldDepth == 0 && m_rereadDue;
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

    // Like EndFreeze, reporting does not clear m_rereadDue: only Refresh() does.
    return m_resetHoldDepth == 0 && (m_rereadDue || m_pendingHierarchy.has_value());
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

std::vector<SourceRecord*> const& SourceTable::TopLevel() const noexcept
{
    return m_topLevel;
}

bool SourceTable::HasHierarchy() const noexcept
{
    return m_hierarchy != nullptr;
}

void SourceTable::SetLiveShaping(bool enabled)
{
    // Milestone 6 subscribes and unsubscribes the records here; until then the flag is only stored.
    m_liveShapingEnabled = enabled;
}

bool SourceTable::IsLiveShaping() const noexcept
{
    return m_liveShapingEnabled;
}

SourceRecord* SourceTable::TryFindByRawPointer(winrt::Windows::Foundation::IInspectable const& item) const noexcept
{
    auto const it = m_rawLookup.find(winrt::get_abi(item));
    return it == m_rawLookup.end() ? nullptr : it->second;
}

SourceRecord* SourceTable::TryFindByAddress(ObjectAddress address) const noexcept
{
    auto const it = m_addressLookup.find(address);
    return it == m_addressLookup.end() ? nullptr : it->second;
}

SourceRecord* SourceTable::TryFindByItem(winrt::Windows::Foundation::IInspectable const& item) const
{
    if (!item)
    {
        return nullptr;
    }

    if (SourceRecord* const record = TryFindByRawPointer(item))
    {
        return record;
    }

    // The raw pointer can differ between calls for the same managed object; the canonical
    // IUnknown cannot.
    return TryFindByAddress(SourceRecord::AddressOf(item));
}

// ---- ISourceHierarchyHost ---------------------------------------------------------------------

ITrackerHandleManager const* SourceTable::TrackerOwner() const noexcept
{
    return m_trackerOwner;
}

std::weak_ptr<ISourceHierarchyHost> SourceTable::WeakHost() noexcept
{
    // The base is private, so convert through the aliasing constructor inside the class.
    if (auto self = weak_from_this().lock())
    {
        return std::shared_ptr<ISourceHierarchyHost>(self, static_cast<ISourceHierarchyHost*>(this));
    }

    return {};
}

SourceHierarchy* SourceTable::CurrentHierarchy() noexcept
{
    return m_hierarchy.get();
}

std::vector<SourceRecordPtr> const& SourceTable::RootRecords() const noexcept
{
    return m_records;
}

SourceRecordPtr SourceTable::CreateRecord(winrt::Windows::Foundation::IInspectable const& item)
{
    return std::make_shared<SourceRecord>(m_trackerOwner, item);
}

void SourceTable::ReleaseRecord(SourceRecord* record) noexcept
{
    RemoveFromLookups(record);
}

void SourceTable::ValidateIncoming(
    std::vector<winrt::Windows::Foundation::IInspectable> const& items,
    SourceRecord const* replacing) const
{
    ValidateItems(items);

    for (auto const& item : items)
    {
        SourceRecord const* const existing = TryFindByItem(item);
        if (existing && existing != replacing)
        {
            throw winrt::hresult_invalid_argument(c_duplicateItemMessage);
        }
    }
}

std::vector<SourceRecord*>& SourceTable::SiblingsOf(SourceRecord* parent) noexcept
{
    return parent ? parent->Children : m_topLevel;
}

void SourceTable::Reparent(SourceRecord*, SourceRecord*, uint32_t)
{
    // Called only by a hierarchy strategy (Milestone 3).
    throw winrt::hresult_not_implemented();
}

bool SourceTable::AdmitChange(bool isReset)
{
    if (IsDetached())
    {
        return false;
    }

    // Frozen: nothing changes under the engine. Held: a Reset (or anything after one) waits for
    // the re-read at EndHoldResets; incremental changes are still applied.
    if (IsFrozen() || (m_resetHoldDepth > 0 && (isReset || m_diverged)))
    {
        m_diverged = true;
        m_rereadDue = true;
        return false;
    }

    // A whole-collection change, or any change once diverged, is a re-read. Its throw (validation,
    // enumerator) propagates into the app's mutation. This is the turn's synchronous re-read, so
    // the caller must not DrainPendingChanges afterwards.
    if (isReset || m_diverged)
    {
        Refresh();
        return false;
    }

    return true;
}

void SourceTable::DrainPendingChanges()
{
    // A change skipped during this notification's own dispatch: the turn's one synchronous re-read.
    if (!IsDetached() && !IsFrozen() && m_resetHoldDepth == 0 && m_rereadDue)
    {
        Refresh();
    }
}

void SourceTable::PostReread() noexcept
{
    if (m_rereadPosted || IsDetached())
    {
        return;
    }

    try
    {
        auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
        if (!queue)
        {
            // Headless or test: the re-read stays owed until the next report point or Refresh().
            return;
        }

        m_rereadPosted = queue.TryEnqueue([weakThis = weak_from_this()]()
        {
            auto const self = weakThis.lock();
            if (!self || self->IsDetached())
            {
                return;
            }

            self->m_rereadPosted = false;
            if (!self->m_rereadDue || self->IsFrozen() || self->m_resetHoldDepth > 0)
            {
                // Done already, or left for the open freeze's or hold's report point.
                return;
            }

            try
            {
                self->Refresh();
            }
            catch (...)
            {
                // The flags follow Refresh()'s failure rules.
            }
        });
    }
    catch (...)
    {
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
    m_removedView.clear();
    for (auto const& record : removed)
    {
        m_removedView.push_back(record.get());
    }

    SourceChange const change{ kind, oldParent, newParent, oldIndex, newIndex, added, m_removedView };

    // Frozen around the handler, so a change it causes is skipped rather than nested.
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
        throw;
    }

    (void)EndFreeze();
    m_removedView.clear();
}

void SourceTable::RequestReset() noexcept
{
    m_diverged = true;
    m_rereadDue = true;
}
