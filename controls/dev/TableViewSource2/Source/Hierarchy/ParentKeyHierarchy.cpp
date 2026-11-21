// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Source/Hierarchy/ParentKeyHierarchy.h"

#include <algorithm>
#include <unordered_set>

#include "ParentKeyIndex.h"

namespace
{
    // Lookup form of a selector result (v1 MakeNodeKey): boxed value types by value (int 1 ==
    // int? 1), objects by reference. Empty for null.
    winrt::hstring NodeKeyOf(winrt::Windows::Foundation::IInspectable const& key)
    {
        return winrt::hstring{ ShapingHelpers::MakeNodeKey(key) };
    }

    // The record keyed by record's parent key, or nullptr: top level (no parent key, an orphan,
    // or record's own key).
    template <typename NodeKeyMap>
    SourceRecord* FindParent(
        std::unordered_map<winrt::hstring, SourceRecord*> const& keyIndex,
        NodeKeyMap const& nodeKeys,
        SourceRecord const* record)
    {
        auto const keys = nodeKeys.find(record);
        if (keys == nodeKeys.end() || keys->second.ParentKey.empty())
        {
            return nullptr;
        }

        auto const parent = keyIndex.find(keys->second.ParentKey);
        return parent == keyIndex.end() || parent->second == record ? nullptr : parent->second;
    }

    // True when ancestor is candidate or one of candidate's ancestors (the linked tree has no cycle).
    bool IsSelfOrAncestor(SourceRecord const* ancestor, SourceRecord const* candidate) noexcept
    {
        for (SourceRecord const* current = candidate; current; current = current->Parent)
        {
            if (current == ancestor)
            {
                return true;
            }
        }

        return false;
    }

    // record's own SourceRecordPtr, so a Remove's removed list keeps it alive for the handler.
    SourceRecordPtr RootPtr(ISourceHierarchyHost& host, SourceRecord const* record)
    {
        auto const& roots = host.RootRecords();
        auto const it = std::find_if(roots.begin(), roots.end(), [record](SourceRecordPtr const& root) { return root.get() == record; });
        return it == roots.end() ? nullptr : *it;
    }
}

ParentKeyHierarchy::ParentKeyHierarchy(
    ITrackerHandleManager const* owner,
    winrt::TableViewKeySelector const& keySelector,
    winrt::TableViewKeySelector const& parentKeySelector)
{
    m_keySelector.emplace(owner, keySelector);
    m_parentKeySelector.emplace(owner, parentKeySelector);
}

void ParentKeyHierarchy::Prepare(
    ISourceHierarchyHost& host,
    std::vector<winrt::Windows::Foundation::IInspectable> const& rootItems)
{
    // Built in locals and assigned at the end, so a throw leaves the strategy as it was.
    bool const strict = !m_committed;
    auto const& keySelector = m_keySelector->get();
    auto const& parentKeySelector = m_parentKeySelector->get();

    std::unordered_map<winrt::hstring, uint32_t> keyIndex;
    std::vector<NodeKeys> nodeKeys;
    keyIndex.reserve(rootItems.size());
    nodeKeys.reserve(rootItems.size());
    for (uint32_t i = 0; i < rootItems.size(); ++i)
    {
        auto const key = keySelector(rootItems[i]);
        auto const parentKey = parentKeySelector(rootItems[i]);

        NodeKeys& staged = nodeKeys.emplace_back();
        staged.ParentKey = NodeKeyOf(parentKey);
        winrt::hstring const nodeKey = NodeKeyOf(key);
        if (nodeKey.empty())
        {
            if (strict)
            {
                throw winrt::hresult_invalid_argument(
                    L"TableViewSource2.ParentBy: the key selector returned null or an empty key for an item.");
            }
            continue;  // excluded: Key stays empty
        }

        if (strict && nodeKey == staged.ParentKey)
        {
            throw winrt::hresult_invalid_argument(L"TableViewSource2.ParentBy: an item is its own parent.");
        }

        auto const [slot, inserted] = keyIndex.emplace(nodeKey, i);
        if (!inserted)
        {
            if (strict)
            {
                throw winrt::hresult_invalid_argument(
                    L"TableViewSource2.ParentBy: two items have the same key. Keys must be unique.");
            }

            // The record holding the key keeps it; otherwise the first in source order does.
            auto const holder = m_keyIndex.find(nodeKey);
            bool const isHolder = holder != m_keyIndex.end() &&
                holder->second->Address == SourceRecord::AddressOf(rootItems[i]);
            if (!isHolder)
            {
                continue;  // excluded
            }

            NodeKeys& displaced = nodeKeys[slot->second];
            displaced.Key = {};
            displaced.KeyObject.reset();
            slot->second = i;
        }

        staged.Key = nodeKey;
        if (ShapingHelpers::IsObjectNodeKey(nodeKey))
        {
            staged.KeyObject.emplace(host.TrackerOwner(), key);
        }
    }

    if (strict)
    {
        ValidateNoCycles(keyIndex, nodeKeys);
    }

    m_stagedKeyIndex = std::move(keyIndex);
    m_stagedNodeKeys = std::move(nodeKeys);
}

void ParentKeyHierarchy::ValidateNoCycles(
    std::unordered_map<winrt::hstring, uint32_t> const& keyIndex,
    std::vector<NodeKeys> const& nodeKeys)
{
    // Every position reachable from the top level; an unreached one is on or below a cycle. A
    // self-parent was rejected before, so a parent key naming the item's own key cannot reach here.
    size_t const count = nodeKeys.size();
    std::vector<std::vector<uint32_t>> children(count);
    std::vector<uint32_t> pending;
    for (uint32_t i = 0; i < count; ++i)
    {
        auto const& parentKey = nodeKeys[i].ParentKey;
        auto const parent = parentKey.empty() ? keyIndex.end() : keyIndex.find(parentKey);
        if (parent == keyIndex.end())
        {
            pending.push_back(i);
        }
        else
        {
            children[parent->second].push_back(i);
        }
    }

    size_t reached = 0;
    while (!pending.empty())
    {
        uint32_t const i = pending.back();
        pending.pop_back();
        ++reached;
        pending.insert(pending.end(), children[i].begin(), children[i].end());
    }

    if (reached != count)
    {
        throw winrt::hresult_invalid_argument(L"TableViewSource2.ParentBy: the parent keys form a cycle.");
    }
}

void ParentKeyHierarchy::MatchForRefresh(
    ISourceHierarchyHost&,
    std::vector<SourceRecord*> const& rootMatches,
    std::vector<SourceRecordPtr>&)
{
    // Every record is a root record: SourceTable::MatchRecords matched them all. Only kept records
    // stay preferred cycle breaks; a record that left may be freed and its address reused.
    std::unordered_set<SourceRecord*> kept;
    for (SourceRecord* const record : rootMatches)
    {
        if (record && m_forcedTop.count(record))
        {
            kept.insert(record);
        }
    }

    m_forcedTop = std::move(kept);
}

void ParentKeyHierarchy::Commit(ISourceHierarchyHost& host, std::vector<SourceRecord*>&)
{
    auto const& roots = host.RootRecords();
    size_t const count = roots.size();
    MUX_ASSERT(count == m_stagedNodeKeys.size());

    std::unordered_set<SourceRecord*> const preferred = std::move(m_forcedTop);
    m_forcedTop.clear();
    m_keyIndex.clear();
    m_nodeKeys.clear();
    m_excluded.clear();

    std::unordered_map<SourceRecord const*, size_t> position;
    position.reserve(count);
    for (size_t i = 0; i < count; ++i)
    {
        SourceRecord* const record = roots[i].get();
        position.emplace(record, i);
        if (m_stagedNodeKeys[i].Key.empty())
        {
            m_excluded.insert(record);
            continue;
        }

        m_keyIndex.emplace(m_stagedNodeKeys[i].Key, record);
        m_nodeKeys.emplace(record, std::move(m_stagedNodeKeys[i]));
    }

    m_stagedKeyIndex.clear();
    m_stagedNodeKeys.clear();

    std::vector<SourceRecord*> parentOf(count, nullptr);
    for (size_t i = 0; i < count; ++i)
    {
        parentOf[i] = FindParent(m_keyIndex, m_nodeKeys, roots[i].get());
    }

    // Cycles (a tolerant re-read only; the strict paths rejected them). Follow each record's parent
    // chain once: 0 = unvisited, 1 = on the current chain, 2 = reaches the top level. Reaching a
    // record on the current chain closes a cycle; it is broken at a member forced before (stable
    // across re-reads), else at the earliest in root order.
    std::unordered_set<SourceRecord*> forced;
    std::vector<uint8_t> state(count, 0);
    std::vector<size_t> chain;
    for (size_t i = 0; i < count; ++i)
    {
        chain.clear();
        for (size_t at = i;;)
        {
            if (state[at] == 2)
            {
                break;
            }

            if (state[at] == 1)
            {
                auto const first = std::find(chain.begin(), chain.end(), at);
                size_t breakAt = *first;
                bool breakAtPreferred = preferred.count(roots[breakAt].get()) != 0;
                for (auto member = first + 1; member != chain.end(); ++member)
                {
                    bool const memberPreferred = preferred.count(roots[*member].get()) != 0;
                    if ((memberPreferred && !breakAtPreferred) ||
                        (memberPreferred == breakAtPreferred && *member < breakAt))
                    {
                        breakAt = *member;
                        breakAtPreferred = memberPreferred;
                    }
                }

                parentOf[breakAt] = nullptr;
                forced.insert(roots[breakAt].get());
                break;
            }

            state[at] = 1;
            chain.push_back(at);
            if (!parentOf[at])
            {
                break;
            }
            at = position.at(parentOf[at]);
        }

        for (size_t const member : chain)
        {
            state[member] = 2;
        }
    }

    // Link in root order, so every sibling list follows source order.
    host.SiblingsOf(nullptr).clear();
    for (auto const& root : roots)
    {
        root->Children.clear();
    }

    for (size_t i = 0; i < count; ++i)
    {
        SourceRecord* const record = roots[i].get();
        record->Parent = nullptr;
        record->Depth = 0;
        if (!m_excluded.count(record))
        {
            record->Parent = parentOf[i];
            host.SiblingsOf(parentOf[i]).push_back(record);
        }
    }

    std::vector<SourceRecord*> pending(host.SiblingsOf(nullptr).begin(), host.SiblingsOf(nullptr).end());
    while (!pending.empty())
    {
        SourceRecord* const record = pending.back();
        pending.pop_back();
        for (SourceRecord* const child : record->Children)
        {
            child->Depth = static_cast<uint16_t>(record->Depth + 1);
            pending.push_back(child);
        }
    }

    m_forcedTop = std::move(forced);
    m_committed = true;
}

void ParentKeyHierarchy::CollectIncoming(
    winrt::Windows::Foundation::IInspectable const&,
    std::vector<winrt::Windows::Foundation::IInspectable>&)
{
    // An incoming root item brings nothing else.
}

bool ParentKeyHierarchy::Keys(
    ISourceHierarchyHost& host,
    SourceRecord const* record,
    NodeKeys& keys,
    std::exception_ptr& error) const
{
    auto const keep = [&error](std::exception_ptr failure) {
        if (!error)
        {
            error = std::move(failure);
        }
    };

    auto const item = record->Item.get();
    winrt::Windows::Foundation::IInspectable key{ nullptr };
    try
    {
        key = m_keySelector->get()(item);
        keys.Key = NodeKeyOf(key);
    }
    catch (...)
    {
        keep(std::current_exception());  // no key: excluded (B3)
        return false;
    }

    try
    {
        keys.ParentKey = NodeKeyOf(m_parentKeySelector->get()(item));
    }
    catch (...)
    {
        keep(std::current_exception());  // top level, like an orphan (B3)
        keys.ParentKey = {};
    }

    if (keys.Key.empty())
    {
        keep(std::make_exception_ptr(winrt::hresult_invalid_argument(
            L"TableViewSource2.ParentBy: the key selector returned null or an empty key for an item.")));
        return false;
    }

    auto const holder = m_keyIndex.find(keys.Key);
    if (holder != m_keyIndex.end() && holder->second != record)
    {
        keep(std::make_exception_ptr(winrt::hresult_invalid_argument(
            L"TableViewSource2.ParentBy: two items have the same key. Keys must be unique.")));
        return false;
    }

    if (keys.Key == keys.ParentKey)
    {
        // FindParent puts it at the top level.
        keep(std::make_exception_ptr(winrt::hresult_invalid_argument(L"TableViewSource2.ParentBy: an item is its own parent.")));
    }

    if (ShapingHelpers::IsObjectNodeKey(keys.Key))
    {
        keys.KeyObject.emplace(host.TrackerOwner(), key);
    }

    return true;
}

SourceRecord* ParentKeyHierarchy::PlaceParent(
    SourceRecord* record,
    std::vector<SourceRecord*> const& orphans,
    std::exception_ptr& error)
{
    SourceRecord* const parent = FindParent(m_keyIndex, m_nodeKeys, record);
    bool closesCycle = false;
    for (SourceRecord const* above = parent; above && !closesCycle; above = above->Parent)
    {
        closesCycle = above == record || std::find(orphans.begin(), orphans.end(), above) != orphans.end();
    }

    if (!closesCycle)
    {
        m_forcedTop.erase(record);
        return parent;
    }

    m_forcedTop.insert(record);
    if (!error)
    {
        error = std::make_exception_ptr(winrt::hresult_invalid_argument(L"TableViewSource2.ParentBy: the parent keys form a cycle."));
    }
    return nullptr;
}

std::vector<SourceRecord*> ParentKeyHierarchy::WaitingFor(
    ISourceHierarchyHost& host,
    winrt::hstring const& key,
    SourceRecord const* record) const
{
    std::vector<SourceRecord*> orphans;
    for (SourceRecord* const top : host.SiblingsOf(nullptr))
    {
        if (top == record || m_forcedTop.count(top))
        {
            continue;
        }

        auto const keys = m_nodeKeys.find(top);
        if (keys != m_nodeKeys.end() && keys->second.ParentKey == key)
        {
            orphans.push_back(top);
        }
    }

    return orphans;
}

uint32_t ParentKeyHierarchy::SourceOrderPosition(
    ISourceHierarchyHost& host,
    SourceRecord const* record,
    SourceRecord* parent) const
{
    uint32_t position = 0;
    for (auto const& root : host.RootRecords())
    {
        if (root.get() == record)
        {
            break;
        }

        if (root->Parent == parent && !m_excluded.count(root.get()))
        {
            ++position;
        }
    }

    if (m_leaving && m_leaving->Parent == parent)
    {
        auto const& siblings = host.SiblingsOf(parent);
        uint32_t const leavingIndex = IndexInSourceSiblings(siblings, m_leaving);
        if (leavingIndex < siblings.size() && leavingIndex <= position)
        {
            ++position;
        }
    }

    return position;
}

void ParentKeyHierarchy::LinkAndAdopt(ISourceHierarchyHost& host, SourceRecord* record, std::exception_ptr& error)
{
    try
    {
        auto const orphans = WaitingFor(host, m_nodeKeys.at(record).Key, record);
        SourceRecord* const parent = PlaceParent(record, orphans, error);
        uint32_t const index = SourceOrderPosition(host, record, parent);
        host.Reparent(record, parent, index);

        std::vector<SourceRecordPtr> removed;
        if (orphans.size() > 1)
        {
            // Spec Bulk threshold: one Reset, with no Add first.
            for (SourceRecord* const orphan : orphans)
            {
                host.Reparent(orphan, record, SourceOrderPosition(host, orphan, record));
            }
            host.RaiseSourceChanged(SourceChangeKind::Reset, nullptr, nullptr, 0, 0, { record }, removed);
        }
        else
        {
            host.RaiseSourceChanged(SourceChangeKind::Add, nullptr, parent, 0, index, { record }, removed);
            if (!orphans.empty())
            {
                MoveTo(host, orphans.front(), record);
            }
        }

        RestoreForcedTop(host);
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }
}

void ParentKeyHierarchy::Adopt(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecord*> const& orphans)
{
    // Spec Orphan adoption and Bulk threshold: one orphan is a Move; more than one is one Reset.
    try
    {
        if (orphans.size() == 1)
        {
            MoveTo(host, orphans.front(), record);
        }
        else if (orphans.size() > 1)
        {
            for (SourceRecord* const orphan : orphans)
            {
                host.Reparent(orphan, record, SourceOrderPosition(host, orphan, record));
            }

            std::vector<SourceRecordPtr> removed;
            host.RaiseSourceChanged(SourceChangeKind::Reset, nullptr, nullptr, 0, 0, {}, removed);
        }
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }
}

void ParentKeyHierarchy::MoveTo(ISourceHierarchyHost& host, SourceRecord* record, SourceRecord* newParent)
{
    SourceRecord* const oldParent = record->Parent;
    uint32_t const oldIndex = IndexInSourceSiblings(host.SiblingsOf(oldParent), record);
    uint32_t const newIndex = SourceOrderPosition(host, record, newParent);
    if (oldParent == newParent && oldIndex == newIndex)
    {
        return;
    }

    host.Reparent(record, newParent, newIndex);
    std::vector<SourceRecordPtr> removed;
    host.RaiseSourceChanged(SourceChangeKind::Move, oldParent, newParent, oldIndex, newIndex, {}, removed);
}

void ParentKeyHierarchy::Exclude(ISourceHierarchyHost& host, SourceRecord* record)
{
    // Behaves as filtered out (spec Misbehaving records): it leaves the tree, not the table.
    MUX_ASSERT(record->Children.empty());
    SourceRecord* const oldParent = record->Parent;
    auto& siblings = host.SiblingsOf(oldParent);
    uint32_t const oldIndex = IndexInSourceSiblings(siblings, record);
    MUX_ASSERT(oldIndex < siblings.size());
    siblings.erase(siblings.begin() + oldIndex);
    record->Parent = nullptr;
    record->Depth = 0;
    m_forcedTop.erase(record);
    m_excluded.insert(record);

    // A copy of its pointer: the record stays in the table, so the handler sees it alive and
    // nothing is destroyed afterwards.
    std::vector<SourceRecordPtr> removed{ RootPtr(host, record) };
    host.RaiseSourceChanged(SourceChangeKind::Remove, oldParent, nullptr, oldIndex, 0, {}, removed);
}

void ParentKeyHierarchy::OrphanChildren(ISourceHierarchyHost& host, SourceRecord* record)
{
    // Its children followed its key; without it they are orphans (spec: top level), one Move each.
    std::vector<SourceRecord*> const children = record->Children;
    try
    {
        for (SourceRecord* const child : children)
        {
            MoveTo(host, child, nullptr);
        }
    }
    catch (...)
    {
        host.RequestReset();  // the remaining children were not raised: re-read the whole tree
        throw;
    }
}

void ParentKeyHierarchy::RestoreForcedTop(ISourceHierarchyHost& host)
{
    // Spec: back to its parent once that no longer closes a cycle. Each move can free another;
    // scanning the top level in source order keeps the Move order deterministic.
    try
    {
        for (bool moved = true; moved && !m_forcedTop.empty();)
        {
            moved = false;
            for (SourceRecord* const record : host.SiblingsOf(nullptr))
            {
                if (!m_forcedTop.count(record))
                {
                    continue;
                }

                SourceRecord* const parent = FindParent(m_keyIndex, m_nodeKeys, record);
                if (parent && IsSelfOrAncestor(record, parent))
                {
                    continue;
                }

                m_forcedTop.erase(record);
                MoveTo(host, record, parent);  // changes the top level: rescan
                moved = true;
                break;
            }
        }
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }
}

void ParentKeyHierarchy::Unkey(SourceRecord const* record)
{
    auto const keys = m_nodeKeys.find(record);
    if (keys == m_nodeKeys.end())
    {
        return;
    }

    auto const holder = m_keyIndex.find(keys->second.Key);
    if (holder != m_keyIndex.end() && holder->second == record)
    {
        m_keyIndex.erase(holder);
    }
    m_nodeKeys.erase(keys);
}

void ParentKeyHierarchy::OnInserted(ISourceHierarchyHost& host, SourceRecord* record)
{
    // The item is already in the source: a bad key never undoes the insert (spec Misbehaving
    // records). An excluded record raises nothing: like a filtered-out record, it is not in the tree.
    std::exception_ptr error;
    NodeKeys keys;
    if (!Keys(host, record, keys, error))
    {
        MUX_ASSERT(error);
        m_excluded.insert(record);
        std::rethrow_exception(error);
    }

    winrt::hstring const key = keys.Key;
    m_keyIndex.emplace(key, record);
    m_nodeKeys[record] = std::move(keys);
    LinkAndAdopt(host, record, error);
    if (error)
    {
        std::rethrow_exception(error);
    }
}

void ParentKeyHierarchy::OnRemoving(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecordPtr>& removed)
{
    if (m_excluded.erase(record))
    {
        return;  // in no sibling list: its Remove was raised when it was excluded
    }

    m_forcedTop.erase(record);
    Unkey(record);

    // Children move out while record is still in the shaped tree, so a consumer never removes them
    // with it.
    m_leaving = record;
    try
    {
        OrphanChildren(host, record);
    }
    catch (...)
    {
        m_leaving = nullptr;
        throw;
    }
    m_leaving = nullptr;

    SourceRecord* const oldParent = record->Parent;
    auto& siblings = host.SiblingsOf(oldParent);
    uint32_t const oldIndex = IndexInSourceSiblings(siblings, record);
    MUX_ASSERT(oldIndex < siblings.size());
    siblings.erase(siblings.begin() + oldIndex);
    record->Parent = nullptr;

    try
    {
        host.RaiseSourceChanged(SourceChangeKind::Remove, oldParent, nullptr, oldIndex, 0, {}, removed);
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }

    RestoreForcedTop(host);  // a cycle through record is gone
}

void ParentKeyHierarchy::OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t, uint32_t)
{
    if (!m_excluded.count(record))
    {
        host.Reparent(record, record->Parent, SourceOrderPosition(host, record, record->Parent));
    }
}

void ParentKeyHierarchy::ForEachChildRecord(std::function<void(SourceRecordPtr const&)> const&) const
{
    // No child records: every record is a root record.
}

bool ParentKeyHierarchy::Reevaluate(ISourceHierarchyHost& host, SourceRecord* record, winrt::hstring const&)
{
    // Any property can feed a selector (B5): re-key on every PropertyChanged. Each tree change
    // raises its own SourceChange; the first failure propagates into the app's setter afterwards.
    std::exception_ptr error;
    NodeKeys keys;
    bool const keyed = Keys(host, record, keys, error);
    bool const wasExcluded = m_excluded.count(record) != 0;
    auto const current = m_nodeKeys.find(record);
    bool const unchanged = wasExcluded
        ? !keyed
        : keyed && current != m_nodeKeys.end() && current->second.Key == keys.Key && current->second.ParentKey == keys.ParentKey;
    if (unchanged)
    {
        return false;  // decided (B5): no change, nothing raised and no rethrow
    }

    bool const keyChanged = wasExcluded || !keyed || current == m_nodeKeys.end() || current->second.Key != keys.Key;
    if (!wasExcluded)
    {
        if (keyChanged)
        {
            OrphanChildren(host, record);
        }
        Unkey(record);
    }

    if (!keyed)
    {
        Exclude(host, record);
        RestoreForcedTop(host);
    }
    else
    {
        winrt::hstring const key = keys.Key;
        m_keyIndex[key] = record;
        m_nodeKeys[record] = std::move(keys);
        if (wasExcluded)
        {
            m_excluded.erase(record);
            LinkAndAdopt(host, record, error);
        }
        else
        {
            std::vector<SourceRecord*> orphans;
            if (keyChanged)
            {
                orphans = WaitingFor(host, key, record);
            }

            // A parent at or below record (or below an orphan it adopts) closes a cycle: record
            // goes to the top level with its subtree (spec).
            MoveTo(host, record, PlaceParent(record, orphans, error));
            Adopt(host, record, orphans);
            RestoreForcedTop(host);
        }
    }

    if (error)
    {
        std::rethrow_exception(error);
    }
    return true;
}

void ParentKeyHierarchy::TakeChildRecords(ISourceHierarchyHost&, std::vector<SourceRecordPtr>&)
{
    // No child records: every record is a root record.
}

void ParentKeyHierarchy::Release(ISourceHierarchyHost&) noexcept
{
    // Destroys the KeyObject and selector tracker_refs while the tracker owner is alive.
    m_keyIndex.clear();
    m_nodeKeys.clear();
    m_stagedKeyIndex.clear();
    m_stagedNodeKeys.clear();
    m_excluded.clear();
    m_forcedTop.clear();
    m_leaving = nullptr;
    m_committed = false;
    m_keySelector.reset();
    m_parentKeySelector.reset();
}
