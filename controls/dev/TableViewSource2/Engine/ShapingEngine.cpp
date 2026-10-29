// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/ShapingEngine.h"
#include "ShapingHelpers.h"

#include <algorithm>
#include <string>

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

void ShapingEngine::Rebuild(ShapeSpec const& desired, ShapeDelta delta, std::span<LiveChange const> only)
{
    // Live re-reads (`only`) arrive with live shaping (Milestone 6).
    MUX_ASSERT(only.empty());
    (void)only;

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

    // 1. Filter. A predicate's exception propagates with nothing written.
    std::vector<uint8_t> passes(n, 1);
    if (filterActive)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            if (refilter)
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
            bool const cached = !rereadSorts &&
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
            bool const cached = !rereadGroups && cachedGroup && !cachedGroup->IsRoot() &&
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

    // ---- Write: nothing below throws. ----
    m_tree.BeginReset();

    ShapeFlags const kept = ShapeFlags::Expanded | ShapeFlags::PlacementMark |
        (clearOverlay ? ShapeFlags::None : ShapeFlags::OverlayCollapsed);
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

    m_applied = std::move(appliedNext);
    m_tree.EndReset(levels, m_groupsByIdentity, newGroupSelector);
}

void ShapingEngine::ApplySourceChange(SourceChange const& change)
{
    // Spec §6: under the empty spec a single-item Add, Remove or Move (including a ParentBy
    // reparent and an orphan moving to the top level) links or unlinks that one node (drift #20).
    // Replace, and any non-empty spec, rebuild with the applied spec (one Reset).
    bool const surgical = m_applied == ShapeSpec{} &&
        (change.Kind == SourceChangeKind::Add || change.Kind == SourceChangeKind::Remove || change.Kind == SourceChangeKind::Move);
    if (!surgical)
    {
        ShapeSpec const applied{ m_applied };
        Reapply(applied);
        return;
    }

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
