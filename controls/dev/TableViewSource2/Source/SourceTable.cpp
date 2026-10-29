// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Source/SourceTable.h"

#include <algorithm>
#include <exception>
#include <unordered_set>

#include <winrt/Microsoft.UI.Xaml.Data.h>

namespace
{
    // v1 wording, prefixed with the type name. hresult_invalid_argument surfaces as
    // System.ArgumentException in C#.
    constexpr wchar_t c_nullItemMessage[] =
        L"TableViewSource2: A row could not be given a stable identity: null item";
    constexpr wchar_t c_duplicateItemMessage[] =
        L"TableViewSource2: The same item object appears in the source more than once. Rows are identified by object identity, "
        L"so two rows backed by one object cannot be told apart. Use a distinct object per row.";

    // Decided (drift #12): a null selector is rejected at the verb.
    constexpr wchar_t c_nullKeySelectorMessage[] =
        L"TableViewSource2.ParentBy: keySelector and parentKeySelector are required.";

    constexpr wchar_t c_nullChildrenSelectorMessage[] =
        L"TableViewSource2.ChildrenBy: childrenSelector is required.";
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

    // Read first: a From() that fails validation leaves no handler on the app's collection.
    Refresh();
    SubscribeToSourceChanges();
}

void SourceTable::ThrowIfOffBoundThread(ThreadBinding const& binding)
{
    // An off-thread source change is an app error: reject it in the app's mutation, before the
    // table is locked or touched (see Threading).
    DWORD const threadId = binding.threadId.load(std::memory_order_acquire);
    if (threadId != 0 && threadId != ::GetCurrentThreadId())
    {
        throw winrt::hresult_wrong_thread();
    }
}

void SourceTable::SubscribeToSourceChanges()
{
    using ObserveKind = CollectionAccessor2::ObserveKind;
    std::weak_ptr<SourceTable> weakThis = weak_from_this();

    switch (m_sourceAccessor->GetObserveKind())
    {
    case ObserveKind::NotifyCollectionChanged:
        m_sourceEventToken = m_sourceAccessor->AsNotifyCollectionChanged().CollectionChanged(
            [weakThis, binding = m_binding](auto const&, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
            {
                ThrowIfOffBoundThread(*binding);
                if (auto const self = weakThis.lock())
                {
                    self->OnSourceCollectionChanged(args);
                }
            });
        break;

    case ObserveKind::ObservableVector:
        m_sourceEventToken = m_sourceAccessor->AsObservableVector().VectorChanged(
            [weakThis, binding = m_binding](auto const&, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
            {
                ThrowIfOffBoundThread(*binding);
                if (auto const self = weakThis.lock())
                {
                    self->OnSourceVectorChanged(args);
                }
            });
        break;

    case ObserveKind::BindableObservableVector:
        m_sourceEventToken = m_sourceAccessor->AsBindableObservableVector().VectorChanged(
            [weakThis, binding = m_binding](auto const&, winrt::Windows::Foundation::IInspectable const&)
            {
                ThrowIfOffBoundThread(*binding);
                if (auto const self = weakThis.lock())
                {
                    self->OnSourceBindableVectorChanged();
                }
            });
        break;

    default:
        break;
    }
}

void SourceTable::UnsubscribeFromSourceChanges() noexcept
{
    if (!m_sourceAccessor || !m_sourceEventToken.value)
    {
        return;
    }

    using ObserveKind = CollectionAccessor2::ObserveKind;
    auto const token = std::exchange(m_sourceEventToken, {});

    try
    {
        // safe_get: a collected source is skipped.
        switch (m_sourceAccessor->GetObserveKind())
        {
        case ObserveKind::NotifyCollectionChanged:
            if (auto const source = m_sourceAccessor->AsNotifyCollectionChanged(true))
            {
                source.CollectionChanged(token);
            }
            break;

        case ObserveKind::ObservableVector:
            if (auto const source = m_sourceAccessor->AsObservableVector(true))
            {
                source.VectorChanged(token);
            }
            break;

        case ObserveKind::BindableObservableVector:
            if (auto const source = m_sourceAccessor->AsBindableObservableVector(true))
            {
                source.VectorChanged(token);
            }
            break;

        default:
            break;
        }
    }
    catch (...)
    {
    }
}

void SourceTable::OnSourceCollectionChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    using Action = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

    if (IsDetached())
    {
        return;
    }

    auto const action = args.Action();
    auto const newItems = args.NewItems();
    auto const oldItems = args.OldItems();
    int32_t const newIndex = args.NewStartingIndex();
    int32_t const oldIndex = args.OldStartingIndex();
    auto const size = static_cast<int32_t>(m_records.size());

    // Apply only a single-item change whose positions and old item match the table.
    auto const oneOf = [](auto const& items) { return items && items.Size() == 1; };
    auto const recordIs = [this](int32_t index, auto const& items)
    {
        return TryFindByItem(items.GetAt(0)) == m_records[index].get();
    };

    bool isReset = true;
    switch (action)
    {
    case Action::Add:
        isReset = !oneOf(newItems) || newIndex < 0 || newIndex > size;
        break;
    case Action::Remove:
        isReset = !oneOf(oldItems) || oldIndex < 0 || oldIndex >= size || !recordIs(oldIndex, oldItems);
        break;
    case Action::Replace:
        isReset = !oneOf(newItems) || !oneOf(oldItems) || oldIndex < 0 || oldIndex >= size ||
            newIndex != oldIndex || !recordIs(oldIndex, oldItems);
        break;
    case Action::Move:
        isReset = !oneOf(oldItems) || oldIndex < 0 || oldIndex >= size || newIndex < 0 || newIndex >= size ||
            !recordIs(oldIndex, oldItems);
        break;
    default:
        break;
    }

    if (!AdmitChange(isReset))
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

void SourceTable::OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
{
    using Change = winrt::Windows::Foundation::Collections::CollectionChange;

    if (IsDetached())
    {
        return;
    }

    auto const change = args.CollectionChange();
    auto const size = static_cast<uint32_t>(m_records.size());

    // Index is not read for a Reset: some implementations throw from it.
    uint32_t index = 0;
    winrt::Windows::Foundation::IInspectable item{ nullptr };
    bool isReset = true;
    switch (change)
    {
    case Change::ItemInserted:
        index = args.Index();
        isReset = index > size || !m_sourceAccessor->TryGetAt(index, item);
        break;
    case Change::ItemRemoved:
        index = args.Index();
        isReset = index >= size;
        break;
    case Change::ItemChanged:
        index = args.Index();
        isReset = index >= size || !m_sourceAccessor->TryGetAt(index, item);
        break;
    default:
        break;
    }

    if (!AdmitChange(isReset))
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
    // IBindableObservableVector carries no detail: always a re-read (or skipped while frozen or
    // held). AdmitChange(true) never asks the caller to apply anything.
    (void)AdmitChange(true);
}

void SourceTable::Refresh()
{
    if (IsDetached())
    {
        return;
    }

    // A parked verb (DeferRefresh) replaces the current strategy at this re-read.
    bool const swapping = m_pendingHierarchy.has_value();
    SourceHierarchy* const strategy = swapping ? m_pendingHierarchy->get() : m_hierarchy.get();

    // 1. Read: every app call and check. A throw leaves the table untouched.
    std::vector<winrt::Windows::Foundation::IInspectable> items;
    try
    {
        items = m_sourceAccessor->Enumerate(true /* throwIfUnsupported */);

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

        if (strategy)
        {
            if (swapping)
            {
                try
                {
                    strategy->Prepare(*this, items);
                }
                catch (...)
                {
                    // The parked strategy cannot be applied: drop it, finish the re-read with the
                    // current one so edits made during the deferral show, then surface the error.
                    auto const error = std::current_exception();
                    m_pendingHierarchy.reset();
                    Refresh();
                    std::rethrow_exception(error);
                }
            }
            else
            {
                try
                {
                    strategy->Prepare(*this, items);
                }
                catch (...)
                {
                    m_diverged = true;
                    m_rereadDue = false;
                    throw;
                }
            }
        }
    }
    catch (...)
    {
        // A parked strategy that cannot be read is discarded; the current one stays.
        m_pendingHierarchy.reset();
        throw;
    }

    // 2. Match: nothing below runs app code or throws for app data.
    std::vector<SourceRecordPtr> removed;
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

    // Records a failed remove left in the engine's tree leave with this Reset.
    for (auto& zombie : m_zombies)
    {
        removed.push_back(std::move(zombie));
    }
    m_zombies.clear();

    // 3. Link.
    std::vector<SourceRecord*> added;
    CommitRecords(items, matches, added);
    ResetTreeToFlat();
    if (m_hierarchy)
    {
        m_hierarchy->Commit(*this, added);
    }

    m_diverged = false;
    m_rereadDue = false;

    // 4. One Reset. Anti-loop (one synchronous re-read per turn): a change skipped during this
    // Reset's dispatch is posted, never re-read here.
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

    // The lookups also hold child records (ChildrenBy); a root item matches root records only.
    std::unordered_set<SourceRecord*> roots;
    roots.reserve(m_records.size());
    for (auto const& record : m_records)
    {
        roots.insert(record.get());
    }

    for (auto const& item : items)
    {
        SourceRecord* match = TryFindByItem(item);
        if (match && !roots.count(match))
        {
            match = nullptr;
        }
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

void SourceTable::Clear()
{
    // Handlers first: from here on no consumer reads Item or Identity().
    m_sourceChanged = nullptr;
    m_itemChanged = nullptr;

    // Before anything is released: no notification reaches a half-cleared table.
    UnsubscribeFromSourceChanges();

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
    m_zombies.clear();

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
    return m_resetHoldDepth == 0 && m_freezeDepth == 0 &&
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
    if (m_liveShapingEnabled == enabled)
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

void SourceTable::SubscribeRecordPropertyChanged(SourceRecordPtr const& record) noexcept
{
    // NotObservable stays cached across toggles: an item is probed once.
    if (record->Watch != WatchState::Unchecked)
    {
        return;
    }

    auto const notifier = record->Item.get().try_as<winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged>();
    if (!notifier)
    {
        record->Watch = WatchState::NotObservable;
        return;
    }

    // A subscribe failure must not unwind a record commit; the item is treated as not observable.
    try
    {
        record->PropertyChangedToken = notifier.PropertyChanged(
            [weakThis = weak_from_this(), weakRecord = SourceRecordWeakPtr{ record }, binding = m_binding](
                auto const&, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs const& args)
            {
                auto const name = args.PropertyName();
                DWORD const threadId = binding->threadId.load(std::memory_order_acquire);
                if (threadId != 0 && threadId != ::GetCurrentThreadId())
                {
                    try
                    {
                        (void)binding->queue.TryEnqueue([weakThis, weakRecord, name]()
                        {
                            if (auto const self = weakThis.lock())
                            {
                                self->OnRecordPropertyChanged(weakRecord, name);
                            }
                        });
                    }
                    catch (...)
                    {
                        // A failed post is ignored.
                    }
                    return;
                }

                if (auto const self = weakThis.lock())
                {
                    self->OnRecordPropertyChanged(weakRecord, name);
                }
            });
        record->Watch = WatchState::Subscribed;
    }
    catch (...)
    {
        record->PropertyChangedToken = {};
        record->Watch = WatchState::NotObservable;
    }
}

void SourceTable::UnsubscribeRecordPropertyChanged(SourceRecord* record) noexcept
{
    if (record->Watch != WatchState::Subscribed)
    {
        return;
    }

    auto const token = std::exchange(record->PropertyChangedToken, {});
    record->Watch = WatchState::Unchecked;
    try
    {
        if (auto const notifier = record->Item.safe_get().try_as<winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged>())
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
    if (IsDetached())
    {
        return;
    }

    auto const record = weakRecord.lock();
    if (!record)
    {
        return;
    }

    // ParentBy: a key may have changed. Its error propagates into the app's setter.
    (void)ReevaluateHierarchy(record.get(), propertyName);

    // ShapingManager::OnItemChanged: queues the record for the next live drain.
    if (!m_itemChanged || IsDetached())
    {
        return;
    }

    m_itemChanged(record.get(), propertyName);
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
    auto record = std::make_shared<SourceRecord>(m_trackerOwner, item);
    try
    {
        AddToLookups(record.get());
    }
    catch (...)
    {
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

void SourceTable::ValidateIncoming(
    std::vector<winrt::Windows::Foundation::IInspectable> const& items,
    SourceRecord const* replacing) const
{
    ValidateItems(items);

    for (auto const& item : items)
    {
        SourceRecord const* const existing = TryFindByItem(item);
        if (existing && existing != replacing && !IsLeavingWith(existing, replacing))
        {
            throw winrt::hresult_invalid_argument(c_duplicateItemMessage);
        }
    }
}

bool SourceTable::IsLeavingWith(SourceRecord const* existing, SourceRecord const* replacing) const noexcept
{
    // A ChildrenBy descendant of the leaving root leaves with it. A ParentBy child is another
    // root record, which stays.
    if (!replacing)
    {
        return false;
    }

    for (SourceRecord const* above = existing->Parent; above; above = above->Parent)
    {
        if (above == replacing)
        {
            return std::none_of(m_records.begin(), m_records.end(), [existing](SourceRecordPtr const& root)
            {
                return root.get() == existing;
            });
        }
    }

    return false;
}

std::vector<winrt::Windows::Foundation::IInspectable> SourceTable::CollectIncomingItems(
    winrt::Windows::Foundation::IInspectable const& item) const
{
    std::vector<winrt::Windows::Foundation::IInspectable> incoming{ item };
    if (m_hierarchy && item)
    {
        m_hierarchy->CollectIncoming(item, incoming);
    }
    return incoming;
}

void SourceTable::InsertAt(uint32_t index, winrt::Windows::Foundation::IInspectable const& item)
{
    MUX_ASSERT(index <= m_records.size());

    try
    {
        // ChildrenBy: the item brings its whole subtree; every item in it is checked first.
        ValidateIncoming(CollectIncomingItems(item), nullptr);
    }
    catch (...)
    {
        // The source now holds an item the table rejected: the next notification re-reads.
        m_diverged = true;
        throw;
    }

    auto record = CreateRecord(item);
    SourceRecord* const raw = record.get();
    m_records.insert(m_records.begin() + index, std::move(record));

    std::vector<SourceRecordPtr> removed;
    if (!m_hierarchy)
    {
        m_topLevel.insert(m_topLevel.begin() + index, raw);
        RaiseEdit(SourceChangeKind::Add, nullptr, nullptr, 0, index, { raw }, removed);
        return;
    }

    // Spec Misbehaving records: a bad key or a throwing selector never undoes the insert. The
    // strategy places the record (or excludes it: no Add), the Add is raised, the strategy then
    // raises what follows from it (ParentBy adoption: one Move, or one Reset for more than one),
    // and the first error propagates into the app's Add / Insert.
    std::exception_ptr error;
    if (m_hierarchy->OnInserted(*this, raw, error))
    {
        std::vector<SourceRecord*> added;
        AppendSourceSubtree(raw, added);
        try
        {
            RaiseEdit(
                SourceChangeKind::Add,
                nullptr,
                raw->Parent,
                0,
                IndexInSourceSiblings(SiblingsOf(raw->Parent), raw),
                added,
                removed);
        }
        catch (...)
        {
            // AfterInserted (orphan adoption) is skipped: the next notification re-reads.
            RequestReset();
            throw;
        }

        try
        {
            m_hierarchy->AfterInserted(*this, raw);
        }
        catch (...)
        {
            // The strategy's own bookkeeping (a forced-top record to restore) may be stale.
            RequestReset();
            PostRereadIfOwed();
            throw;
        }
    }

    if (error)
    {
        std::rethrow_exception(error);
    }
}

void SourceTable::RemoveAt(uint32_t index)
{
    MUX_ASSERT(index < m_records.size());

    // Compute: room for the record's zombie slot, while nothing is detached yet, so parking it
    // after an OnRemoving fault cannot throw (a bad_alloc there would free a record the engine
    // still points to).
    m_zombies.reserve(m_zombies.size() + 1);

    std::vector<SourceRecordPtr> removed;
    removed.push_back(std::move(m_records[index]));
    m_records.erase(m_records.begin() + index);
    SourceRecord* const raw = removed.front().get();
    RemoveFromLookups(raw);

    // `removed` keeps the records alive (Item, Identity()) until the handler returns.
    if (!m_hierarchy)
    {
        m_topLevel.erase(m_topLevel.begin() + index);
        RaiseEdit(SourceChangeKind::Remove, nullptr, nullptr, index, 0, {}, removed);
        return;
    }

    // ParentBy moves its children to the top level first (one Move each, raised while it is
    // still in the shaped tree); ChildrenBy appends its subtree to removed. An excluded record
    // is in no sibling list and raises nothing.
    bool linked = false;
    try
    {
        linked = m_hierarchy->OnRemoving(*this, raw, removed);
    }
    catch (...)
    {
        // The orphan Moves raised before the fault already happened; the record is still linked
        // in its sibling list and the engine's tree. Finish detaching it from the table and keep
        // it alive until the re-read drops its engine nodes, so no pointer outlives its owner.
        SourceRecord* const parent = raw->Parent;
        auto& siblings = SiblingsOf(parent);
        auto const it = std::find(siblings.begin(), siblings.end(), raw);
        if (it != siblings.end())
        {
            siblings.erase(it);
        }
        raw->Parent = nullptr;

        // noexcept: a subtree that outgrew the reserved slot fails fast rather than freeing records
        // the engine still holds.
        [&]() noexcept {
            m_zombies.reserve(m_zombies.size() + removed.size());
            for (auto& record : removed)
            {
                m_zombies.push_back(std::move(record));
            }
        }();
        RequestReset();
        PostRereadIfOwed();
        throw;
    }

    if (!linked)
    {
        return;
    }

    SourceRecord* const oldParent = raw->Parent;
    auto& siblings = SiblingsOf(oldParent);
    uint32_t const oldIndex = IndexInSourceSiblings(siblings, raw);
    siblings.erase(siblings.begin() + oldIndex);
    raw->Parent = nullptr;
    RaiseEdit(SourceChangeKind::Remove, oldParent, nullptr, oldIndex, 0, {}, removed);
}

void SourceTable::ReplaceAt(uint32_t index, winrt::Windows::Foundation::IInspectable const& item)
{
    MUX_ASSERT(index < m_records.size());

    if (m_hierarchy)
    {
        // Drift #16: validate the incoming item (and, under ChildrenBy, its subtree) against the
        // leaving record and its leaving subtree first, so a rejection writes nothing; then a
        // Remove and an Add, each placed by the strategy (InsertAt validates again; the children
        // selector may run twice).
        try
        {
            ValidateIncoming(CollectIncomingItems(item), m_records[index].get());
        }
        catch (...)
        {
            m_diverged = true;
            throw;
        }

        try
        {
            RemoveAt(index);
        }
        catch (...)
        {
            // The Add never runs: the source holds the new item, so re-read.
            RequestReset();
            throw;
        }

        InsertAt(index, item);
        return;
    }

    SourceRecord* const old = m_records[index].get();
    try
    {
        ValidateIncoming({ item }, old);
    }
    catch (...)
    {
        m_diverged = true;
        throw;
    }

    // Always a new record, even for the same object: the old one leaves the lookups first.
    auto record = CreateRecord(item);
    SourceRecord* const raw = record.get();
    RemoveFromLookups(old);

    std::vector<SourceRecordPtr> removed;
    removed.push_back(std::exchange(m_records[index], std::move(record)));
    m_topLevel[index] = raw;

    RaiseEdit(SourceChangeKind::Replace, nullptr, nullptr, index, index, { raw }, removed);
}

void SourceTable::Move(uint32_t fromIndex, uint32_t toIndex)
{
    MUX_ASSERT(fromIndex < m_records.size() && toIndex < m_records.size());

    if (fromIndex == toIndex)
    {
        return;
    }

    auto moving = std::move(m_records[fromIndex]);
    SourceRecord* const raw = moving.get();
    m_records.erase(m_records.begin() + fromIndex);
    m_records.insert(m_records.begin() + toIndex, std::move(moving));

    std::vector<SourceRecordPtr> removed;
    if (!m_hierarchy)
    {
        m_topLevel.erase(m_topLevel.begin() + fromIndex);
        m_topLevel.insert(m_topLevel.begin() + toIndex, raw);
        RaiseEdit(SourceChangeKind::Move, nullptr, nullptr, fromIndex, toIndex, {}, removed);
        return;
    }

    // The record keeps its parent; only its place among its siblings can change.
    SourceRecord* const parent = raw->Parent;
    uint32_t const oldSibling = IndexInSourceSiblings(SiblingsOf(parent), raw);
    m_hierarchy->OnMoved(*this, raw, fromIndex, toIndex);
    uint32_t const newSibling = IndexInSourceSiblings(SiblingsOf(parent), raw);
    if (oldSibling != newSibling)
    {
        RaiseEdit(SourceChangeKind::Move, parent, parent, oldSibling, newSibling, {}, removed);
    }
}

void SourceTable::RaiseEdit(
    SourceChangeKind kind,
    SourceRecord* oldParent,
    SourceRecord* newParent,
    uint32_t oldIndex,
    uint32_t newIndex,
    std::vector<SourceRecord*> const& added,
    std::vector<SourceRecordPtr>& removed)
{
    // A listener fault skips DrainPendingChanges: post a re-read owed by this dispatch, as Refresh
    // does.
    try
    {
        RaiseSourceChanged(kind, oldParent, newParent, oldIndex, newIndex, added, removed);
    }
    catch (...)
    {
        PostRereadIfOwed();
        throw;
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

bool SourceTable::ReevaluateHierarchy(SourceRecord* record, winrt::hstring const& propertyName)
{
    // The same gate as a single-item change (drift #17): frozen or held, it is skipped and a
    // re-read is owed; diverged, it re-reads now.
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

        // The handler's throw skips DrainPendingChanges: post a re-read owed by this dispatch for
        // every raiser, including the hierarchy strategies' own raises (idempotent with RaiseEdit).
        PostRereadIfOwed();
        throw;
    }

    (void)EndFreeze();
    m_removedView.clear();
}

void SourceTable::RequestReset() noexcept
{
    // Header contract: m_diverged only; the next notification re-reads.
    m_diverged = true;
}

std::function<void()> SourceTable::MakeThreadCheck() const
{
    return [binding = m_binding]() { ThrowIfOffBoundThread(*binding); };
}

// ---- Hierarchy verbs --------------------------------------------------------------------------

void SourceTable::SetParentKeyHierarchy(
    winrt::TableViewKeySelector const& keySelector,
    winrt::TableViewKeySelector const& parentKeySelector)
{
    if (!keySelector || !parentKeySelector)
    {
        throw winrt::hresult_invalid_argument(c_nullKeySelectorMessage);
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
        throw winrt::hresult_invalid_argument(c_nullChildrenSelectorMessage);
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
        // and commits it. The current tree keeps receiving changes meanwhile.
        if (!next && !m_hierarchy && !m_pendingHierarchy.has_value())
        {
            return;
        }

        m_pendingHierarchy = std::move(next);
        return;
    }

    if (!next && !m_hierarchy)
    {
        return;
    }

    // Compute before commit: every app call and check writes only next's scratch state.
    if (next)
    {
        std::vector<winrt::Windows::Foundation::IInspectable> items;
        items.reserve(m_records.size());
        for (auto const& record : m_records)
        {
            items.push_back(record->Item.get());
        }

        next->Prepare(*this, items);
    }

    std::vector<SourceRecordPtr> removed;
    if (m_hierarchy)
    {
        m_hierarchy->TakeChildRecords(*this, removed);
        m_hierarchy->Release(*this);
    }
    m_hierarchy = std::move(next);

    std::vector<SourceRecord*> added;
    ResetTreeToFlat();
    if (m_hierarchy)
    {
        m_hierarchy->Commit(*this, added);
    }

    RaiseEdit(SourceChangeKind::Reset, nullptr, nullptr, 0, 0, added, removed);
}
