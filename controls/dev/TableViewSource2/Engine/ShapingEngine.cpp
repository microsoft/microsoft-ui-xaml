// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/ShapingEngine.h"
#include "ShapingHelpers.h"

#include <algorithm>
#include <string>
#include <unordered_set>

ShapingEngine::ShapingEngine(
    ITrackerHandleManager const* trackerOwner,
    SourceTable const& source,
    ShapeDelegates& delegates,
    PathEvaluator& paths)
    : m_trackerOwner(trackerOwner)
    , m_source(source)
    , m_delegates(delegates)
    , m_paths(paths)
{
}

ShapeSpec const& ShapingEngine::Applied() const noexcept
{
    return m_applied;
}

ShapedTree& ShapingEngine::Tree() noexcept
{
    return m_tree;
}

ShapedTree const& ShapingEngine::Tree() const noexcept
{
    return m_tree;
}

GroupMap const& ShapingEngine::GroupsByIdentity() const noexcept
{
    return m_groupsByIdentity;
}

bool ShapingEngine::InShapedParent(SourceRecord const& record) noexcept
{
    // E8: a record failing the filter stays as the ancestor of a match exactly when its shaped
    // children are non-empty.
    return (record.Shape.Flags & ShapeFlags::PassesFilter) != ShapeFlags::None || !record.Shape.ShapedChildren.empty();
}

bool ShapingEngine::InShapedTree(ShapedNode* node) noexcept
{
    if (!node || node->IsGroup())
    {
        return node != nullptr;
    }

    for (SourceRecord const* record = node->AsRecord(); record; record = record->Parent)
    {
        if (!InShapedParent(*record))
        {
            return false;
        }
    }

    return true;
}

namespace
{
    // A record as the empty spec leaves it: it keeps its own expansion; everything else is
    // recomputed. Without ChildrenShaped, ShapedChildAt reads the source Children.
    void ResetRecordShape(SourceRecord* record, GroupNode& root) noexcept
    {
        record->Shape.Flags = (record->Shape.Flags & (ShapeFlags::Expanded | ShapeFlags::OverlayCollapsed)) | ShapeFlags::PassesFilter;
        record->Shape.SortKeys.clear();
        record->Shape.ShapedChildren.clear();
        record->Shape.Group = record->Parent ? nullptr : &root;
        record->ChildRows.MarkStale();
        for (uint32_t j = 0; j < record->Children.size(); ++j)
        {
            record->Children[j]->IndexInParent = j;
        }
    }
}

namespace
{
    // Stable bottom-up merge sort over indices. Unlike std::stable_sort it stays in bounds (and does
    // not assert in debug) when an app comparer is inconsistent: every step reads only the two
    // runs being merged, so a bad comparer gives some order, never a fault. Exceptions from `less`
    // propagate; only local buffers are touched.
    template <typename Less>
    void StableSortIndices(std::vector<uint32_t>& items, Less&& less)
    {
        size_t const n = items.size();
        if (n < 2)
        {
            return;
        }

        std::vector<uint32_t> buffer(n);
        for (size_t width = 1; width < n; width *= 2)
        {
            for (size_t lo = 0; lo < n; lo += 2 * width)
            {
                size_t const mid = std::min(lo + width, n);
                size_t const hi = std::min(lo + 2 * width, n);
                size_t i = lo;
                size_t j = mid;
                size_t k = lo;
                while (i < mid && j < hi)
                {
                    // Take from the right run only when strictly less: equal keys keep their order.
                    buffer[k++] = less(items[j], items[i]) ? items[j++] : items[i++];
                }
                while (i < mid)
                {
                    buffer[k++] = items[i++];
                }
                while (j < hi)
                {
                    buffer[k++] = items[j++];
                }
            }
            items.swap(buffer);
        }
    }

    // Null-key ordering (spec): null sorts before every non-null key (ValueComparer does this);
    // Descending negates the whole comparison, so nulls come last.
    int CompareKeys(winrt::IInspectable const& a, winrt::IInspectable const& b, winrt::SortDirection direction)
    {
        int const c = ShapingHelpers::ValueComparer::Compare(a, b);
        return direction == winrt::SortDirection::Descending ? -c : c;
    }

    // One length-prefixed segment per group level after the "g|" marker (V3), so no key text can
    // forge a boundary between levels.
    winrt::hstring ChildGroupIdentity(winrt::hstring const& parentIdentity, winrt::IInspectable const& key)
    {
        winrt::hstring const segment = ShapingHelpers::ValueKey::ToObjectLookupKey(key);
        std::wstring identity{ parentIdentity.empty() ? std::wstring_view{ L"g|" } : std::wstring_view{ parentIdentity } };
        identity.append(std::to_wstring(segment.size())).append(L":").append(segment);
        return winrt::hstring{ identity };
    }
}

void ShapingEngine::Apply(ShapeSpec const& desired, ShapeDelta delta)
{
    Rebuild(desired, delta);
}

void ShapingEngine::Reapply(ShapeSpec const& desired)
{
    Rebuild(desired, ShapeDelta::All);
}

void ShapingEngine::ReapplyForSource(ShapeSpec const& desired, std::span<SourceRecord* const> arrivals)
{
    try
    {
        Rebuild(desired, ShapeDelta::All);
    }
    catch (...)
    {
        // The source has already changed: departed records must not stay linked once the
        // SourceTable frees them.
        UnlinkOnly(arrivals);
        throw;
    }
}

bool ShapingEngine::IsStale() const noexcept
{
    return m_stale;
}

void ShapingEngine::Rebuild(ShapeSpec const& desired, ShapeDelta delta, std::span<LiveChange const> only)
{
    if (m_stale)
    {
        // Cached state cannot be trusted: every record is re-read.
        delta = ShapeDelta::All;
        only = {};
    }

    bool const filterActive = !desired.Filters.empty();
    bool const sortActive = !desired.Sorts.empty();
    bool const childrenShaped = filterActive || sortActive;
    uint16_t const levels = static_cast<uint16_t>(desired.Groups.size());
    bool const refilter = (delta & ShapeDelta::Filter) != ShapeDelta::None;
    bool const rereadSorts = (delta & ShapeDelta::Sort) != ShapeDelta::None;
    bool const rereadGroups = (delta & ShapeDelta::GroupKey) != ShapeDelta::None;

    // ---- Compute: every app call happens here; nothing visible is written. ----

    // Every record breadth-first, so a record's children follow it as one contiguous run starting
    // at childStart[i]; a reverse walk visits children before parents.
    auto const& topLevel = m_source.TopLevel();
    uint32_t const topCount = static_cast<uint32_t>(topLevel.size());
    std::vector<SourceRecord*> records(topLevel.begin(), topLevel.end());
    std::vector<uint32_t> childStart;
    for (size_t i = 0; i < records.size(); ++i)
    {
        childStart.push_back(static_cast<uint32_t>(records.size()));
        records.insert(records.end(), records[i]->Children.begin(), records[i]->Children.end());
    }
    uint32_t const n = static_cast<uint32_t>(records.size());

    std::vector<winrt::IInspectable> items(n, nullptr);
    for (uint32_t i = 0; i < n; ++i)
    {
        items[i] = records[i]->Item.get();
    }

    // Live shaping (E7): the re-reads `delta` names cover only the dirty records; every other
    // record reuses its cached state. A dirty record re-reads every part (drift #43).
    std::vector<uint8_t> reread(n, 1);
    if (!only.empty())
    {
        std::unordered_set<SourceRecord const*> dirty;
        dirty.reserve(only.size());
        for (LiveChange const& change : only)
        {
            dirty.insert(change.Record);
        }
        for (uint32_t i = 0; i < n; ++i)
        {
            reread[i] = dirty.contains(records[i]) ? 1 : 0;
        }
    }

    // 1. Filter. A predicate's exception propagates with nothing written.
    std::vector<uint8_t> passes(n, 1);
    if (filterActive)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            if (refilter && reread[i])
            {
                for (FilterSpec const& filter : desired.Filters)
                {
                    if (!m_delegates.Passes(filter.Predicate.Id, items[i]))
                    {
                        passes[i] = 0;
                        break;
                    }
                }
            }
            else
            {
                passes[i] = (records[i]->Shape.Flags & ShapeFlags::PassesFilter) != ShapeFlags::None;
            }
        }
    }

    // Ancestors of matches (E8): a record is in its shaped parent when it passes or keeps a child.
    std::vector<uint8_t> inShaped(passes);
    for (uint32_t i = n; i-- > 0;)
    {
        uint32_t const first = childStart[i];
        uint32_t const last = first + static_cast<uint32_t>(records[i]->Children.size());
        for (uint32_t j = first; j < last && !inShaped[i]; ++j)
        {
            inShaped[i] = inShaped[j];
        }
    }

    // 2. Sort keys, for the records in the shaped tree. Path keys never throw (null when the path
    // cannot be evaluated); a key selector's exception propagates. Comparer levels hold no key.
    std::vector<std::vector<winrt::IInspectable>> keys(sortActive ? n : 0);
    std::vector<uint8_t> keysRead(n, 0);
    StagedReapply staged;
    staged.Records.resize(n);
    if (sortActive)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            if (!inShaped[i])
            {
                continue;
            }

            SourceRecord* const record = records[i];
            keys[i].resize(desired.Sorts.size(), nullptr);
            bool const cached = !(rereadSorts && reread[i]) &&
                (record->Shape.Flags & ShapeFlags::SortKeysValid) != ShapeFlags::None &&
                record->Shape.SortKeys.size() == desired.Sorts.size();
            if (cached)
            {
                for (size_t l = 0; l < desired.Sorts.size(); ++l)
                {
                    auto const& slot = record->Shape.SortKeys[l];
                    keys[i][l] = slot ? slot->get() : nullptr;
                }
                continue;
            }

            auto& slots = staged.Records[i].Shape.SortKeys;
            slots.resize(desired.Sorts.size());
            for (size_t l = 0; l < desired.Sorts.size(); ++l)
            {
                SortSpec const& level = desired.Sorts[l];
                if (level.SortKind == SortSpec::Kind::Comparer)
                {
                    continue;
                }
                keys[i][l] = level.SortKind == SortSpec::Kind::Path ?
                    m_paths.Value(level.Path, items[i]) :
                    m_delegates.Key(level.Delegate.Id, items[i]);
                slots[l].emplace(m_trackerOwner, keys[i][l]);
            }
            keysRead[i] = 1;
        }
    }

    auto const rowLess = [&](uint32_t a, uint32_t b) -> bool
    {
        for (size_t l = 0; l < desired.Sorts.size(); ++l)
        {
            SortSpec const& level = desired.Sorts[l];
            int const c = level.SortKind == SortSpec::Kind::Comparer ?
                m_delegates.Compare(level.Delegate.Id, items[a], items[b]) :
                CompareKeys(keys[a][l], keys[b][l], level.Direction);
            if (c != 0)
            {
                return c < 0;
            }
        }
        return false;
    };

    // 3. Shaped children of every record, filtered and sorted (only while a filter or sort is on;
    // otherwise the source Children are the shaped children).
    if (childrenShaped)
    {
        std::vector<uint32_t> run;
        for (uint32_t i = 0; i < n; ++i)
        {
            uint32_t const first = childStart[i];
            uint32_t const last = first + static_cast<uint32_t>(records[i]->Children.size());
            run.clear();
            for (uint32_t j = first; j < last; ++j)
            {
                if (inShaped[j])
                {
                    run.push_back(j);
                }
            }
            if (sortActive)
            {
                StableSortIndices(run, rowLess);
            }

            auto& shapedChildren = staged.Records[i].Shape.ShapedChildren;
            shapedChildren.reserve(run.size());
            for (uint32_t const j : run)
            {
                shapedChildren.push_back(records[j]);
            }
        }
    }

    // 4. Top level: grouped (top-level records only) or straight under the root.
    GroupNode& root = m_tree.Root();
    std::vector<uint32_t> topRun;
    for (uint32_t i = 0; i < topCount; ++i)
    {
        if (inShaped[i])
        {
            topRun.push_back(i);
        }
    }

    if (levels == 0)
    {
        if (sortActive)
        {
            StableSortIndices(topRun, rowLess);
        }
        staged.RootChildren.reserve(topRun.size());
        for (uint32_t const i : topRun)
        {
            staged.RootChildren.push_back(records[i]);
            staged.Records[i].Shape.Group = &root;
        }
    }
    else
    {
        // Bucket in filtered source order, so group order None is first appearance. A throwing
        // group key selector puts the record in the null-key group (spec, "Misbehaving records").
        std::vector<std::pair<GroupNode*, std::vector<uint32_t>>> buckets;
        std::unordered_map<GroupNode*, size_t> bucketOf;
        std::vector<winrt::IInspectable> groupKeys(levels, nullptr);
        for (uint32_t const i : topRun)
        {
            SourceRecord* const record = records[i];
            GroupNode* const cachedGroup = record->Shape.Group;
            bool const cached = !(rereadGroups && reread[i]) && cachedGroup && !cachedGroup->IsRoot() &&
                cachedGroup->GroupLevel + 1 == levels;
            if (cached)
            {
                for (GroupNode* g = cachedGroup; g && !g->IsRoot(); g = g->Parent)
                {
                    groupKeys[g->GroupLevel] = g->Key ? g->Key->get() : nullptr;
                }
            }
            else
            {
                for (uint16_t l = 0; l < levels; ++l)
                {
                    try
                    {
                        groupKeys[l] = m_delegates.Key(desired.Groups[l].Key.Id, items[i]);
                    }
                    catch (...)
                    {
                        groupKeys[l] = nullptr;
                    }
                }
            }

            GroupNode* parent = &root;
            for (uint16_t l = 0; l < levels; ++l)
            {
                winrt::hstring const identity = ChildGroupIdentity(parent->Identity, groupKeys[l]);
                auto it = staged.GroupsByIdentity.find(identity);
                if (it == staged.GroupsByIdentity.end())
                {
                    auto group = std::make_unique<GroupNode>();
                    group->Key.emplace(m_trackerOwner, groupKeys[l]);
                    group->Identity = identity;
                    group->Parent = parent;
                    group->GroupLevel = l;
                    it = staged.GroupsByIdentity.emplace(identity, std::move(group)).first;
                    if (parent == &root)
                    {
                        staged.RootChildren.push_back(it->second.get());
                    }
                    else
                    {
                        parent->Children.push_back(it->second.get());
                    }
                }
                parent = it->second.get();
                ++parent->ItemCount;
            }

            auto const [bucket, added] = bucketOf.emplace(parent, buckets.size());
            if (added)
            {
                buckets.emplace_back(parent, std::vector<uint32_t>{});
            }
            buckets[bucket->second].second.push_back(i);
            staged.Records[i].Shape.Group = parent;
        }

        // Rows within each bottom-level group; row sort never moves groups.
        for (auto& [group, rows] : buckets)
        {
            if (sortActive)
            {
                StableSortIndices(rows, rowLess);
            }
            group->Children.reserve(rows.size());
            for (uint32_t const i : rows)
            {
                group->Children.push_back(records[i]);
            }
        }

        // 5. Groups by key within each parent (None keeps first appearance).
        auto const sortGroups = [](std::vector<ShapedNode*>& list, winrt::SortDirection direction)
        {
            if (direction == winrt::SortDirection::None || list.size() < 2)
            {
                return;
            }
            std::vector<winrt::IInspectable> groupKey;
            std::vector<uint32_t> order;
            for (uint32_t k = 0; k < list.size(); ++k)
            {
                GroupNode const* const group = list[k]->AsGroup();
                groupKey.push_back(group->Key ? group->Key->get() : nullptr);
                order.push_back(k);
            }
            StableSortIndices(order, [&](uint32_t a, uint32_t b)
            {
                return CompareKeys(groupKey[a], groupKey[b], direction) < 0;
            });
            std::vector<ShapedNode*> sorted;
            sorted.reserve(list.size());
            for (uint32_t const k : order)
            {
                sorted.push_back(list[k]);
            }
            list.swap(sorted);
        };

        sortGroups(staged.RootChildren, desired.Groups[0].Direction);
        for (auto& entry : staged.GroupsByIdentity)
        {
            GroupNode* const group = entry.second.get();
            if (group->GroupLevel + 1u < levels)
            {
                sortGroups(group->Children, desired.Groups[group->GroupLevel + 1u].Direction);
            }
        }
    }

    // Compute bits per record.
    for (uint32_t i = 0; i < n; ++i)
    {
        ShapeFlags flags = passes[i] ? ShapeFlags::PassesFilter : ShapeFlags::None;
        if (childrenShaped)
        {
            flags |= ShapeFlags::ChildrenShaped;
        }
        if (sortActive && inShaped[i])
        {
            flags |= ShapeFlags::SortKeysValid;
        }
        staged.Records[i].Record = records[i];
        staged.Records[i].Shape.Flags = flags;
    }

    // A level present in both specs with another selector changes what an identity means (F4).
    bool newGroupSelector = false;
    for (size_t l = 0; l < std::min(m_applied.Groups.size(), desired.Groups.size()); ++l)
    {
        newGroupSelector = newGroupSelector || m_applied.Groups[l].Key.Id != desired.Groups[l].Key.Id;
    }

    // F5: the overlay is cleared when the filter itself changes, not by a source-driven rebuild.
    bool const clearOverlay = !(m_applied.Filters == desired.Filters);

    // Capacity for the write phase, which must not allocate.
    ShapeSpec appliedNext{ desired };
    m_retired.reserve(m_retired.size() + m_groupsByIdentity.size());

    ShapeFlags const kept = ShapeFlags::Expanded | ShapeFlags::PlacementMark |
        (clearOverlay ? ShapeFlags::None : ShapeFlags::OverlayCollapsed);
    CommitStaged(records, topCount, staged, keysRead, std::move(appliedNext), levels, newGroupSelector, kept);
}

void ShapingEngine::CommitStaged(
    std::vector<SourceRecord*> const& records,
    uint32_t topCount,
    StagedReapply& staged,
    std::vector<uint8_t> const& keysRead,
    ShapeSpec&& applied,
    uint16_t levels,
    bool newGroupSelector,
    ShapeFlags kept) noexcept
{
    GroupNode& root = m_tree.Root();
    uint32_t const n = static_cast<uint32_t>(records.size());
    bool const childrenShaped = !applied.Filters.empty() || !applied.Sorts.empty();

    // ---- Write: nothing below throws. ----
    m_tree.BeginReset();

    for (uint32_t i = 0; i < n; ++i)
    {
        SourceRecord* const record = records[i];
        ShapeState& next = staged.Records[i].Shape;
        record->Shape.Flags = (record->Shape.Flags & kept) | next.Flags;
        if (keysRead[i])
        {
            record->Shape.SortKeys = std::move(next.SortKeys);
        }
        else if ((next.Flags & ShapeFlags::SortKeysValid) == ShapeFlags::None)
        {
            record->Shape.SortKeys.clear();
        }
        record->Shape.ShapedChildren = std::move(next.ShapedChildren);
        record->Shape.Group = i < topCount ? next.Group : nullptr;
        record->ChildRows.MarkStale();

        if (childrenShaped)
        {
            auto const& shapedChildren = record->Shape.ShapedChildren;
            for (uint32_t j = 0; j < shapedChildren.size(); ++j)
            {
                shapedChildren[j]->IndexInParent = j;
            }
        }
        else
        {
            for (uint32_t j = 0; j < record->Children.size(); ++j)
            {
                record->Children[j]->IndexInParent = j;
            }
        }
    }

    for (auto& entry : staged.GroupsByIdentity)
    {
        GroupNode* const group = entry.second.get();
        for (uint32_t j = 0; j < group->Children.size(); ++j)
        {
            group->Children[j]->IndexInParent = j;
        }
        group->ChildRows.MarkStale();
    }

    for (uint32_t j = 0; j < staged.RootChildren.size(); ++j)
    {
        staged.RootChildren[j]->IndexInParent = j;
    }
    root.Children = std::move(staged.RootChildren);
    root.ChildRows.MarkStale();

    // The replaced groups stay alive until ReleaseRetired (P5).
    m_groupsByIdentity.swap(staged.GroupsByIdentity);
    for (auto& entry : staged.GroupsByIdentity)
    {
        m_retired.push_back(std::move(entry.second));
    }
    staged.GroupsByIdentity.clear();

    m_applied = std::move(applied);
    m_stale = false;
    m_tree.EndReset(levels, m_groupsByIdentity, newGroupSelector);
}

void ShapingEngine::UnlinkOnly(std::span<SourceRecord* const> arrivals) noexcept
{
    ShapeSpec const& applied = m_applied;
    bool const filterActive = !applied.Filters.empty();
    bool const sortActive = !applied.Sorts.empty();
    bool const childrenShaped = filterActive || sortActive;
    uint16_t const levels = static_cast<uint16_t>(applied.Groups.size());

    // The current records breadth-first, as in Rebuild.
    auto const& topLevel = m_source.TopLevel();
    uint32_t const topCount = static_cast<uint32_t>(topLevel.size());
    std::vector<SourceRecord*> records(topLevel.begin(), topLevel.end());
    std::vector<uint32_t> childStart;
    for (size_t i = 0; i < records.size(); ++i)
    {
        childStart.push_back(static_cast<uint32_t>(records.size()));
        records.insert(records.end(), records[i]->Children.begin(), records[i]->Children.end());
    }
    uint32_t const n = static_cast<uint32_t>(records.size());

    // Old lists are matched by pointer value only: an old entry not found here left the source
    // and is never dereferenced.
    std::unordered_map<ShapedNode const*, uint32_t> indexOf;
    indexOf.reserve(n);
    for (uint32_t i = 0; i < n; ++i)
    {
        indexOf.emplace(static_cast<ShapedNode const*>(records[i]), i);
    }
    std::unordered_set<SourceRecord const*> const arrived(arrivals.begin(), arrivals.end());

    // An arrival is never shown: its delegates were not read. Under the empty spec this hides only
    // top-level arrivals (children are not shaped there).
    std::vector<uint8_t> passes(n, 1);
    for (uint32_t i = 0; i < n; ++i)
    {
        passes[i] = !arrived.contains(records[i]) &&
            (!childrenShaped || !filterActive || (records[i]->Shape.Flags & ShapeFlags::PassesFilter) != ShapeFlags::None);
    }

    std::vector<uint8_t> inShaped(passes);
    for (uint32_t i = n; i-- > 0;)
    {
        uint32_t const first = childStart[i];
        uint32_t const last = first + static_cast<uint32_t>(records[i]->Children.size());
        for (uint32_t j = first; j < last && !inShaped[i]; ++j)
        {
            inShaped[i] = inShaped[j];
        }
    }

    StagedReapply staged;
    staged.Records.resize(n);
    std::vector<uint8_t> placed(n, 0);

    // Takes a current record from an old list when it still belongs to [first, last).
    auto const take = [&](ShapedNode const* old, uint32_t first, uint32_t last) -> std::optional<uint32_t>
    {
        auto const it = indexOf.find(old);
        if (it == indexOf.end())
        {
            return std::nullopt;
        }
        uint32_t const j = it->second;
        if (j < first || j >= last || !inShaped[j] || placed[j])
        {
            return std::nullopt;
        }
        placed[j] = 1;
        return j;
    };

    // Shaped children: the survivors in their old order, then the rest in source order.
    if (childrenShaped)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            uint32_t const first = childStart[i];
            uint32_t const last = first + static_cast<uint32_t>(records[i]->Children.size());
            auto& shapedChildren = staged.Records[i].Shape.ShapedChildren;
            for (SourceRecord const* const old : records[i]->Shape.ShapedChildren)
            {
                if (auto const j = take(old, first, last))
                {
                    shapedChildren.push_back(records[*j]);
                }
            }
            for (uint32_t j = first; j < last; ++j)
            {
                if (inShaped[j] && !placed[j])
                {
                    placed[j] = 1;
                    shapedChildren.push_back(records[j]);
                }
            }
        }
    }

    GroupNode& root = m_tree.Root();
    if (levels == 0)
    {
        // Under the empty spec root keeps source order (the order surgical edits index by);
        // otherwise the survivors keep their shaped order.
        if (childrenShaped)
        {
            for (ShapedNode const* const old : root.Children)
            {
                if (auto const j = take(old, 0, topCount))
                {
                    staged.RootChildren.push_back(records[*j]);
                }
            }
        }
        for (uint32_t j = 0; j < topCount; ++j)
        {
            if (inShaped[j] && !placed[j])
            {
                placed[j] = 1;
                staged.RootChildren.push_back(records[j]);
            }
        }
        for (ShapedNode* const node : staged.RootChildren)
        {
            staged.Records[indexOf.at(node)].Shape.Group = &root;
        }
    }
    else
    {
        // The old groups, mirrored with the same identity and key; an emptied group is dropped.
        std::unordered_set<ShapedNode const*> oldGroups;
        oldGroups.reserve(m_groupsByIdentity.size());
        for (auto const& entry : m_groupsByIdentity)
        {
            oldGroups.insert(static_cast<ShapedNode const*>(entry.second.get()));
        }

        auto const mirror = [&](auto const& self, std::vector<ShapedNode*> const& oldList, GroupNode* parent, std::vector<ShapedNode*>& out) -> void
        {
            for (ShapedNode* const old : oldList)
            {
                if (oldGroups.contains(old))
                {
                    auto const* const oldGroup = static_cast<GroupNode const*>(old);
                    auto group = std::make_unique<GroupNode>();
                    if (oldGroup->Key)
                    {
                        group->Key.emplace(m_trackerOwner, oldGroup->Key->get());
                    }
                    group->Identity = oldGroup->Identity;
                    group->Parent = parent;
                    group->GroupLevel = oldGroup->GroupLevel;
                    self(self, oldGroup->Children, group.get(), group->Children);
                    if (group->Children.empty())
                    {
                        continue;
                    }

                    if (group->GroupLevel + 1u == levels)
                    {
                        group->ItemCount = static_cast<uint32_t>(group->Children.size());
                    }
                    else
                    {
                        for (ShapedNode* const child : group->Children)
                        {
                            group->ItemCount += static_cast<GroupNode const*>(child)->ItemCount;
                        }
                    }
                    out.push_back(group.get());
                    winrt::hstring const identity = group->Identity;
                    staged.GroupsByIdentity.emplace(identity, std::move(group));
                }
                else if (auto const j = take(old, 0, topCount))
                {
                    out.push_back(records[*j]);
                    staged.Records[*j].Shape.Group = parent;
                }
            }
        };
        mirror(mirror, root.Children, &root, staged.RootChildren);

        // A top-level record with no cached group (it was not bucketed before) has no safe place.
        for (uint32_t j = 0; j < topCount; ++j)
        {
            if (inShaped[j] && !placed[j])
            {
                inShaped[j] = 0;
                passes[j] = 0;
                staged.Records[j].Shape.ShapedChildren.clear();
            }
        }
    }

    for (uint32_t i = 0; i < n; ++i)
    {
        ShapeFlags flags = passes[i] ? ShapeFlags::PassesFilter : ShapeFlags::None;
        if (childrenShaped)
        {
            flags |= ShapeFlags::ChildrenShaped;
        }
        if (sortActive && inShaped[i] && (records[i]->Shape.Flags & ShapeFlags::SortKeysValid) != ShapeFlags::None)
        {
            flags |= ShapeFlags::SortKeysValid;
        }
        staged.Records[i].Record = records[i];
        staged.Records[i].Shape.Flags = flags;
    }

    std::vector<uint8_t> const keysRead(n, 0);
    ShapeSpec appliedNext{ applied };
    m_retired.reserve(m_retired.size() + m_groupsByIdentity.size());

    ShapeFlags const kept = ShapeFlags::Expanded | ShapeFlags::PlacementMark | ShapeFlags::OverlayCollapsed;
    CommitStaged(records, topCount, staged, keysRead, std::move(appliedNext), levels, false /* newGroupSelector */, kept);
    m_stale = true;
}

namespace
{
    // Bits a surgical commit keeps from the record (the user's expansion, the context-row overlay;
    // PlacementMark is always clear outside PlacementIndex).
    constexpr ShapeFlags c_keptFlags = ShapeFlags::Expanded | ShapeFlags::PlacementMark | ShapeFlags::OverlayCollapsed;

    // The index of `node` in `parent`'s list when it is there (a node about to leave it).
    std::optional<uint32_t> IndexInList(ShapedNode* parent, ShapedNode const* node) noexcept
    {
        if (!node || node->ShapedParent() != parent)
        {
            return std::nullopt;
        }
        uint32_t const at = node->IndexInParent;
        if (at >= parent->ShapedChildCount() || parent->ShapedChildAt(at) != node)
        {
            return std::nullopt;
        }
        return at;
    }
}

void ShapingEngine::ApplySourceChange(SourceChange const& change)
{
    // Spec §6: a single-item Add, Remove, Replace or Move links or unlinks the nodes that enter
    // or leave (drift #20, #35). A reparenting Move under a non-empty spec, a nested Replace (not
    // raised by SourceTable) and a stale engine (drift #33: the root may hide arrivals) rebuild
    // with the applied spec (one Reset).
    bool const emptySpec = m_applied == ShapeSpec{};
    bool const reparent = change.Kind == SourceChangeKind::Move && change.OldParent != change.NewParent;
    bool const nestedReplace = change.Kind == SourceChangeKind::Replace && (change.OldParent || change.NewParent);
    bool const surgical = !m_stale && change.Kind != SourceChangeKind::Reset && !nestedReplace && (emptySpec || !reparent);
    if (!surgical)
    {
        ShapeSpec const applied{ m_applied };
        ReapplyForSource(applied, change.Added);
        return;
    }

    if (emptySpec && change.Kind != SourceChangeKind::Replace)
    {
        ApplyUnshapedSourceChange(change);
        return;
    }

    // Compute first: every app call (filter, sort and group keys, comparers) and every allocation
    // runs before any write; the write phases (CommitInsert, CommitLeave, Link, Unlink) cannot
    // throw. A failed compute still has to drop the departed records (drift #33).
    try
    {
        switch (change.Kind)
        {
        case SourceChangeKind::Add:
        {
            StagedEdit edit;
            StageInsert(edit, change.Added, change.NewParent);
            if (edit.Parent)
            {
                StagePlacement(edit, change.NewIndex, nullptr);
            }
            CommitInsert(edit);
            return;
        }

        case SourceChangeKind::Remove:
        {
            // Runs no app code. The departed subtree is alive until the handler returns.
            auto* const record = const_cast<SourceRecord*>(change.Removed.front());
            StagedLeave leave = StageLeave(record, change.OldParent, change.OldIndex, nullptr);
            CommitLeave(leave);
            return;
        }

        case SourceChangeKind::Replace:
        {
            // A top-level leaf (SourceTable's only Replace): the new record's chain is computed
            // first, so the groups it reuses are not pruned when the old record leaves them.
            StagedEdit edit;
            GroupNode* const keep = StageInsert(edit, change.Added, nullptr);
            auto* const old = const_cast<SourceRecord*>(change.Removed.front());
            StagedLeave leave = StageLeave(old, nullptr, change.OldIndex, keep);
            if (edit.Parent)
            {
                StagePlacement(edit, change.NewIndex, leave.Top);
            }
            CommitLeave(leave);
            CommitInsert(edit);
            return;
        }

        default: // SourceChangeKind::Move within one parent
        {
            std::vector<SourceRecord*> const& siblings = change.NewParent ? change.NewParent->Children : m_source.TopLevel();
            SourceRecord* const record = siblings[change.NewIndex];
            if (!InShapedParent(*record))
            {
                return;
            }

            // A source-owned list (no filter or sort) follows the source.
            if (change.NewParent && (change.NewParent->Shape.Flags & ShapeFlags::ChildrenShaped) == ShapeFlags::None)
            {
                m_tree.Unlink(record, change.NewParent, change.OldIndex, true /* isMove */);
                m_tree.Link(record, change.NewParent, change.NewIndex, true /* isMove */);
                return;
            }

            // Otherwise the record moves only when its place among equal keys (or in source
            // order) changes; a move a sort hides raises nothing. Groups never move (sticky, E9).
            ShapedNode* const parent = record->ShapedParent();
            uint32_t const index = PlacementIndex(record, record->Shape, parent, record, change.NewIndex);
            if (index == record->IndexInParent)
            {
                return;
            }
            m_tree.Unlink(record, true /* isMove */);
            m_tree.Link(record, parent, index, true /* isMove */);
            return;
        }
        }
    }
    catch (...)
    {
        UnlinkOnly(change.Added);
        throw;
    }
}

void ShapingEngine::ApplyUnshapedSourceChange(SourceChange const& change)
{
    // Under the empty spec every record is in the shaped tree in source order, so the source
    // position is the shaped position. Runs no app code.
    GroupNode& root = m_tree.Root();
    auto const shapedParent = [&root](SourceRecord* parent) -> ShapedNode* {
        return parent ? static_cast<ShapedNode*>(parent) : static_cast<ShapedNode*>(&root);
    };

    switch (change.Kind)
    {
    case SourceChangeKind::Add:
    {
        // Compute: capacity for the engine-owned root list (E4).
        if (!change.NewParent)
        {
            root.Children.reserve(root.Children.size() + 1);
        }

        // Write: nothing below throws. Added is the new subtree in pre-order (Added[0] its root);
        // its records start as Reapply leaves a record under the empty spec.
        for (SourceRecord* const record : change.Added)
        {
            ResetRecordShape(record, root);
        }

        m_tree.Link(change.Added.front(), shapedParent(change.NewParent), change.NewIndex, false /* isMove */);
        return;
    }

    case SourceChangeKind::Remove:
    {
        // The departed subtree is alive until the handler returns; nothing below reads its
        // children. Unlink takes the old place: SourceTable has already detached the record.
        auto* const record = const_cast<SourceRecord*>(change.Removed.front());
        m_tree.Unlink(record, shapedParent(change.OldParent), change.OldIndex, false /* isMove */);
        record->Shape.Group = nullptr;
        return;
    }

    default: // SourceChangeKind::Move
    {
        // A Move carries no record: it is the one now at NewIndex among NewParent's children.
        std::vector<SourceRecord*> const& siblings = change.NewParent ? change.NewParent->Children : m_source.TopLevel();
        SourceRecord* const record = siblings[change.NewIndex];

        // Compute: capacity for the engine-owned root list (E4).
        if (!change.NewParent && change.OldParent)
        {
            root.Children.reserve(root.Children.size() + 1);
        }

        // Write: nothing below throws. The node keeps its counts and expansion across the move.
        m_tree.Unlink(record, shapedParent(change.OldParent), change.OldIndex, true /* isMove */);
        record->Shape.Group = change.NewParent ? nullptr : &root;
        m_tree.Link(record, shapedParent(change.NewParent), change.NewIndex, true /* isMove */);
        return;
    }
    }
}

namespace
{
    // The first member of a property path ("Addr.City" -> "Addr", "Items[0]" -> "Items"): the
    // property a row item's PropertyChanged names when the path's value may have changed.
    std::wstring_view FirstPathSegment(winrt::hstring const& path) noexcept
    {
        std::wstring_view const view{ path };
        size_t const end = view.find_first_of(L".[");
        return end == std::wstring_view::npos ? view : view.substr(0, end);
    }

    // Pre-order: record, then each child's subtree (StageInsert's `added` order).
    void CollectSubtree(SourceRecord* record, std::vector<SourceRecord*>& out)
    {
        out.push_back(record);
        for (SourceRecord* const child : record->Children)
        {
            CollectSubtree(child, out);
        }
    }
}

void ShapingEngine::ApplyItemChanged(SourceRecord* record, winrt::hstring const& propertyName)
{
    ShapeSpec const& spec = m_applied;
    if (spec == ShapeSpec{})
    {
        return;
    }

    if (m_stale)
    {
        // Drift #33: cached state may be wrong, so one item cannot be placed against it.
        ShapeSpec const applied{ m_applied };
        Rebuild(applied, ShapeDelta::All);
        return;
    }

    bool const filterActive = !spec.Filters.empty();
    bool const sortActive = !spec.Sorts.empty();
    bool const childrenShaped = filterActive || sortActive;
    uint16_t const levels = static_cast<uint16_t>(spec.Groups.size());
    bool const topLevel = record->Parent == nullptr;
    if (!childrenShaped && !topLevel)
    {
        // GroupBy only: a nested record's list follows the source.
        return;
    }

    // ---- Compute: every app call (predicates, keys, comparers) runs before any write. ----
    winrt::IInspectable const item = record->Item.get();
    bool passes = true;
    for (FilterSpec const& filter : spec.Filters)
    {
        if (!m_delegates.Passes(filter.Predicate.Id, item))
        {
            passes = false;
            break;
        }
    }

    bool const wasPassing = (record->Shape.Flags & ShapeFlags::PassesFilter) != ShapeFlags::None;
    bool const wasIn = InShapedParent(*record);
    bool const isIn = passes || !record->Shape.ShapedChildren.empty();

    if (!wasIn && !isIn)
    {
        return;
    }

    if (!wasIn)
    {
        // Enters (E8: with any failing ancestors that had no shaped child). StageInsert re-reads
        // the subtree, which has no shaped member yet.
        std::vector<SourceRecord*> subtree;
        CollectSubtree(record, subtree);
        StagedEdit edit;
        StageInsert(edit, subtree, record->Parent);
        if (edit.Parent)
        {
            StagePlacement(edit, std::nullopt, nullptr);
        }
        CommitInsert(edit);
        return;
    }

    if (!isIn)
    {
        // Leaves (E8: with the failing ancestors it was the only shaped child of; E9: with the
        // groups it was the last member of). Runs no app code.
        StagedLeave leave = StageLeave(record, record->Parent, record->IndexInParent, nullptr);
        CommitLeave(leave);
        record->Shape.Flags &= ~(ShapeFlags::PassesFilter | ShapeFlags::SortKeysValid);
        record->Shape.SortKeys.clear();
        record->ChildRows.MarkStale();
        return;
    }

    // Stays: re-read what the property may affect, and diff against the cached keys.
    bool const keysValid = (record->Shape.Flags & ShapeFlags::SortKeysValid) != ShapeFlags::None &&
        record->Shape.SortKeys.size() == spec.Sorts.size();
    ShapeState next;
    next.Flags = (passes ? ShapeFlags::PassesFilter : ShapeFlags::None) |
        (childrenShaped ? ShapeFlags::ChildrenShaped : ShapeFlags::None) |
        (sortActive ? ShapeFlags::SortKeysValid : ShapeFlags::None);
    bool sortChanged = false;
    if (sortActive)
    {
        next.SortKeys.resize(spec.Sorts.size());
        for (size_t l = 0; l < spec.Sorts.size(); ++l)
        {
            SortSpec const& level = spec.Sorts[l];
            if (level.SortKind == SortSpec::Kind::Comparer)
            {
                // Opaque: the record is re-placed (a place that does not change raises nothing).
                sortChanged = true;
                continue;
            }

            winrt::IInspectable const old = keysValid && record->Shape.SortKeys[l] ? record->Shape.SortKeys[l]->get() : nullptr;
            bool const read = !keysValid || level.SortKind == SortSpec::Kind::Key ||
                propertyName.empty() || FirstPathSegment(level.Path) == std::wstring_view{ propertyName };
            winrt::IInspectable const key = !read ? old : level.SortKind == SortSpec::Kind::Path ?
                m_paths.Value(level.Path, item) :
                m_delegates.Key(level.Delegate.Id, item);
            next.SortKeys[l].emplace(m_trackerOwner, key);
            sortChanged = sortChanged || !keysValid || CompareKeys(old, key, winrt::SortDirection::Ascending) != 0;
        }
    }

    // A top-level record's group chain (group keys are delegates: always re-read). A throwing
    // selector puts the record in the null-key group, as in a rebuild.
    std::vector<winrt::IInspectable> groupKeys;
    bool groupChanged = false;
    if (topLevel && levels > 0)
    {
        groupKeys.resize(levels, nullptr);
        winrt::hstring identity;
        for (uint16_t l = 0; l < levels; ++l)
        {
            try
            {
                groupKeys[l] = m_delegates.Key(spec.Groups[l].Key.Id, item);
            }
            catch (...)
            {
                groupKeys[l] = nullptr;
            }
            identity = ChildGroupIdentity(identity, groupKeys[l]);
        }
        GroupNode const* const group = record->Shape.Group;
        groupChanged = !group || group->Identity != identity;
    }

    if (!sortChanged && !groupChanged)
    {
        // Only a failing ancestor of a match may flip PassesFilter here; its row stays.
        if (passes != wasPassing)
        {
            record->Shape.Flags ^= ShapeFlags::PassesFilter;
        }
        return;
    }

    if (groupChanged)
    {
        // To another group: a surgical remove then insert (E9: groups created and pruned). The
        // record keeps its subtree, shaped children and expansion.
        StagedEdit edit;
        edit.Staged.Record = record;
        edit.Staged.Shape.Flags = next.Flags;
        edit.Staged.Shape.SortKeys = std::move(next.SortKeys);
        edit.Staged.Shape.ShapedChildren = record->Shape.ShapedChildren;
        GroupNode const* const keep = StageGroupChain(edit, groupKeys);
        StagedLeave leave = StageLeave(record, nullptr, record->IndexInParent, keep);
        StagePlacement(edit, std::nullopt, leave.Top);

        CommitLeave(leave);
        CommitInsert(edit);
        return;
    }

    // A new place among its siblings: one move (a place that does not change raises nothing).
    ShapedNode* const parent = record->ShapedParent();
    uint32_t const index = PlacementIndex(record, next, parent, record, std::nullopt);

    // Write: nothing below throws.
    bool const moves = index != record->IndexInParent;
    if (moves)
    {
        m_tree.Unlink(record, true /* isMove */);
    }
    record->Shape.Flags = (record->Shape.Flags & ~(ShapeFlags::PassesFilter | ShapeFlags::SortKeysValid)) |
        (next.Flags & (ShapeFlags::PassesFilter | ShapeFlags::SortKeysValid));
    record->Shape.SortKeys = std::move(next.SortKeys);
    if (moves)
    {
        m_tree.Link(record, parent, index, true /* isMove */);
    }
}

void ShapingEngine::ApplyItemsChanged(std::span<LiveChange const> changes)
{
    if (m_applied == ShapeSpec{})
    {
        return;
    }

    ShapeSpec const applied{ m_applied };
    Rebuild(applied, ShapeDelta::Filter | ShapeDelta::GroupKey | ShapeDelta::Sort, changes);
}

int32_t ShapingEngine::CompareRows(
    winrt::IInspectable const& a,
    ShapeState const& aShape,
    winrt::IInspectable const& b,
    ShapeState const& bShape) const
{
    auto const key = [](ShapeState const& shape, size_t level) -> winrt::IInspectable {
        if (level < shape.SortKeys.size() && shape.SortKeys[level])
        {
            return shape.SortKeys[level]->get();
        }
        return nullptr;
    };

    for (size_t l = 0; l < m_applied.Sorts.size(); ++l)
    {
        SortSpec const& level = m_applied.Sorts[l];
        int const c = level.SortKind == SortSpec::Kind::Comparer ?
            m_delegates.Compare(level.Delegate.Id, a, b) :
            CompareKeys(key(aShape, l), key(bShape, l), level.Direction);
        if (c != 0)
        {
            return c;
        }
    }
    return 0;
}

GroupNode* ShapingEngine::StageInsert(StagedEdit& edit, std::span<SourceRecord* const> added, SourceRecord* parent)
{
    ShapeSpec const& spec = m_applied;
    bool const filterActive = !spec.Filters.empty();
    bool const sortActive = !spec.Sorts.empty();
    bool const childrenShaped = filterActive || sortActive;
    uint32_t const n = static_cast<uint32_t>(added.size());

    auto const readSortKeys = [&](winrt::IInspectable const& item, ShapeState& shape)
    {
        shape.SortKeys.resize(spec.Sorts.size());
        for (size_t l = 0; l < spec.Sorts.size(); ++l)
        {
            SortSpec const& level = spec.Sorts[l];
            if (level.SortKind == SortSpec::Kind::Comparer)
            {
                continue;
            }
            winrt::IInspectable const key = level.SortKind == SortSpec::Kind::Path ?
                m_paths.Value(level.Path, item) :
                m_delegates.Key(level.Delegate.Id, item);
            shape.SortKeys[l].emplace(m_trackerOwner, key);
        }
    };

    std::vector<StagedRecord> staged(n);
    std::vector<winrt::IInspectable> items(n, nullptr);
    std::unordered_map<SourceRecord const*, uint32_t> indexOf;
    indexOf.reserve(n);
    for (uint32_t i = 0; i < n; ++i)
    {
        staged[i].Record = added[i];
        items[i] = added[i]->Item.get();
        indexOf.emplace(added[i], i);
    }

    // Filter. A predicate's exception propagates with nothing written.
    std::vector<uint8_t> passes(n, 1);
    if (filterActive)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            for (FilterSpec const& filter : spec.Filters)
            {
                if (!m_delegates.Passes(filter.Predicate.Id, items[i]))
                {
                    passes[i] = 0;
                    break;
                }
            }
        }
    }

    // Ancestors of matches (E8): pre-order puts every child after its parent.
    std::vector<uint8_t> inShaped(passes);
    for (uint32_t i = n; i-- > 1;)
    {
        if (inShaped[i])
        {
            auto const up = indexOf.find(added[i]->Parent);
            MUX_ASSERT(up != indexOf.end());
            inShaped[up->second] = 1;
        }
    }

    for (uint32_t i = 0; i < n; ++i)
    {
        ShapeFlags flags = passes[i] ? ShapeFlags::PassesFilter : ShapeFlags::None;
        if (childrenShaped)
        {
            flags |= ShapeFlags::ChildrenShaped;
        }
        if (sortActive && inShaped[i])
        {
            flags |= ShapeFlags::SortKeysValid;
            readSortKeys(items[i], staged[i].Shape);
        }
        staged[i].Shape.Flags = flags;
    }

    // Shaped children of every staged record, filtered and sorted.
    if (childrenShaped)
    {
        std::vector<uint32_t> run;
        for (uint32_t i = 0; i < n; ++i)
        {
            run.clear();
            for (SourceRecord* const child : added[i]->Children)
            {
                auto const at = indexOf.find(child);
                MUX_ASSERT(at != indexOf.end());
                if (inShaped[at->second])
                {
                    run.push_back(at->second);
                }
            }
            if (sortActive)
            {
                StableSortIndices(run, [&](uint32_t a, uint32_t b)
                {
                    return CompareRows(items[a], staged[a].Shape, items[b], staged[b].Shape) < 0;
                });
            }

            auto& shapedChildren = staged[i].Shape.ShapedChildren;
            shapedChildren.reserve(run.size());
            for (uint32_t const j : run)
            {
                shapedChildren.push_back(added[j]);
            }
        }
    }

    edit.Staged = std::move(staged[0]);
    edit.Descendants.reserve(n - 1);
    for (uint32_t i = 1; i < n; ++i)
    {
        edit.Descendants.push_back(std::move(staged[i]));
    }

    if (!inShaped[0])
    {
        return nullptr;
    }

    // E8: failing ancestors with no shaped child enter with the record, bottom-up.
    SourceRecord* lower = added[0];
    SourceRecord* current = parent;
    while (current && !InShapedParent(*current))
    {
        StagedRecord link;
        link.Record = current;
        link.Shape.Flags = (current->Shape.Flags & ShapeFlags::PassesFilter) | ShapeFlags::ChildrenShaped;
        if (sortActive)
        {
            link.Shape.Flags |= ShapeFlags::SortKeysValid;
            readSortKeys(current->Item.get(), link.Shape);
        }
        link.Shape.ShapedChildren.push_back(lower);
        edit.Chain.push_back(std::move(link));
        lower = current;
        current = current->Parent;
    }

    if (current)
    {
        edit.Parent = current;
        return nullptr;
    }

    // The topmost record is top-level: under the root, or in its group chain.
    GroupNode& root = m_tree.Root();
    StagedRecord& top = edit.Chain.empty() ? edit.Staged : edit.Chain.back();
    if (spec.Groups.empty())
    {
        top.Shape.Group = &root;
        edit.Parent = &root;
        return &root;
    }

    // A throwing group key selector puts the record in the null-key group (spec, "Misbehaving
    // records"), as in a rebuild.
    winrt::IInspectable const item = top.Record->Item.get();
    std::vector<winrt::IInspectable> keys(spec.Groups.size(), nullptr);
    for (size_t l = 0; l < spec.Groups.size(); ++l)
    {
        try
        {
            keys[l] = m_delegates.Key(spec.Groups[l].Key.Id, item);
        }
        catch (...)
        {
            keys[l] = nullptr;
        }
    }
    return StageGroupChain(edit, keys);
}

GroupNode* ShapingEngine::StageGroupChain(StagedEdit& edit, std::vector<winrt::IInspectable> const& keys)
{
    StagedRecord& top = edit.Chain.empty() ? edit.Staged : edit.Chain.back();
    uint16_t const levels = static_cast<uint16_t>(keys.size());

    GroupNode* existing = &m_tree.Root();
    uint16_t level = 0;
    for (; level < levels; ++level)
    {
        auto const it = m_groupsByIdentity.find(ChildGroupIdentity(existing->Identity, keys[level]));
        if (it == m_groupsByIdentity.end())
        {
            break;
        }
        existing = it->second.get();
    }

    GroupNode* bottom = existing;
    if (level < levels)
    {
        GroupMap fresh;
        edit.NewGroupEntries.reserve(levels - level);
        for (; level < levels; ++level)
        {
            winrt::hstring const identity = ChildGroupIdentity(bottom->Identity, keys[level]);
            auto group = std::make_unique<GroupNode>();
            group->Key.emplace(m_trackerOwner, keys[level]);
            group->Identity = identity;
            group->Parent = bottom;
            group->GroupLevel = level;
            group->Children.reserve(1);
            GroupNode* const raw = group.get();
            fresh.emplace(identity, std::move(group));
            edit.NewGroupEntries.push_back(fresh.extract(identity));
            bottom = raw;
        }
        m_groupsByIdentity.reserve(m_groupsByIdentity.size() + edit.NewGroupEntries.size());
    }

    top.Shape.Group = bottom;
    edit.Parent = existing;
    return existing;
}

void ShapingEngine::StagePlacement(StagedEdit& edit, std::optional<uint32_t> sourceIndex, ShapedNode const* skip)
{
    ShapedNode* const parent = edit.Parent;
    if (!edit.NewGroupEntries.empty())
    {
        edit.ShapedIndex = GroupPlacementIndex(edit.NewGroupEntries.front().mapped().get(), parent, skip);
    }
    else
    {
        StagedRecord const& top = edit.Chain.empty() ? edit.Staged : edit.Chain.back();
        edit.ShapedIndex = PlacementIndex(top.Record, top.Shape, parent, skip, edit.Chain.empty() ? sourceIndex : std::nullopt);
    }

    // Capacity for the write phase's insert (E4).
    if (GroupNode* const group = parent->AsGroup())
    {
        group->Children.reserve(group->Children.size() + 1);
    }
    else if (SourceRecord* const record = parent->AsRecord();
             (record->Shape.Flags & ShapeFlags::ChildrenShaped) != ShapeFlags::None)
    {
        record->Shape.ShapedChildren.reserve(record->Shape.ShapedChildren.size() + 1);
    }
}

uint32_t ShapingEngine::PlacementIndex(
    SourceRecord* record,
    ShapeState const& shape,
    ShapedNode* parent,
    ShapedNode const* skip,
    std::optional<uint32_t> sourceIndex)
{
    std::vector<SourceRecord*> const& siblings = record->Parent ? record->Parent->Children : m_source.TopLevel();
    uint32_t const at = sourceIndex ? *sourceIndex :
        static_cast<uint32_t>(std::find(siblings.begin(), siblings.end(), record) - siblings.begin());

    SourceRecord* const parentRecord = parent->AsRecord();
    if (parentRecord && (parentRecord->Shape.Flags & ShapeFlags::ChildrenShaped) == ShapeFlags::None)
    {
        return at;
    }

    // Virtual indices: the list without `skip`.
    uint32_t const count = parent->ShapedChildCount();
    std::optional<uint32_t> const skipped = IndexInList(parent, skip);
    uint32_t const skipAt = skipped ? *skipped : count;
    uint32_t const size = skipped ? count - 1 : count;
    auto const actual = [skipAt](uint32_t v) { return v >= skipAt ? v + 1 : v; };
    auto const virtualOf = [skipAt](uint32_t a) { return a > skipAt ? a - 1 : a; };

    if (m_applied.Sorts.empty())
    {
        // Shaped order is source order: after the nearest earlier source sibling in this list.
        for (uint32_t s = at; s-- > 0;)
        {
            SourceRecord* const prior = siblings[s];
            if (prior != skip && InShapedParent(*prior) && prior->ShapedParent() == parent)
            {
                return virtualOf(prior->IndexInParent) + 1;
            }
        }
        return 0;
    }

    winrt::IInspectable const item = record->Item.get();
    auto const sibling = [&](uint32_t v) { return parent->ShapedChildAt(actual(v))->AsRecord(); };

    // The run of siblings whose keys tie with the record's: [first, last).
    uint32_t first = 0;
    uint32_t hi = size;
    while (first < hi)
    {
        uint32_t const mid = first + (hi - first) / 2;
        SourceRecord* const s = sibling(mid);
        if (CompareRows(s->Item.get(), s->Shape, item, shape) < 0)
        {
            first = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    uint32_t last = first;
    hi = size;
    while (last < hi)
    {
        uint32_t const mid = last + (hi - last) / 2;
        SourceRecord* const s = sibling(mid);
        if (CompareRows(item, shape, s->Item.get(), s->Shape) < 0)
        {
            hi = mid;
        }
        else
        {
            last = mid + 1;
        }
    }
    if (first == last)
    {
        return first;
    }

    // Equal keys keep source order: after the nearest earlier source sibling in the run.
    for (uint32_t v = first; v < last; ++v)
    {
        sibling(v)->Shape.Flags |= ShapeFlags::PlacementMark;
    }
    uint32_t result = first;
    for (uint32_t s = at; s-- > 0;)
    {
        SourceRecord* const prior = siblings[s];
        if ((prior->Shape.Flags & ShapeFlags::PlacementMark) != ShapeFlags::None)
        {
            result = virtualOf(prior->IndexInParent) + 1;
            break;
        }
    }
    for (uint32_t v = first; v < last; ++v)
    {
        sibling(v)->Shape.Flags &= ~ShapeFlags::PlacementMark;
    }
    return result;
}

uint32_t ShapingEngine::GroupPlacementIndex(GroupNode const* group, ShapedNode* parent, ShapedNode const* skip) const
{
    uint32_t const count = parent->ShapedChildCount();
    std::optional<uint32_t> const skipped = IndexInList(parent, skip);
    uint32_t const skipAt = skipped ? *skipped : count;
    uint32_t const size = skipped ? count - 1 : count;

    winrt::SortDirection const direction = m_applied.Groups[group->GroupLevel].Direction;
    if (direction == winrt::SortDirection::None)
    {
        return size;
    }

    winrt::IInspectable const key = group->Key ? group->Key->get() : nullptr;
    uint32_t lo = 0;
    uint32_t hi = size;
    while (lo < hi)
    {
        uint32_t const mid = lo + (hi - lo) / 2;
        GroupNode const* const other = parent->ShapedChildAt(mid >= skipAt ? mid + 1 : mid)->AsGroup();
        winrt::IInspectable const otherKey = other->Key ? other->Key->get() : nullptr;
        if (CompareKeys(key, otherKey, direction) < 0)
        {
            hi = mid;
        }
        else
        {
            lo = mid + 1;
        }
    }
    return lo;
}

void ShapingEngine::CommitInsert(StagedEdit& edit) noexcept
{
    auto const commit = [](StagedRecord& staged) noexcept
    {
        SourceRecord* const record = staged.Record;
        ShapeState& next = staged.Shape;
        record->Shape.Flags = (record->Shape.Flags & c_keptFlags) | next.Flags;
        if ((next.Flags & ShapeFlags::SortKeysValid) != ShapeFlags::None)
        {
            record->Shape.SortKeys = std::move(next.SortKeys);
        }
        else
        {
            record->Shape.SortKeys.clear();
        }
        record->Shape.ShapedChildren = std::move(next.ShapedChildren);
        record->Shape.Group = record->Parent ? nullptr : next.Group;
        record->ChildRows.MarkStale();

        if ((next.Flags & ShapeFlags::ChildrenShaped) != ShapeFlags::None)
        {
            auto const& shapedChildren = record->Shape.ShapedChildren;
            for (uint32_t j = 0; j < shapedChildren.size(); ++j)
            {
                shapedChildren[j]->IndexInParent = j;
            }
        }
        else
        {
            for (uint32_t j = 0; j < record->Children.size(); ++j)
            {
                record->Children[j]->IndexInParent = j;
            }
        }
    };

    commit(edit.Staged);
    for (StagedRecord& descendant : edit.Descendants)
    {
        commit(descendant);
    }
    for (StagedRecord& link : edit.Chain)
    {
        commit(link);
    }

    if (!edit.Parent)
    {
        return;
    }

    if (GroupNode* group = edit.Parent->AsGroup())
    {
        for (; !group->IsRoot(); group = group->Parent)
        {
            ++group->ItemCount;
        }
    }

    // The detached new group chain, bottom-up, then its map entries (capacity reserved).
    ShapedNode* top = edit.Chain.empty() ? edit.Staged.Record : edit.Chain.back().Record;
    for (size_t k = edit.NewGroupEntries.size(); k-- > 0;)
    {
        GroupNode* const group = edit.NewGroupEntries[k].mapped().get();
        group->Children.push_back(top);
        top->IndexInParent = 0;
        group->ItemCount = 1;
        group->ChildRows.MarkStale();
        top = group;
    }
    for (GroupMap::node_type& entry : edit.NewGroupEntries)
    {
        m_groupsByIdentity.insert(std::move(entry));
    }
    edit.NewGroupEntries.clear();

    m_tree.Link(top, edit.Parent, edit.ShapedIndex, false /* isMove */);
}

ShapingEngine::StagedLeave ShapingEngine::StageLeave(
    SourceRecord* record,
    SourceRecord* oldParent,
    uint32_t oldIndex,
    GroupNode const* keep)
{
    StagedLeave leave;
    leave.Record = record;
    if (!InShapedParent(*record))
    {
        return leave;
    }

    // E8: a failing ancestor whose only shaped child leaves goes with it.
    SourceRecord* top = record;
    SourceRecord* current = oldParent;
    while (current &&
           (current->Shape.Flags & (ShapeFlags::PassesFilter | ShapeFlags::ChildrenShaped)) == ShapeFlags::ChildrenShaped &&
           current->Shape.ShapedChildren.size() == 1)
    {
        leave.Chain.push_back(current);
        top = current;
        current = current->Parent;
    }

    leave.Top = top;
    if (top == record)
    {
        // A removed record is already out of its source list: a source-owned list (no filter or
        // sort) takes the old source index.
        leave.Parent = oldParent ? static_cast<ShapedNode*>(oldParent) : static_cast<ShapedNode*>(record->Shape.Group);
        bool const engineOwned = !oldParent || (oldParent->Shape.Flags & ShapeFlags::ChildrenShaped) != ShapeFlags::None;
        leave.Index = engineOwned ? record->IndexInParent : oldIndex;
    }
    else
    {
        leave.Parent = top->ShapedParent();
        leave.Index = top->IndexInParent;
    }

    // E9: a top-level record leaving takes every group it was the last member of, short of the
    // chain a replacing record keeps.
    if (!current && !m_applied.Groups.empty())
    {
        leave.Group = top->Shape.Group;
        auto const kept = [keep](GroupNode const* group)
        {
            for (GroupNode const* k = keep; k; k = k->Parent)
            {
                if (k == group)
                {
                    return true;
                }
            }
            return false;
        };

        size_t pruned = 0;
        for (GroupNode* group = leave.Group; group && !group->IsRoot() && group->ItemCount == 1 && !kept(group); group = group->Parent)
        {
            leave.TopGroup = group;
            ++pruned;
        }
        if (leave.TopGroup)
        {
            leave.Top = leave.TopGroup;
            leave.Parent = leave.TopGroup->Parent;
            leave.Index = leave.TopGroup->IndexInParent;
            m_retired.reserve(m_retired.size() + pruned);
        }
    }
    return leave;
}

void ShapingEngine::CommitLeave(StagedLeave& leave) noexcept
{
    if (leave.Top)
    {
        for (GroupNode* group = leave.Group; group && !group->IsRoot(); group = group->Parent)
        {
            --group->ItemCount;
        }

        m_tree.Unlink(leave.Top, leave.Parent, leave.Index, false /* isMove */);

        for (SourceRecord* const link : leave.Chain)
        {
            link->Shape.Flags &= ~ShapeFlags::SortKeysValid;
            link->Shape.SortKeys.clear();
            link->Shape.ShapedChildren.clear();
            link->Shape.Group = nullptr;
            link->ChildRows.MarkStale();
        }

        PruneEmptyGroups(leave.Group, leave.TopGroup);
    }
    leave.Record->Shape.Group = nullptr;
}

void ShapingEngine::PruneEmptyGroups(GroupNode* bottom, GroupNode* top) noexcept
{
    if (!top)
    {
        return;
    }

    for (GroupNode* group = bottom;; )
    {
        GroupNode* const up = group->Parent;
        auto entry = m_groupsByIdentity.extract(group->Identity);
        MUX_ASSERT(!entry.empty() && entry.mapped().get() == group);
        group->Children.clear();
        if (!entry.empty())
        {
            m_retired.push_back(std::move(entry.mapped()));
        }
        if (group == top)
        {
            break;
        }
        group = up;
    }
}

namespace
{
    void ClearShape(SourceRecord* record) noexcept
    {
        record->Shape.SortKeys.clear();
        record->Shape.ShapedChildren.clear();
        record->Shape.Group = nullptr;
        for (SourceRecord* const child : record->Children)
        {
            ClearShape(child);
        }
    }
}

void ShapingEngine::Clear()
{
    m_tree.Clear();
    m_groupsByIdentity.clear();
    m_retired.clear();

    for (SourceRecord* const record : m_source.TopLevel())
    {
        ClearShape(record);
    }

    m_applied = ShapeSpec{};
}

void ShapingEngine::ReleaseRetired() noexcept
{
    m_retired.clear();
}
