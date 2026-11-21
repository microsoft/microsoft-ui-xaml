// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Source/Hierarchy/ChildCollectionsHierarchy.h"

#include <algorithm>

namespace
{
    constexpr wchar_t c_nullChildMessage[] =
        L"TableViewSource2.ChildrenBy: a child collection contains a null item.";
    constexpr wchar_t c_duplicateChildMessage[] =
        L"TableViewSource2.ChildrenBy: the same item object appears in the tree more than once. "
        L"Each item can appear only once.";

    // Room for one more element, growing geometrically, so the push or insert that follows
    // cannot throw.
    template <typename T>
    void ReserveForOneMore(std::vector<T>& values)
    {
        if (values.size() == values.capacity())
        {
            values.reserve(std::max<size_t>(4, values.capacity() * 2));
        }
    }
}

ChildCollectionsHierarchy::ChildCollectionsHierarchy(
    ITrackerHandleManager const* owner,
    winrt::TableViewChildrenSelector2 const& childrenSelector)
{
    m_childrenSelector.emplace(owner, childrenSelector);
}

// ---- Reading ---------------------------------------------------------------------------------

void ChildCollectionsHierarchy::ReadSubtree(
    winrt::Windows::Foundation::IInspectable const& item,
    std::vector<StagedNode>& nodes,
    std::unordered_set<ObjectAddress>& seen,
    std::exception_ptr* selectorError) const
{
    auto const selector = m_childrenSelector->get();

    // The children of nodes[index]; empty when it has no collection (or, incremental, the
    // selector or the enumeration threw: the item is a leaf).
    auto const readChildren = [&](size_t index) -> std::vector<winrt::Windows::Foundation::IInspectable>
    {
        try
        {
            auto const collection = selector(nodes[index].Item);
            if (!collection)
            {
                return {};
            }

            auto children = CollectionAccessor2::Enumerate(collection, true /* throwIfUnsupported */);
            nodes[index].ChildCollection = collection;
            return children;
        }
        catch (...)
        {
            if (!selectorError)
            {
                throw;
            }

            if (!*selectorError)
            {
                *selectorError = std::current_exception();
            }
            return {};
        }
    };

    // Explicit stack (H2): one frame per item whose children are still being appended.
    struct Frame
    {
        size_t Index;
        std::vector<winrt::Windows::Foundation::IInspectable> Children;
        size_t Next{ 0 };
    };
    std::vector<Frame> pending;

    size_t const root = nodes.size();
    nodes.push_back(StagedNode{ item });
    if (item)
    {
        auto children = readChildren(root);
        pending.push_back(Frame{ root, std::move(children) });
    }

    while (!pending.empty())
    {
        Frame& top = pending.back();
        if (top.Next == top.Children.size())
        {
            nodes[top.Index].SubtreeSize = static_cast<uint32_t>(nodes.size() - top.Index - 1);
            pending.pop_back();
            continue;
        }

        auto child = std::move(top.Children[top.Next++]);
        bool expand = false;
        if (!child)
        {
            if (!selectorError)
            {
                throw winrt::hresult_invalid_argument(c_nullChildMessage);
            }
        }
        else if (!seen.insert(SourceRecord::AddressOf(child)).second)
        {
            if (!selectorError)
            {
                throw winrt::hresult_invalid_argument(c_duplicateChildMessage);
            }
        }
        else
        {
            expand = true;
        }

        // A null or repeated child stays a leaf in the read; ValidateIncoming rejects it.
        size_t const index = nodes.size();
        nodes.push_back(StagedNode{ std::move(child) });
        if (expand)
        {
            auto children = readChildren(index);
            pending.push_back(Frame{ index, std::move(children) });
        }
    }
}

std::vector<ChildCollectionsHierarchy::StagedNode> ChildCollectionsHierarchy::ReadIncoming(
    winrt::Windows::Foundation::IInspectable const& item,
    std::exception_ptr& selectorError) const
{
    std::vector<StagedNode> nodes;
    std::unordered_set<ObjectAddress> seen;
    if (item)
    {
        seen.insert(SourceRecord::AddressOf(item));
    }

    ReadSubtree(item, nodes, seen, &selectorError);
    return nodes;
}

void ChildCollectionsHierarchy::AppendStagedItems(
    std::vector<StagedNode> const& nodes,
    size_t first,
    std::vector<winrt::Windows::Foundation::IInspectable>& items)
{
    size_t const end = first + 1 + nodes[first].SubtreeSize;
    items.reserve(items.size() + (end - first - 1));
    for (size_t i = first + 1; i < end; ++i)
    {
        items.push_back(nodes[i].Item);
    }
}

// ---- Verb and Refresh ------------------------------------------------------------------------

void ChildCollectionsHierarchy::Prepare(
    ISourceHierarchyHost&,
    std::vector<winrt::Windows::Foundation::IInspectable> const& rootItems)
{
    // m_staged: each root item followed by its subtree, pre-order. The host validated the root
    // items; the read rejects a null child and any object seen twice in the whole tree.
    m_staged.clear();
    try
    {
        std::unordered_set<ObjectAddress> seen;
        seen.reserve(rootItems.size());
        for (auto const& item : rootItems)
        {
            seen.insert(SourceRecord::AddressOf(item));
        }

        for (auto const& item : rootItems)
        {
            ReadSubtree(item, m_staged, seen, nullptr);
        }
    }
    catch (...)
    {
        m_staged.clear();
        throw;
    }
}

void ChildCollectionsHierarchy::MatchForRefresh(
    ISourceHierarchyHost& host,
    std::vector<SourceRecord*> const& rootMatches,
    std::vector<SourceRecordPtr>& removed)
{
    // A staged child reuses a record of the same owner's feed: same position and same raw
    // pointer first, else the same object address. One forward pass: a node's Match is set
    // before its children are reached. No app calls.
    std::unordered_set<SourceRecord const*> kept;

    size_t position = 0;
    for (SourceRecord* const match : rootMatches)
    {
        MUX_ASSERT(position < m_staged.size());
        m_staged[position].Match = match;
        if (match)
        {
            kept.insert(match);
        }
        position += m_staged[position].SubtreeSize + 1;
    }
    MUX_ASSERT(position == m_staged.size());

    for (size_t i = 0; i < m_staged.size(); ++i)
    {
        SourceRecord const* const owner = m_staged[i].Match;
        if (!owner || m_staged[i].SubtreeSize == 0)
        {
            continue;
        }

        auto const found = m_childFeeds.find(owner);
        if (found == m_childFeeds.end())
        {
            continue;
        }

        auto const& records = found->second->Records;
        std::vector<bool> used(records.size(), false);
        std::unordered_map<ObjectAddress, size_t> byAddress;
        byAddress.reserve(records.size());
        for (size_t j = 0; j < records.size(); ++j)
        {
            byAddress.emplace(records[j]->Address, j);
        }

        size_t const end = i + 1 + m_staged[i].SubtreeSize;
        size_t childPosition = 0;
        for (size_t c = i + 1; c < end; c += m_staged[c].SubtreeSize + 1, ++childPosition)
        {
            auto const& item = m_staged[c].Item;
            SourceRecord* match = nullptr;
            if (childPosition < records.size() && !used[childPosition] &&
                winrt::get_abi(records[childPosition]->Item.get()) == winrt::get_abi(item))
            {
                match = records[childPosition].get();
                used[childPosition] = true;
            }
            else if (!records.empty())
            {
                auto const at = byAddress.find(SourceRecord::AddressOf(item));
                if (at != byAddress.end() && !used[at->second])
                {
                    match = records[at->second].get();
                    used[at->second] = true;
                }
            }

            m_staged[c].Match = match;
            if (match)
            {
                kept.insert(match);
            }
        }
    }

    // Child records that match nothing leave the lookups now and die after the Reset.
    for (auto& entry : m_childFeeds)
    {
        for (auto& record : entry.second->Records)
        {
            if (record && !kept.count(record.get()))
            {
                host.ReleaseRecord(record.get());
                removed.push_back(std::move(record));
            }
        }
    }

    // Feeds whose owner left (all their records were released above).
    for (auto it = m_childFeeds.begin(); it != m_childFeeds.end();)
    {
        if (kept.count(it->first))
        {
            ++it;
            continue;
        }

        ChildFeed& feed = *it->second;
        if (feed.Accessor)
        {
            feed.Accessor->Unsubscribe(feed.EventToken);
        }
        it = m_childFeeds.erase(it);
    }
}

void ChildCollectionsHierarchy::Commit(ISourceHierarchyHost& host, std::vector<SourceRecord*>& added)
{
    // Every feed is rebound to the collection Prepare read (C3), even when unchanged.
    size_t position = 0;
    for (auto const& root : host.RootRecords())
    {
        MUX_ASSERT(position < m_staged.size());
        BuildChildFeed(host, root, m_staged, position, added);
        position += m_staged[position].SubtreeSize + 1;
    }
    MUX_ASSERT(position == m_staged.size());

    m_staged.clear();
}

void ChildCollectionsHierarchy::BuildChildFeed(
    ISourceHierarchyHost& host,
    SourceRecordPtr const& record,
    std::vector<StagedNode> const& nodes,
    size_t first,
    std::vector<SourceRecord*>& added)
{
    // Records of rebound feeds, until their staged node claims them (Match).
    std::unordered_map<SourceRecord*, SourceRecordPtr> pool;

    // Binds owner's feed to node's collection (nodes[index]), or drops it when there is none.
    // Reserves room for every direct child, so linking a child cannot throw once it exists.
    auto const bind = [&](SourceRecordPtr const& owner, size_t index) -> ChildFeed*
    {
        StagedNode const& node = nodes[index];
        owner->Children.clear();
        if (!node.ChildCollection)
        {
            DestroyChildFeed(owner.get());
            return nullptr;
        }

        size_t childCount = 0;
        size_t const end = index + 1 + node.SubtreeSize;
        for (size_t c = index + 1; c < end; c += nodes[c].SubtreeSize + 1)
        {
            ++childCount;
        }

        auto& slot = m_childFeeds[owner.get()];
        if (!slot)
        {
            slot = std::make_unique<ChildFeed>();
        }
        else
        {
            if (slot->Accessor)
            {
                slot->Accessor->Unsubscribe(slot->EventToken);
            }

            for (auto& child : slot->Records)
            {
                if (child)
                {
                    SourceRecord* const raw = child.get();
                    pool.emplace(raw, std::move(child));
                }
            }
            slot->Records.clear();
        }

        ChildFeed& feed = *slot;
        feed.Owner = owner;
        feed.Accessor.reset();
        feed.Accessor.emplace(host.TrackerOwner(), node.ChildCollection);
        feed.Records.reserve(childCount);
        owner->Children.reserve(childCount);
        SubscribeToChildChanges(host, feed);
        return &feed;
    };

    // Explicit stack (H2): the feeds still receiving children, innermost last.
    struct Frame
    {
        SourceRecord* Owner;
        ChildFeed* Feed;
        size_t End;
    };
    std::vector<Frame> pending;

    size_t const end = first + 1 + nodes[first].SubtreeSize;
    added.reserve(added.size() + nodes[first].SubtreeSize);
    if (ChildFeed* const feed = bind(record, first))
    {
        pending.push_back(Frame{ record.get(), feed, end });
    }

    for (size_t i = first + 1; i < end; ++i)
    {
        while (i >= pending.back().End)
        {
            pending.pop_back();
        }

        Frame const parent = pending.back();
        StagedNode const& node = nodes[i];

        SourceRecordPtr child;
        if (node.Match)
        {
            auto const at = pool.find(node.Match);
            MUX_ASSERT(at != pool.end());
            if (at != pool.end())
            {
                child = std::move(at->second);
                pool.erase(at);
            }
        }

        if (!child)
        {
            child = host.CreateRecord(node.Item);
            added.push_back(child.get());
        }

        SourceRecord* const raw = child.get();
        raw->Parent = parent.Owner;
        raw->Depth = static_cast<uint16_t>(parent.Owner->Depth + 1);
        parent.Owner->Children.push_back(raw);
        parent.Feed->Records.push_back(std::move(child));

        if (ChildFeed* const feed = bind(parent.Feed->Records.back(), i))
        {
            pending.push_back(Frame{ raw, feed, i + 1 + node.SubtreeSize });
        }
    }

    // Every matched record was claimed by its node; anything left must not outlive its lookups.
    MUX_ASSERT(pool.empty());
    for (auto& entry : pool)
    {
        host.ReleaseRecord(entry.first);
    }
}

void ChildCollectionsHierarchy::DestroyChildFeed(SourceRecord const* owner) noexcept
{
    auto const found = m_childFeeds.find(owner);
    if (found == m_childFeeds.end())
    {
        return;
    }

    std::unique_ptr<ChildFeed> feed = std::move(found->second);
    m_childFeeds.erase(found);
    MUX_ASSERT(std::none_of(feed->Records.begin(), feed->Records.end(), [](SourceRecordPtr const& record) { return !!record; }));
    if (feed->Accessor)
    {
        feed->Accessor->Unsubscribe(feed->EventToken);
    }
}

// ---- Root-collection edits (A1) --------------------------------------------------------------

void ChildCollectionsHierarchy::CollectIncoming(
    winrt::Windows::Foundation::IInspectable const& item,
    std::vector<winrt::Windows::Foundation::IInspectable>& items)
{
    DiscardIncoming();
    try
    {
        m_incoming = ReadIncoming(item, m_incomingError);
        AppendStagedItems(m_incoming, 0, items);
    }
    catch (...)
    {
        DiscardIncoming();
        throw;
    }
}

void ChildCollectionsHierarchy::DiscardIncoming() noexcept
{
    m_incoming.clear();
    m_incomingError = nullptr;
}

void ChildCollectionsHierarchy::OnInserted(ISourceHierarchyHost& host, SourceRecord* record)
{
    auto const& roots = host.RootRecords();
    auto const root = std::find_if(roots.begin(), roots.end(), [record](SourceRecordPtr const& candidate) { return candidate.get() == record; });
    MUX_ASSERT(root != roots.end());
    auto const index = static_cast<uint32_t>(root - roots.begin());

    std::vector<StagedNode> nodes = std::move(m_incoming);
    std::exception_ptr const error = std::exchange(m_incomingError, nullptr);
    m_incoming.clear();

    try
    {
        // H5: the stage is the read CollectIncoming made for this item. Without it the record is
        // linked as a leaf and the next notification re-reads the tree.
        if (nodes.empty() || winrt::get_abi(nodes.front().Item) != winrt::get_abi(record->Item.get()))
        {
            MUX_ASSERT(false);
            nodes.clear();
            nodes.push_back(StagedNode{ record->Item.get() });
            host.RequestReset();
        }

        std::vector<SourceRecord*> added;
        std::vector<SourceRecordPtr> removed;
        added.reserve(1 + nodes.front().SubtreeSize);

        // Every root record is top level, in source order.
        host.Reparent(record, nullptr, index);
        added.push_back(record);
        BuildChildFeed(host, *root, nodes, 0, added);
        host.RaiseSourceChanged(SourceChangeKind::Add, nullptr, nullptr, 0, index, added, removed);
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }

    if (error)
    {
        std::rethrow_exception(error);
    }
}

void ChildCollectionsHierarchy::OnRemoving(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecordPtr>& removed)
{
    // record has left RootRecords() and the lookups; its subtree follows it into removed.
    auto& topLevel = host.SiblingsOf(nullptr);
    uint32_t const index = IndexInSourceSiblings(topLevel, record);
    MUX_ASSERT(index < topLevel.size());

    try
    {
        TakeFeed(host, record, removed);

        if (index < topLevel.size())
        {
            topLevel.erase(topLevel.begin() + index);
        }
        record->Parent = nullptr;

        host.RaiseSourceChanged(SourceChangeKind::Remove, nullptr, nullptr, index, 0, {}, removed);
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }
}

void ChildCollectionsHierarchy::OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t, uint32_t toIndex)
{
    // Every root record is top level, so its top-level index is its root index.
    host.Reparent(record, nullptr, toIndex);
}

bool ChildCollectionsHierarchy::Reevaluate(ISourceHierarchyHost&, SourceRecord*, winrt::hstring const&)
{
    // C4: a children-property replacement is picked up at Refresh.
    return false;
}

void ChildCollectionsHierarchy::ForEachChildRecord(std::function<void(SourceRecordPtr const&)> const& visit) const
{
    for (auto const& entry : m_childFeeds)
    {
        for (auto const& record : entry.second->Records)
        {
            if (record)
            {
                visit(record);
            }
        }
    }
}

void ChildCollectionsHierarchy::TakeChildRecords(ISourceHierarchyHost& host, std::vector<SourceRecordPtr>& removed)
{
    for (auto const& root : host.RootRecords())
    {
        TakeFeed(host, root.get(), removed);
    }
}

void ChildCollectionsHierarchy::TakeFeed(
    ISourceHierarchyHost& host,
    SourceRecord const* owner,
    std::vector<SourceRecordPtr>& removed)
{
    if (!m_childFeeds.count(owner))
    {
        return;
    }

    // Everything that can throw comes first: the subtree is counted and room is made for it, so
    // no record can be dropped between leaving its feed and reaching removed.
    std::vector<SourceRecord*> subtree;
    AppendSourceSubtree(const_cast<SourceRecord*>(owner), subtree);
    removed.reserve(removed.size() + subtree.size());
    std::vector<SourceRecordPtr> pending;
    pending.reserve(subtree.size());

    auto const takeFeedOf = [&](SourceRecord const* record)
    {
        auto const found = m_childFeeds.find(record);
        if (found == m_childFeeds.end())
        {
            return;
        }

        std::unique_ptr<ChildFeed> feed = std::move(found->second);
        m_childFeeds.erase(found);
        if (feed->Accessor)
        {
            feed->Accessor->Unsubscribe(feed->EventToken);
        }

        // Reversed, so the stack pops them in source order (pre-order overall).
        for (auto it = feed->Records.rbegin(); it != feed->Records.rend(); ++it)
        {
            if (*it)
            {
                pending.push_back(std::move(*it));
            }
        }
    };

    takeFeedOf(owner);
    while (!pending.empty())
    {
        SourceRecordPtr record = std::move(pending.back());
        pending.pop_back();

        SourceRecord* const raw = record.get();
        host.ReleaseRecord(raw);
        removed.push_back(std::move(record));
        takeFeedOf(raw);
    }
}

void ChildCollectionsHierarchy::Release(ISourceHierarchyHost&) noexcept
{
    // C1: no host.ReleaseRecord and no Item.get(); the lookups are already empty of these
    // records, or SourceTable::Clear() clears them wholesale.
    for (auto& entry : m_childFeeds)
    {
        ChildFeed& feed = *entry.second;
        if (feed.Accessor)
        {
            feed.Accessor->Unsubscribe(feed.EventToken);
            feed.Accessor.reset();
        }
    }

    m_childFeeds.clear();
    m_staged.clear();
    DiscardIncoming();
    m_childrenSelector.reset();
}

// ---- Child-collection changes (C2, C3) -------------------------------------------------------

void ChildCollectionsHierarchy::SubscribeToChildChanges(ISourceHierarchyHost& host, ChildFeed& feed)
{
    // The handlers hold only weak references. The thread is checked before the host is locked
    // (A2), and a change reaches this strategy only while it is still the host's current one (A4).
    auto const route = [weakHost = host.WeakHost(), binding = host.Binding(), generation = Generation(), owner = feed.Owner](auto const& apply)
    {
        if (binding)
        {
            ThrowIfOffBoundThread(*binding);
        }

        if (auto const locked = weakHost.lock())
        {
            SourceHierarchy* const current = locked->CurrentHierarchy();
            if (current && current->Generation() == generation)
            {
                apply(*locked, *static_cast<ChildCollectionsHierarchy*>(current), owner);
            }
        }
    };

    try
    {
        feed.EventToken = feed.Accessor->Subscribe(
            [route](auto const&, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
            {
                route([&](ISourceHierarchyHost& host, ChildCollectionsHierarchy& self, SourceRecordWeakPtr const& owner)
                {
                    self.OnChildCollectionChanged(host, owner, args);
                });
            },
            [route](auto const&, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
            {
                route([&](ISourceHierarchyHost& host, ChildCollectionsHierarchy& self, SourceRecordWeakPtr const& owner)
                {
                    self.OnChildVectorChanged(host, owner, args);
                });
            },
            [route](auto const&, auto const&)
            {
                route([](ISourceHierarchyHost& host, ChildCollectionsHierarchy& self, SourceRecordWeakPtr const& owner)
                {
                    self.OnChildBindableVectorChanged(host, owner);
                });
            });
    }
    catch (...)
    {
        // C3: a collection that refuses the handler stays unobserved.
        feed.EventToken = {};
    }
}

void ChildCollectionsHierarchy::OnChildCollectionChanged(
    ISourceHierarchyHost& host,
    SourceRecordWeakPtr const& weakOwner,
    winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    using Action = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

    auto const owner = weakOwner.lock();
    if (!owner)
    {
        return;
    }

    auto const found = m_childFeeds.find(owner.get());
    if (found == m_childFeeds.end())
    {
        return;
    }

    ChildFeed& feed = *found->second;
    auto const action = args.Action();
    auto const newItems = args.NewItems();
    auto const oldItems = args.OldItems();
    int32_t const newIndex = args.NewStartingIndex();
    int32_t const oldIndex = args.OldStartingIndex();
    auto const size = static_cast<int32_t>(feed.Records.size());

    auto const isSingle = [](auto const& items) { return items && items.Size() == 1; };

    // The single old item is the record at index.
    auto const isRecordAt = [&](int32_t index)
    {
        if (!isSingle(oldItems) || index < 0 || index >= size)
        {
            return false;
        }

        auto const oldItem = oldItems.GetAt(0);
        return oldItem && SourceRecord::AddressOf(oldItem) == feed.Records[index]->Address;
    };

    // C3: a single Add, a single matching Remove or a single matching Replace applies
    // positionally; everything else re-reads the whole tree.
    bool isReset = true;
    switch (action)
    {
    case Action::Add:
        isReset = !isSingle(newItems) || newIndex < 0 || newIndex > size;
        break;
    case Action::Remove:
        isReset = !isRecordAt(oldIndex);
        break;
    case Action::Replace:
        isReset = !isSingle(newItems) || !isRecordAt(oldIndex) || (newIndex >= 0 && newIndex != oldIndex);
        break;
    default:
        break;
    }

    // C2: false means the host re-read or skipped the change; nothing here may be touched.
    if (!host.AdmitChange(isReset))
    {
        return;
    }
    MUX_ASSERT(!isReset);

    std::exception_ptr error;
    switch (action)
    {
    case Action::Add:
        error = InsertChild(host, owner, feed, static_cast<uint32_t>(newIndex), newItems.GetAt(0));
        break;
    case Action::Remove:
        RemoveChild(host, owner.get(), feed, static_cast<uint32_t>(oldIndex));
        break;
    case Action::Replace:
        error = ReplaceChild(host, owner, feed, static_cast<uint32_t>(oldIndex), newItems.GetAt(0));
        break;
    default:
        break;
    }

    host.DrainPendingChanges();
    if (error)
    {
        std::rethrow_exception(error);
    }
}

void ChildCollectionsHierarchy::OnChildVectorChanged(
    ISourceHierarchyHost& host,
    SourceRecordWeakPtr const& weakOwner,
    winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
{
    using Change = winrt::Windows::Foundation::Collections::CollectionChange;

    auto const owner = weakOwner.lock();
    if (!owner)
    {
        return;
    }

    auto const found = m_childFeeds.find(owner.get());
    if (found == m_childFeeds.end())
    {
        return;
    }

    ChildFeed& feed = *found->second;
    auto const change = args.CollectionChange();
    auto const size = static_cast<uint32_t>(feed.Records.size());

    // Index is not read for a Reset: some implementations throw from it.
    uint32_t index = 0;
    winrt::Windows::Foundation::IInspectable item{ nullptr };
    bool isReset = true;
    switch (change)
    {
    case Change::ItemInserted:
        index = args.Index();
        isReset = index > size || !feed.Accessor->TryGetAt(index, item);
        break;
    case Change::ItemRemoved:
        index = args.Index();
        isReset = index >= size;
        break;
    case Change::ItemChanged:
        index = args.Index();
        isReset = index >= size || !feed.Accessor->TryGetAt(index, item);
        break;
    default:
        break;
    }

    if (!host.AdmitChange(isReset))
    {
        return;
    }
    MUX_ASSERT(!isReset);

    std::exception_ptr error;
    switch (change)
    {
    case Change::ItemInserted:
        error = InsertChild(host, owner, feed, index, item);
        break;
    case Change::ItemRemoved:
        RemoveChild(host, owner.get(), feed, index);
        break;
    case Change::ItemChanged:
        error = ReplaceChild(host, owner, feed, index, item);
        break;
    default:
        break;
    }

    host.DrainPendingChanges();
    if (error)
    {
        std::rethrow_exception(error);
    }
}

void ChildCollectionsHierarchy::OnChildBindableVectorChanged(ISourceHierarchyHost& host, SourceRecordWeakPtr const&)
{
    // No detail: a whole-tree re-read, or a skip while held or frozen.
    (void)host.AdmitChange(true);
}

std::exception_ptr ChildCollectionsHierarchy::InsertChild(
    ISourceHierarchyHost& host,
    SourceRecordPtr const& owner,
    ChildFeed& feed,
    uint32_t index,
    winrt::Windows::Foundation::IInspectable const& item)
{
    // Compute before commit (H1): the item and its whole subtree are read once and checked.
    std::exception_ptr selectorError;
    std::vector<StagedNode> nodes;
    try
    {
        nodes = ReadIncoming(item, selectorError);
        std::vector<winrt::Windows::Foundation::IInspectable> items{ item };
        AppendStagedItems(nodes, 0, items);
        host.ValidateIncoming(items, nullptr);
    }
    catch (...)
    {
        // The collection already holds the item: the feed differs until a re-read succeeds.
        host.RequestReset();
        throw;
    }

    InsertStaged(host, owner, feed, index, nodes);

    // The caller drains the owed changes first, then rethrows.
    return selectorError;
}

std::exception_ptr ChildCollectionsHierarchy::ReplaceChild(
    ISourceHierarchyHost& host,
    SourceRecordPtr const& owner,
    ChildFeed& feed,
    uint32_t index,
    winrt::Windows::Foundation::IInspectable const& item)
{
    // C3: validated against the leaving record and its subtree before anything is written.
    SourceRecord const* const leaving = feed.Records[index].get();
    std::exception_ptr selectorError;
    std::vector<StagedNode> nodes;
    try
    {
        nodes = ReadIncoming(item, selectorError);
        std::vector<winrt::Windows::Foundation::IInspectable> items{ item };
        AppendStagedItems(nodes, 0, items);
        host.ValidateIncoming(items, leaving);
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }

    // The view sees Remove + Add; the new row is collapsed.
    RemoveChild(host, owner.get(), feed, index);
    InsertStaged(host, owner, feed, index, nodes);
    return selectorError;
}

void ChildCollectionsHierarchy::InsertStaged(
    ISourceHierarchyHost& host,
    SourceRecordPtr const& owner,
    ChildFeed& feed,
    uint32_t index,
    std::vector<StagedNode> const& nodes)
{
    // Room first, so the new record is owned by the feed as soon as it exists.
    ReserveForOneMore(feed.Records);
    auto record = host.CreateRecord(nodes.front().Item);
    SourceRecord* const raw = record.get();
    feed.Records.insert(feed.Records.begin() + index, std::move(record));

    try
    {
        std::vector<SourceRecord*> added;
        std::vector<SourceRecordPtr> removed;
        added.reserve(1 + nodes.front().SubtreeSize);

        host.Reparent(raw, owner.get(), index);
        added.push_back(raw);
        BuildChildFeed(host, feed.Records[index], nodes, 0, added);
        host.RaiseSourceChanged(SourceChangeKind::Add, nullptr, owner.get(), 0, index, added, removed);
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }
}

void ChildCollectionsHierarchy::RemoveChild(ISourceHierarchyHost& host, SourceRecord* owner, ChildFeed& feed, uint32_t index)
{
    std::vector<SourceRecordPtr> removed;
    removed.reserve(1);

    SourceRecordPtr record = std::move(feed.Records[index]);
    feed.Records.erase(feed.Records.begin() + index);

    SourceRecord* const raw = record.get();
    auto& siblings = host.SiblingsOf(owner);
    siblings.erase(std::remove(siblings.begin(), siblings.end(), raw), siblings.end());
    raw->Parent = nullptr;

    host.ReleaseRecord(raw);
    removed.push_back(std::move(record));

    try
    {
        TakeFeed(host, raw, removed);

        // removed keeps the subtree alive until the handler returns.
        host.RaiseSourceChanged(SourceChangeKind::Remove, owner, nullptr, index, 0, {}, removed);
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }
}
