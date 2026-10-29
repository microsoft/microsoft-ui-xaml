// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/ShapingEngine.h"

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

void ShapingEngine::Reapply(ShapeSpec const& desired)
{
    if (!(desired == ShapeSpec{}))
    {
        // Filter, Sort and GroupBy land in Milestone 4; every verb throws before reaching here.
        throw winrt::hresult_not_implemented(L"TableViewSource2: shaping a non-empty spec is not implemented yet.");
    }

    // Compute: the empty spec places the top-level records under the root in source order, and
    // every record's source children (SourceRecord::Children, ParentBy / ChildrenBy) under it.
    auto const& topLevel = m_source.TopLevel();
    std::vector<ShapedNode*> rootChildren;
    rootChildren.reserve(topLevel.size());
    std::vector<SourceRecord*> records(topLevel.begin(), topLevel.end());
    for (SourceRecord* const record : topLevel)
    {
        rootChildren.push_back(record);
    }
    for (size_t i = 0; i < records.size(); ++i)
    {
        records.insert(records.end(), records[i]->Children.begin(), records[i]->Children.end());
    }
    GroupMap noGroups;

    // Write: nothing below throws.
    m_tree.BeginReset();

    GroupNode& root = m_tree.Root();
    for (SourceRecord* const record : records)
    {
        // Keep the record's own expansion; everything else is recomputed for the empty spec.
        // Without ChildrenShaped, ShapedChildAt reads the source Children.
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

    for (uint32_t i = 0; i < topLevel.size(); ++i)
    {
        topLevel[i]->IndexInParent = i;
    }
    root.Children = std::move(rootChildren);
    root.ChildRows.MarkStale();

    // The replaced groups stay alive until ReleaseRetired (P5).
    m_groupsByIdentity.swap(noGroups);
    for (auto& entry : noGroups)
    {
        m_retired.push_back(std::move(entry.second));
    }

    m_applied = desired;
    m_tree.EndReset(0, m_groupsByIdentity, false /* newGroupSelector */);
}

void ShapingEngine::ApplySourceChange(SourceChange const& change)
{
    // Spec §6: under the empty spec a single-item Add, Remove or Move (including a ParentBy
    // reparent and an orphan moving to the top level) links or unlinks that one node (drift #20).
    // Replace, and any non-empty spec (Milestone 4), rebuild with the applied spec (one Reset).
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
