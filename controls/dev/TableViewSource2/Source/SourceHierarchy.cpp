// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Source/SourceHierarchy.h"

#include <algorithm>
#include <unordered_set>

#include "ParentKeyIndex.h"

namespace
{
    // v1 wording (Hierarchical-Data-Rows-Parent-Key-Design.md section 3), without the key text.
    constexpr wchar_t c_emptyKeyMessage[] =
        L"TableViewSource2.ParentBy: the key selector returned null or an empty key for an item.";
    constexpr wchar_t c_duplicateKeyMessage[] =
        L"TableViewSource2.ParentBy: two items have the same key. Keys must be unique.";
    constexpr wchar_t c_selfParentMessage[] =
        L"TableViewSource2.ParentBy: an item is its own parent.";
    constexpr wchar_t c_cycleMessage[] =
        L"TableViewSource2.ParentBy: the parent keys form a cycle.";

    struct ComputedKeys
    {
        winrt::hstring Key;
        winrt::hstring ParentKey;
        winrt::Windows::Foundation::IInspectable KeyObject{ nullptr };  // object keys only
    };

    using MakeKeyFn = winrt::hstring (*)(winrt::Windows::Foundation::IInspectable const&);

    // Both selectors for one item, unchecked (Prepare checks). A selector throw propagates.
    ComputedKeys ComputeKeys(
        MakeKeyFn makeKey,
        winrt::TableViewKeySelector const& keySelector,
        winrt::TableViewKeySelector const& parentKeySelector,
        winrt::Windows::Foundation::IInspectable const& item)
    {
        auto const key = keySelector(item);
        auto const parentKey = parentKeySelector(item);

        ComputedKeys result;
        result.Key = makeKey(key);
        result.ParentKey = makeKey(parentKey);
        if (!result.Key.empty() && ShapingHelpers::IsObjectNodeKey(result.Key))
        {
            result.KeyObject = key;
        }

        return result;
    }

    // The record keyed by record's parent key, or nullptr (top level: no parent key, or an orphan).
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

    // Number of root records before record (in root order) whose parent is parent: record's
    // sibling index under parent when siblings follow source order (ruling 7). Excluded records
    // (ParentBy, ruling 5) have no parent but are in no sibling list, so they are skipped.
    uint32_t SourceOrderPosition(
        ISourceHierarchyHost& host,
        SourceRecord const* record,
        SourceRecord const* parent,
        std::unordered_set<SourceRecord const*> const* excluded)
    {
        uint32_t position = 0;
        for (auto const& root : host.RootRecords())
        {
            if (root.get() == record)
            {
                break;
            }

            if (root->Parent == parent && !(excluded && excluded->count(root.get())))
            {
                ++position;
            }
        }

        return position;
    }

    void ReparentInSourceOrder(
        ISourceHierarchyHost& host,
        SourceRecord* record,
        std::unordered_set<SourceRecord const*> const* excluded)
    {
        host.Reparent(record, record->Parent, SourceOrderPosition(host, record, record->Parent, excluded));
    }

    // True when ancestor is candidate or one of its ancestors (the linked tree has no cycle).
    bool IsSelfOrAncestor(SourceRecord const* ancestor, SourceRecord const* candidate)
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

    // record's own SourceRecordPtr (a Remove's removed list keeps it alive for the handler).
    SourceRecordPtr RootPtr(ISourceHierarchyHost& host, SourceRecord const* record)
    {
        auto const& roots = host.RootRecords();
        auto const it = std::find_if(roots.begin(), roots.end(), [record](SourceRecordPtr const& root) { return root.get() == record; });
        return it == roots.end() ? nullptr : *it;
    }

    // Commit's relink (verb and Refresh()): every root record not in excluded goes under
    // parentOf(record), siblings in root order, and Depth is recomputed; excluded records are in
    // no sibling list. A record left unreachable from the top level is on or below a cycle (only
    // a tolerant re-read lets one through, drift #9): one cycle member is forced to the top level
    // and the pass repeats. That member is one already in preferred (forced before) when the
    // cycle has one, else the member earliest in root order. Returns the forced records.
    std::unordered_set<SourceRecord*> RelinkAll(
        ISourceHierarchyHost& host,
        std::function<SourceRecord*(SourceRecord const*)> const& parentOf,
        std::unordered_set<SourceRecord const*> const& excluded,
        std::unordered_set<SourceRecord*> const& preferred)
    {
        auto const& roots = host.RootRecords();
        std::unordered_map<SourceRecord const*, size_t> position;
        position.reserve(roots.size());
        size_t linkedCount = 0;
        for (size_t i = 0; i < roots.size(); ++i)
        {
            position.emplace(roots[i].get(), i);
            if (!excluded.count(roots[i].get()))
            {
                ++linkedCount;
            }
        }

        std::unordered_set<SourceRecord*> forcedTop;
        for (;;)
        {
            auto& topLevel = host.SiblingsOf(nullptr);
            topLevel.clear();
            for (auto const& root : roots)
            {
                root->Children.clear();
            }

            for (auto const& root : roots)
            {
                SourceRecord* const record = root.get();
                if (excluded.count(record))
                {
                    record->Parent = nullptr;
                    record->Depth = 0;
                    continue;
                }

                SourceRecord* const parent = forcedTop.count(record) ? nullptr : parentOf(record);
                record->Parent = parent;
                host.SiblingsOf(parent).push_back(record);
            }

            std::unordered_set<SourceRecord const*> reached;
            reached.reserve(linkedCount);
            std::vector<SourceRecord*> pending(topLevel.begin(), topLevel.end());
            for (SourceRecord* const top : topLevel)
            {
                top->Depth = 0;
            }

            while (!pending.empty())
            {
                SourceRecord* const record = pending.back();
                pending.pop_back();
                reached.insert(record);
                for (SourceRecord* const child : record->Children)
                {
                    child->Depth = static_cast<uint16_t>(record->Depth + 1);
                    pending.push_back(child);
                }
            }

            if (reached.size() == linkedCount)
            {
                return forcedTop;
            }

            // The first unreached linked record leads, through Parent, into a cycle: walk until a
            // record repeats, then pick the member to force.
            SourceRecord* start = nullptr;
            for (auto const& root : roots)
            {
                if (!reached.count(root.get()) && !excluded.count(root.get()))
                {
                    start = root.get();
                    break;
                }
            }

            std::unordered_set<SourceRecord*> seen;
            SourceRecord* onCycle = start;
            while (seen.insert(onCycle).second)
            {
                onCycle = onCycle->Parent;
            }

            SourceRecord* breakAt = onCycle;
            bool breakAtPreferred = preferred.count(onCycle) != 0;
            for (SourceRecord* member = onCycle->Parent; member != onCycle; member = member->Parent)
            {
                bool const memberPreferred = preferred.count(member) != 0;
                if ((memberPreferred && !breakAtPreferred) ||
                    (memberPreferred == breakAtPreferred && position.at(member) < position.at(breakAt)))
                {
                    breakAt = member;
                    breakAtPreferred = memberPreferred;
                }
            }

            forcedTop.insert(breakAt);
        }
    }
}

void AppendSourceSubtree(SourceRecord* record, std::vector<SourceRecord*>& out)
{
    std::vector<SourceRecord*> pending{ record };
    while (!pending.empty())
    {
        SourceRecord* const next = pending.back();
        pending.pop_back();
        out.push_back(next);
        pending.insert(pending.end(), next->Children.rbegin(), next->Children.rend());
    }
}

uint32_t IndexInSourceSiblings(std::vector<SourceRecord*> const& siblings, SourceRecord const* record) noexcept
{
    return static_cast<uint32_t>(std::find(siblings.begin(), siblings.end(), record) - siblings.begin());
}

// ---- ParentKeyHierarchy ----------------------------------------------------------------------

ParentKeyHierarchy::ParentKeyHierarchy(
    ITrackerHandleManager const* owner,
    winrt::TableViewKeySelector const& keySelector,
    winrt::TableViewKeySelector const& parentKeySelector)
{
    m_keySelector.emplace(owner, keySelector);
    m_parentKeySelector.emplace(owner, parentKeySelector);
}

winrt::hstring ParentKeyHierarchy::MakeNodeKey(winrt::Windows::Foundation::IInspectable const& key)
{
    // v1's equality: boxed value types by value (int 1 == int? 1), objects by reference.
    return winrt::hstring{ ShapingHelpers::MakeNodeKey(key) };
}

void ParentKeyHierarchy::Prepare(
    ISourceHierarchyHost& host,
    std::vector<winrt::Windows::Foundation::IInspectable> const& rootItems)
{
    m_stagedKeyIndex.clear();
    m_stagedNodeKeys.clear();

    // Strict at the verb (spec: the verb throws on a null, empty or duplicate key, a self-parent
    // or a cycle), including a verb parked under DeferRefresh. Tolerant when Refresh() re-reads
    // the committed strategy (drift #9): a null or empty key is excluded, a duplicate key stays
    // with the record holding it (else the first in source order) and the newcomer is excluded,
    // and a cycle is broken by Commit. A selector throw propagates in both modes.
    bool const strict = !m_committed;
    try
    {
        auto const& keySelector = m_keySelector->get();
        auto const& parentKeySelector = m_parentKeySelector->get();
        m_stagedNodeKeys.reserve(rootItems.size());
        for (uint32_t i = 0; i < rootItems.size(); ++i)
        {
            ComputedKeys keys = ComputeKeys(&MakeNodeKey, keySelector, parentKeySelector, rootItems[i]);
            NodeKeys& staged = m_stagedNodeKeys.emplace_back();
            staged.ParentKey = keys.ParentKey;
            if (keys.Key.empty())
            {
                if (strict)
                {
                    throw winrt::hresult_invalid_argument(c_emptyKeyMessage);
                }
                continue;  // excluded: staged Key stays empty
            }

            if (strict && keys.Key == keys.ParentKey)
            {
                throw winrt::hresult_invalid_argument(c_selfParentMessage);
            }

            auto const [slot, inserted] = m_stagedKeyIndex.emplace(keys.Key, i);
            if (!inserted)
            {
                if (strict)
                {
                    throw winrt::hresult_invalid_argument(c_duplicateKeyMessage);
                }

                // The record that holds the key now keeps it; otherwise the first one does.
                auto const holder = m_keyIndex.find(keys.Key);
                bool const isHolder = holder != m_keyIndex.end() &&
                    winrt::get_abi(holder->second->Item.get()) == winrt::get_abi(rootItems[i]);
                if (!isHolder)
                {
                    continue;  // excluded
                }

                NodeKeys& displaced = m_stagedNodeKeys[slot->second];
                displaced.Key = {};
                displaced.KeyObject.reset();
                slot->second = i;
            }

            staged.Key = keys.Key;
            if (keys.KeyObject)
            {
                staged.KeyObject.emplace(host.TrackerOwner(), keys.KeyObject);
            }
        }

        if (strict)
        {
            ValidateNoCycles(host);
        }
    }
    catch (...)
    {
        m_stagedKeyIndex.clear();
        m_stagedNodeKeys.clear();
        throw;
    }
}

void ParentKeyHierarchy::ValidateNoCycles(ISourceHierarchyHost const&) const
{
    size_t const count = m_stagedNodeKeys.size();
    std::vector<std::vector<uint32_t>> children(count);
    std::vector<uint32_t> pending;
    for (uint32_t i = 0; i < count; ++i)
    {
        auto const& parentKey = m_stagedNodeKeys[i].ParentKey;
        auto const parent = parentKey.empty() ? m_stagedKeyIndex.end() : m_stagedKeyIndex.find(parentKey);
        if (parent == m_stagedKeyIndex.end())
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
        throw winrt::hresult_invalid_argument(c_cycleMessage);
    }
}

void ParentKeyHierarchy::MatchForRefresh(
    ISourceHierarchyHost&,
    std::vector<SourceRecord*> const&,
    std::vector<SourceRecordPtr>&)
{
    // Every record is a root record: SourceTable::MatchRecords matched them all.
}

void ParentKeyHierarchy::Commit(ISourceHierarchyHost& host, std::vector<SourceRecord*>&)
{
    auto const& roots = host.RootRecords();
    MUX_ASSERT(roots.size() == m_stagedNodeKeys.size());

    // A record forced to the top level before (a live cycle edit) stays the one forced when the
    // re-read still finds its cycle. Only records still in the table count.
    std::unordered_set<SourceRecord*> preferred;
    m_keyIndex.clear();
    m_nodeKeys.clear();
    m_excluded.clear();
    m_pendingAdoptions.clear();
    for (size_t i = 0; i < roots.size(); ++i)
    {
        SourceRecord* const record = roots[i].get();
        if (m_forcedTop.count(record))
        {
            preferred.insert(record);
        }

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

    m_forcedTop = RelinkAll(
        host, [this](SourceRecord const* record) { return FindParent(m_keyIndex, m_nodeKeys, record); }, m_excluded, preferred);
    m_committed = true;
}

void ParentKeyHierarchy::CollectIncoming(
    winrt::Windows::Foundation::IInspectable const&,
    std::vector<winrt::Windows::Foundation::IInspectable>&) const
{
    // An incoming root item brings nothing else.
}

bool ParentKeyHierarchy::Keys(
    ISourceHierarchyHost& host,
    SourceRecord const* record,
    NodeKeys& keys,
    std::exception_ptr& error) const
{
    auto const keep = [&error](std::exception_ptr const& failure) {
        if (!error)
        {
            error = failure;
        }
    };

    auto const item = record->Item.get();
    winrt::Windows::Foundation::IInspectable key{ nullptr };
    try
    {
        key = m_keySelector->get()(item);
        keys.Key = MakeNodeKey(key);
    }
    catch (...)
    {
        keep(std::current_exception());  // no key: excluded (drift #11)
        return false;
    }

    try
    {
        keys.ParentKey = MakeNodeKey(m_parentKeySelector->get()(item));
    }
    catch (...)
    {
        keep(std::current_exception());  // spec: top level, like an orphan
        keys.ParentKey = {};
    }

    if (keys.Key.empty())
    {
        keep(std::make_exception_ptr(winrt::hresult_invalid_argument(c_emptyKeyMessage)));
        return false;
    }

    auto const holder = m_keyIndex.find(keys.Key);
    if (holder != m_keyIndex.end() && holder->second != record)
    {
        keep(std::make_exception_ptr(winrt::hresult_invalid_argument(c_duplicateKeyMessage)));
        return false;
    }

    if (keys.Key == keys.ParentKey)
    {
        keep(std::make_exception_ptr(winrt::hresult_invalid_argument(c_selfParentMessage)));
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
        error = std::make_exception_ptr(winrt::hresult_invalid_argument(c_cycleMessage));
    }
    return nullptr;
}

void ParentKeyHierarchy::MoveTo(ISourceHierarchyHost& host, SourceRecord* record, SourceRecord* newParent)
{
    SourceRecord* const oldParent = record->Parent;
    uint32_t const oldIndex = IndexInSourceSiblings(host.SiblingsOf(oldParent), record);
    uint32_t const newIndex = SourceOrderPosition(host, record, newParent, &m_excluded);
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

void ParentKeyHierarchy::Include(ISourceHierarchyHost& host, SourceRecord* record, std::exception_ptr& error)
{
    m_excluded.erase(record);
    auto const orphans = WaitingFor(host, m_nodeKeys.at(record).Key, record);
    SourceRecord* const parent = PlaceParent(record, orphans, error);
    uint32_t const index = SourceOrderPosition(host, record, parent, &m_excluded);
    host.Reparent(record, parent, index);

    std::vector<SourceRecordPtr> removed;
    host.RaiseSourceChanged(SourceChangeKind::Add, nullptr, parent, 0, index, { record }, removed);
    Adopt(host, record, orphans);
}

std::vector<SourceRecord*> ParentKeyHierarchy::WaitingFor(
    ISourceHierarchyHost& host,
    winrt::hstring const& key,
    SourceRecord const* record) const
{
    // Top-level records whose parent key is key, in source order. A record forced to the top
    // level is RestoreForcedTop's to place.
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

void ParentKeyHierarchy::Adopt(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecord*> const& orphans)
{
    // Spec Orphan adoption and Bulk threshold: one orphan is a Move; more than one is a bulk
    // change, raised as one Reset.
    if (orphans.size() == 1)
    {
        MoveTo(host, orphans.front(), record);
        return;
    }

    if (orphans.empty())
    {
        return;
    }

    try
    {
        for (SourceRecord* const orphan : orphans)
        {
            host.Reparent(orphan, record, SourceOrderPosition(host, orphan, record, &m_excluded));
        }

        std::vector<SourceRecordPtr> removed;
        host.RaiseSourceChanged(SourceChangeKind::Reset, nullptr, nullptr, 0, 0, {}, removed);
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }
}

void ParentKeyHierarchy::OrphanChildren(ISourceHierarchyHost& host, SourceRecord* record)
{
    // Its children followed its key; without it they are orphans (spec: top level), one Move
    // each (drift #10).
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
    // Spec: back to its proper parent once the key is valid. Each move can free another. Source
    // order (the top-level siblings) keeps the Move order deterministic.
    try
    {
        for (bool moved = true; moved && !m_forcedTop.empty();)
        {
            moved = false;
            std::vector<SourceRecord*> const tops = host.SiblingsOf(nullptr);
            for (SourceRecord* const record : tops)
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
                MoveTo(host, record, parent);
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

    auto const owner = m_keyIndex.find(keys->second.Key);
    if (owner != m_keyIndex.end() && owner->second == record)
    {
        m_keyIndex.erase(owner);
    }
    m_nodeKeys.erase(keys);
}

bool ParentKeyHierarchy::OnInserted(ISourceHierarchyHost& host, SourceRecord* record, std::exception_ptr& error)
{
    // The item is already in the source: a bad key never undoes the insert (spec Misbehaving
    // records). The caller raises the Add (when linked) and then rethrows error.
    m_pendingAdoptions.clear();
    NodeKeys keys;
    if (!Keys(host, record, keys, error))
    {
        m_excluded.insert(record);
        return false;
    }

    winrt::hstring const key = keys.Key;
    m_keyIndex.emplace(key, record);
    m_nodeKeys[record] = std::move(keys);

    auto orphans = WaitingFor(host, key, record);
    SourceRecord* const parent = PlaceParent(record, orphans, error);
    host.Reparent(record, parent, SourceOrderPosition(host, record, parent, &m_excluded));

    if (orphans.size() > 1)
    {
        // Spec Bulk threshold: adopting more than one orphan is one stable Reset, with no Add
        // first. Returning false tells the caller there is nothing left to raise.
        try
        {
            for (SourceRecord* const orphan : orphans)
            {
                host.Reparent(orphan, record, SourceOrderPosition(host, orphan, record, &m_excluded));
            }

            std::vector<SourceRecordPtr> removed;
            host.RaiseSourceChanged(SourceChangeKind::Reset, nullptr, nullptr, 0, 0, { record }, removed);
        }
        catch (...)
        {
            host.RequestReset();
            throw;
        }
        RestoreForcedTop(host);
        return false;
    }

    m_pendingAdoptions = std::move(orphans);
    return true;
}

void ParentKeyHierarchy::AfterInserted(ISourceHierarchyHost& host, SourceRecord* record)
{
    auto const orphans = std::move(m_pendingAdoptions);
    m_pendingAdoptions.clear();
    Adopt(host, record, orphans);
    RestoreForcedTop(host);
}

bool ParentKeyHierarchy::OnRemoving(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecordPtr>&)
{
    if (m_excluded.erase(record))
    {
        return false;  // in no sibling list: its Remove was raised when it was excluded
    }

    m_forcedTop.erase(record);
    Unkey(record);
    OrphanChildren(host, record);
    RestoreForcedTop(host);  // a cycle through record is gone
    return true;
}

void ParentKeyHierarchy::OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t, uint32_t)
{
    if (!m_excluded.count(record))
    {
        ReparentInSourceOrder(host, record, &m_excluded);
    }
}

bool ParentKeyHierarchy::Reevaluate(ISourceHierarchyHost& host, SourceRecord* record, winrt::hstring const&)
{
    // Any property can feed a selector (drift #17): re-key on every PropertyChanged. Placement
    // follows spec Misbehaving records; each tree change raises its own SourceChange, and the
    // first failure propagates into the app's setter afterwards.
    std::exception_ptr error;
    NodeKeys keys;
    bool const keyed = Keys(host, record, keys, error);
    bool const wasExcluded = m_excluded.count(record) != 0;
    auto const current = m_nodeKeys.find(record);
    if (wasExcluded ? !keyed
                    : keyed && current != m_nodeKeys.end() && current->second.Key == keys.Key &&
                          current->second.ParentKey == keys.ParentKey)
    {
        return false;  // nothing changed (decided: no rethrow for a record that stays as it was)
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
    }
    else
    {
        winrt::hstring const key = keys.Key;
        m_keyIndex[key] = record;
        m_nodeKeys[record] = std::move(keys);
        if (wasExcluded)
        {
            Include(host, record, error);
        }
        else
        {
            std::vector<SourceRecord*> orphans;
            if (keyChanged)
            {
                orphans = WaitingFor(host, key, record);
            }

            // A parent at or below record (or below an orphan it adopts) is a cycle: record goes
            // to the top level with its subtree (spec).
            MoveTo(host, record, PlaceParent(record, orphans, error));
            Adopt(host, record, orphans);
        }
    }

    RestoreForcedTop(host);
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

void ParentKeyHierarchy::ForEachChildRecord(std::function<void(SourceRecordPtr const&)> const&) const
{
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
    m_pendingAdoptions.clear();
    m_committed = false;
    m_keySelector.reset();
    m_parentKeySelector.reset();
}

// ---- ChildCollectionsHierarchy ---------------------------------------------------------------

namespace
{
    constexpr wchar_t c_nullChildMessage[] =
        L"TableViewSource2.ChildrenBy: a child collection contains a null item.";
    constexpr wchar_t c_duplicateChildMessage[] =
        L"TableViewSource2.ChildrenBy: the same item object appears in the tree more than once. "
        L"Each item can appear only once.";
}

ChildCollectionsHierarchy::ChildCollectionsHierarchy(
    ITrackerHandleManager const* owner,
    winrt::TableViewChildrenSelector2 const& childrenSelector)
    : m_trackerOwner(owner)
{
    m_childrenSelector.emplace(owner, childrenSelector);
}

void ChildCollectionsHierarchy::Prepare(
    ISourceHierarchyHost& host,
    std::vector<winrt::Windows::Foundation::IInspectable> const& rootItems)
{
    // m_staged holds every node, roots included, pre-order; ChildCount is a node's direct
    // children (each followed by its own subtree).
    m_staged.clear();
    try
    {
        auto const& selector = m_childrenSelector->get();
        std::unordered_set<ObjectAddress> seen;
        seen.reserve(rootItems.size());
        for (auto const& item : rootItems)
        {
            seen.insert(SourceRecord::AddressOf(item));
        }

        auto const stage = [&](auto const& self, winrt::Windows::Foundation::IInspectable const& item) -> void
        {
            size_t const index = m_staged.size();
            m_staged.emplace_back().Item = item;

            auto const collection = selector(item);
            if (!collection)
            {
                return;
            }

            auto const children = CollectionAccessor2{ host.TrackerOwner(), collection }.Enumerate(true /* throwIfUnsupported */);
            m_staged[index].ChildCollection = collection;
            m_staged[index].ChildCount = static_cast<uint32_t>(children.size());
            for (auto const& child : children)
            {
                if (!child)
                {
                    throw winrt::hresult_invalid_argument(c_nullChildMessage);
                }

                if (!seen.insert(SourceRecord::AddressOf(child)).second)
                {
                    throw winrt::hresult_invalid_argument(c_duplicateChildMessage);
                }

                self(self, child);
            }
        };

        for (auto const& item : rootItems)
        {
            stage(stage, item);
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
    // pointer first, else the same object address. No app calls.
    std::unordered_set<SourceRecord const*> kept;
    for (SourceRecord* const match : rootMatches)
    {
        if (match)
        {
            kept.insert(match);
        }
    }

    size_t cursor = 0;
    auto const visit = [&](auto const& self, SourceRecord* record) -> void
    {
        uint32_t const childCount = m_staged[cursor].ChildCount;
        m_staged[cursor++].Match = record;

        ChildFeed* feed = nullptr;
        if (record)
        {
            auto const found = m_childFeeds.find(record);
            feed = found == m_childFeeds.end() ? nullptr : found->second.get();
        }

        size_t const feedSize = feed ? feed->Records.size() : 0;
        std::vector<bool> used(feedSize, false);
        std::unordered_map<ObjectAddress, size_t> byAddress;
        for (size_t j = 0; j < feedSize; ++j)
        {
            byAddress.emplace(feed->Records[j]->Address, j);
        }

        for (uint32_t i = 0; i < childCount; ++i)
        {
            auto const& item = m_staged[cursor].Item;
            SourceRecord* found = nullptr;
            if (i < feedSize && !used[i] && winrt::get_abi(feed->Records[i]->Item.get()) == winrt::get_abi(item))
            {
                found = feed->Records[i].get();
                used[i] = true;
            }
            else if (feedSize > 0)
            {
                auto const at = byAddress.find(SourceRecord::AddressOf(item));
                if (at != byAddress.end() && !used[at->second])
                {
                    found = feed->Records[at->second].get();
                    used[at->second] = true;
                }
            }

            if (found)
            {
                kept.insert(found);
            }
            self(self, found);
        }
    };

    for (SourceRecord* const match : rootMatches)
    {
        visit(visit, match);
    }
    MUX_ASSERT(cursor == m_staged.size());

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

        UnsubscribeFromChildChanges(*it->second);
        it->second->Accessor.reset();
        it = m_childFeeds.erase(it);
    }
}

void ChildCollectionsHierarchy::Commit(ISourceHierarchyHost& host, std::vector<SourceRecord*>& added)
{
    // Every feed is rebound to the collection Prepare read (drift #15), even when unchanged.
    size_t cursor = 0;
    auto const link = [&](auto const& self, SourceRecordPtr const& owner) -> void
    {
        StagedNode const& node = m_staged[cursor++];
        owner->Children.clear();
        if (!node.ChildCollection)
        {
            DestroyChildFeed(host, owner.get());
            return;
        }

        auto& slot = m_childFeeds[owner.get()];
        if (!slot)
        {
            slot = std::make_unique<ChildFeed>();
        }

        ChildFeed& feed = *slot;
        UnsubscribeFromChildChanges(feed);
        feed.Owner = owner;
        feed.Accessor.reset();
        feed.Accessor.emplace(host.TrackerOwner(), node.ChildCollection);

        std::unordered_map<SourceRecord*, SourceRecordPtr> owned;
        for (auto& record : feed.Records)
        {
            if (record)
            {
                SourceRecord* const raw = record.get();
                owned.emplace(raw, std::move(record));
            }
        }
        feed.Records.clear();

        uint32_t const childCount = node.ChildCount;
        for (uint32_t i = 0; i < childCount; ++i)
        {
            StagedNode const& childNode = m_staged[cursor];
            SourceRecordPtr child;
            if (childNode.Match)
            {
                child = std::move(owned.at(childNode.Match));
            }
            else
            {
                child = host.CreateRecord(childNode.Item);
                added.push_back(child.get());
            }

            child->Parent = owner.get();
            child->Depth = static_cast<uint16_t>(owner->Depth + 1);
            owner->Children.push_back(child.get());
            feed.Records.push_back(child);
            self(self, child);
        }

        SubscribeToChildChanges(host, feed);
    };

    for (auto const& root : host.RootRecords())
    {
        link(link, root);
    }
    MUX_ASSERT(cursor == m_staged.size());
    m_staged.clear();
}

void ChildCollectionsHierarchy::CollectIncoming(
    winrt::Windows::Foundation::IInspectable const& item,
    std::vector<winrt::Windows::Foundation::IInspectable>& items) const
{
    CollectSubtreeItems(item, items);
}

void ChildCollectionsHierarchy::CollectSubtreeItems(
    winrt::Windows::Foundation::IInspectable const& item,
    std::vector<winrt::Windows::Foundation::IInspectable>& items) const
{
    // Every descendant is pushed (a null or a repeat included, for ValidateIncoming to reject);
    // an object is expanded once, so a collection that contains an ancestor terminates. A
    // selector or enumeration that throws makes that item a leaf here (spec Misbehaving
    // records); BuildChildFeed calls the selector again and reports the error.
    auto const& selector = m_childrenSelector->get();
    std::unordered_set<ObjectAddress> expanded;
    auto const collect = [&](auto const& self, winrt::Windows::Foundation::IInspectable const& parent) -> void
    {
        if (!parent || !expanded.insert(SourceRecord::AddressOf(parent)).second)
        {
            return;
        }

        std::vector<winrt::Windows::Foundation::IInspectable> children;
        try
        {
            auto const collection = selector(parent);
            if (!collection)
            {
                return;
            }

            children = CollectionAccessor2{ m_trackerOwner, collection }.Enumerate(true /* throwIfUnsupported */);
        }
        catch (...)
        {
            return;
        }

        for (auto const& child : children)
        {
            items.push_back(child);
            self(self, child);
        }
    };

    collect(collect, item);
}

void ChildCollectionsHierarchy::BuildChildFeed(
    ISourceHierarchyHost& host,
    SourceRecordPtr const& record,
    std::exception_ptr& error)
{
    auto const capture = [&error]()
    {
        if (!error)
        {
            error = std::current_exception();
        }
    };

    winrt::Windows::Foundation::IInspectable collection{ nullptr };
    std::vector<winrt::Windows::Foundation::IInspectable> children;
    try
    {
        collection = m_childrenSelector->get()(record->Item.get());
        if (!collection)
        {
            return;
        }

        children = CollectionAccessor2{ host.TrackerOwner(), collection }.Enumerate(true /* throwIfUnsupported */);
    }
    catch (...)
    {
        // Spec Misbehaving records: a throwing selector makes a leaf (no feed). The error still
        // propagates from the app's Add / Insert or setter; the tree is consistent, so no re-read.
        capture();
        return;
    }

    auto& slot = m_childFeeds[record.get()];
    slot = std::make_unique<ChildFeed>();
    ChildFeed& feed = *slot;
    feed.Owner = record;
    feed.Accessor.emplace(host.TrackerOwner(), collection);

    for (auto const& child : children)
    {
        try
        {
            host.ValidateIncoming({ child }, nullptr);
        }
        catch (...)
        {
            // The feed now differs from its collection: the next notification re-reads.
            capture();
            host.RequestReset();
            continue;
        }

        auto childRecord = host.CreateRecord(child);
        childRecord->Parent = record.get();
        childRecord->Depth = static_cast<uint16_t>(record->Depth + 1);
        record->Children.push_back(childRecord.get());
        feed.Records.push_back(childRecord);
        BuildChildFeed(host, childRecord, error);
    }

    SubscribeToChildChanges(host, feed);
}

void ChildCollectionsHierarchy::SubscribeToChildChanges(ISourceHierarchyHost& host, ChildFeed& feed)
{
    using ObserveKind = CollectionAccessor2::ObserveKind;

    // Raw event token; the lambda holds only weak references and a thread check. It routes to
    // this strategy only while it is still the host's current one.
    auto const weakHost = host.WeakHost();
    auto const threadCheck = host.MakeThreadCheck();
    uint64_t const generation = Generation();
    SourceRecordWeakPtr const owner = feed.Owner;

    try
    {
        switch (feed.Accessor->GetObserveKind())
        {
        case ObserveKind::NotifyCollectionChanged:
            feed.EventToken = feed.Accessor->AsNotifyCollectionChanged().CollectionChanged(
                [weakHost, threadCheck, generation, owner](auto const&, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
                {
                    threadCheck();
                    if (auto const locked = weakHost.lock())
                    {
                        SourceHierarchy* const current = locked->CurrentHierarchy();
                        if (current && current->Generation() == generation)
                        {
                            static_cast<ChildCollectionsHierarchy*>(current)->OnChildCollectionChanged(*locked, owner, args);
                        }
                    }
                });
            break;

        case ObserveKind::ObservableVector:
            feed.EventToken = feed.Accessor->AsObservableVector().VectorChanged(
                [weakHost, threadCheck, generation, owner](auto const&, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
                {
                    threadCheck();
                    if (auto const locked = weakHost.lock())
                    {
                        SourceHierarchy* const current = locked->CurrentHierarchy();
                        if (current && current->Generation() == generation)
                        {
                            static_cast<ChildCollectionsHierarchy*>(current)->OnChildVectorChanged(*locked, owner, args);
                        }
                    }
                });
            break;

        case ObserveKind::BindableObservableVector:
            feed.EventToken = feed.Accessor->AsBindableObservableVector().VectorChanged(
                [weakHost, threadCheck, generation, owner](auto const&, winrt::Windows::Foundation::IInspectable const&)
                {
                    threadCheck();
                    if (auto const locked = weakHost.lock())
                    {
                        SourceHierarchy* const current = locked->CurrentHierarchy();
                        if (current && current->Generation() == generation)
                        {
                            static_cast<ChildCollectionsHierarchy*>(current)->OnChildBindableVectorChanged(*locked, owner);
                        }
                    }
                });
            break;

        default:
            break;
        }
    }
    catch (...)
    {
        // Drift #15: a collection that refuses the handler stays unobserved.
        feed.EventToken = {};
    }
}

void ChildCollectionsHierarchy::UnsubscribeFromChildChanges(ChildFeed& feed) noexcept
{
    if (!feed.Accessor || !feed.EventToken.value)
    {
        return;
    }

    using ObserveKind = CollectionAccessor2::ObserveKind;
    auto const token = std::exchange(feed.EventToken, {});
    try
    {
        // safe_get: a collected collection is skipped. Never get() on this path.
        switch (feed.Accessor->GetObserveKind())
        {
        case ObserveKind::NotifyCollectionChanged:
            if (auto const source = feed.Accessor->AsNotifyCollectionChanged(true))
            {
                source.CollectionChanged(token);
            }
            break;

        case ObserveKind::ObservableVector:
            if (auto const source = feed.Accessor->AsObservableVector(true))
            {
                source.VectorChanged(token);
            }
            break;

        case ObserveKind::BindableObservableVector:
            if (auto const source = feed.Accessor->AsBindableObservableVector(true))
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
    auto const oneOf = [](auto const& items) { return items && items.Size() == 1; };

    // Drift #15: only a single Add or a single matching Remove applies positionally.
    bool isReset = true;
    switch (action)
    {
    case Action::Add:
        isReset = !oneOf(newItems) || newIndex < 0 || newIndex > size;
        break;
    case Action::Remove:
        if (oneOf(oldItems) && oldIndex >= 0 && oldIndex < size)
        {
            auto const oldItem = oldItems.GetAt(0);
            isReset = !oldItem || SourceRecord::AddressOf(oldItem) != feed.Records[oldIndex]->Address;
        }
        break;
    default:
        break;
    }

    // False: the host re-read or skipped the change; nothing here may be touched (drift #14).
    if (!host.AdmitChange(isReset))
    {
        return;
    }

    std::exception_ptr error;
    if (action == Action::Add)
    {
        error = InsertChild(host, owner, feed, static_cast<uint32_t>(newIndex), newItems.GetAt(0));
    }
    else
    {
        RemoveChild(host, owner.get(), feed, static_cast<uint32_t>(oldIndex));
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
    default:
        // ItemChanged (a replace) re-reads (drift #15).
        break;
    }

    if (!host.AdmitChange(isReset))
    {
        return;
    }

    std::exception_ptr error;
    if (change == Change::ItemInserted)
    {
        error = InsertChild(host, owner, feed, index, item);
    }
    else
    {
        RemoveChild(host, owner.get(), feed, index);
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
    // Compute before commit: the item and its whole subtree are read and checked first.
    std::vector<winrt::Windows::Foundation::IInspectable> incoming{ item };
    try
    {
        CollectSubtreeItems(item, incoming);
        host.ValidateIncoming(incoming, nullptr);
    }
    catch (...)
    {
        host.RequestReset();
        throw;
    }

    auto record = host.CreateRecord(item);
    SourceRecord* const raw = record.get();
    feed.Records.insert(feed.Records.begin() + index, record);
    host.Reparent(raw, owner.get(), index);

    std::exception_ptr error;
    BuildChildFeed(host, record, error);

    std::vector<SourceRecord*> added;
    AppendSourceSubtree(raw, added);
    std::vector<SourceRecordPtr> removed;
    host.RaiseSourceChanged(SourceChangeKind::Add, nullptr, owner.get(), 0, index, added, removed);

    // The caller drains the owed re-read first, then rethrows.
    return error;
}

void ChildCollectionsHierarchy::RemoveChild(ISourceHierarchyHost& host, SourceRecord* owner, ChildFeed& feed, uint32_t index)
{
    SourceRecordPtr record = std::move(feed.Records[index]);
    feed.Records.erase(feed.Records.begin() + index);

    SourceRecord* const raw = record.get();
    auto& siblings = host.SiblingsOf(owner);
    siblings.erase(std::remove(siblings.begin(), siblings.end(), raw), siblings.end());
    raw->Parent = nullptr;

    std::vector<SourceRecordPtr> removed;
    host.ReleaseRecord(raw);
    removed.push_back(std::move(record));
    TakeFeed(host, raw, removed);

    // `removed` keeps the subtree alive until the handler returns.
    host.RaiseSourceChanged(SourceChangeKind::Remove, owner, nullptr, index, 0, {}, removed);
}

void ChildCollectionsHierarchy::TakeFeed(
    ISourceHierarchyHost& host,
    SourceRecord const* owner,
    std::vector<SourceRecordPtr>& removed)
{
    auto const found = m_childFeeds.find(owner);
    if (found == m_childFeeds.end())
    {
        return;
    }

    std::unique_ptr<ChildFeed> feed = std::move(found->second);
    m_childFeeds.erase(found);
    UnsubscribeFromChildChanges(*feed);
    feed->Accessor.reset();

    for (auto& record : feed->Records)
    {
        if (!record)
        {
            continue;
        }

        SourceRecord* const raw = record.get();
        host.ReleaseRecord(raw);
        removed.push_back(std::move(record));
        TakeFeed(host, raw, removed);
    }
}

bool ChildCollectionsHierarchy::OnInserted(ISourceHierarchyHost& host, SourceRecord* record, std::exception_ptr& error)
{
    // Top level in source order; the caller validated the subtree (CollectIncoming).
    host.Reparent(record, nullptr, SourceOrderPosition(host, record, nullptr, nullptr));
    for (auto const& root : host.RootRecords())
    {
        if (root.get() == record)
        {
            BuildChildFeed(host, root, error);
            break;
        }
    }

    return true;
}

void ChildCollectionsHierarchy::AfterInserted(ISourceHierarchyHost&, SourceRecord*)
{
    // Nothing waits for a ChildrenBy record.
}

bool ChildCollectionsHierarchy::OnRemoving(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecordPtr>& removed)
{
    // record stays in the top level (the caller reads its index and unlinks it); its subtree
    // leaves the lookups and goes into removed after it.
    TakeFeed(host, record, removed);
    return true;
}

void ChildCollectionsHierarchy::OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t, uint32_t)
{
    ReparentInSourceOrder(host, record, nullptr);
}

bool ChildCollectionsHierarchy::Reevaluate(ISourceHierarchyHost&, SourceRecord*, winrt::hstring const&)
{
    // Drift #17: a child-collection property swap is Milestone 6 (live shaping).
    return false;
}

void ChildCollectionsHierarchy::TakeChildRecords(ISourceHierarchyHost& host, std::vector<SourceRecordPtr>& removed)
{
    std::vector<SourceRecord const*> owners;
    for (auto const& root : host.RootRecords())
    {
        if (m_childFeeds.count(root.get()))
        {
            owners.push_back(root.get());
        }
    }

    for (SourceRecord const* const owner : owners)
    {
        TakeFeed(host, owner, removed);
    }
}

void ChildCollectionsHierarchy::DestroyChildFeed(ISourceHierarchyHost& host, SourceRecord const* owner) noexcept
{
    // Teardown-safe: unsubscribes through safe_get and reads no Item. Records are not released
    // through the host (drift #13): every caller has already taken them out of the lookups, or
    // SourceTable::Clear() clears the lookups wholesale.
    auto const found = m_childFeeds.find(owner);
    if (found == m_childFeeds.end())
    {
        return;
    }

    std::unique_ptr<ChildFeed> feed = std::move(found->second);
    m_childFeeds.erase(found);
    UnsubscribeFromChildChanges(*feed);
    feed->Accessor.reset();

    for (auto const& record : feed->Records)
    {
        if (record)
        {
            DestroyChildFeed(host, record.get());
        }
    }
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

void ChildCollectionsHierarchy::Release(ISourceHierarchyHost& host) noexcept
{
    while (!m_childFeeds.empty())
    {
        DestroyChildFeed(host, m_childFeeds.begin()->first);
    }

    m_staged.clear();
    m_childrenSelector.reset();
}
