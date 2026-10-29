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
