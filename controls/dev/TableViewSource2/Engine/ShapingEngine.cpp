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

    // Compute: the empty spec places the top-level records under the root in source order. Flat
    // sources have no children; Milestone 3 extends this to source children.
    auto const& topLevel = m_source.TopLevel();
    std::vector<ShapedNode*> rootChildren;
    rootChildren.reserve(topLevel.size());
    for (SourceRecord* const record : topLevel)
    {
        rootChildren.push_back(record);
    }
    GroupMap noGroups;

    // Write: nothing below throws.
    m_tree.BeginReset();

    GroupNode& root = m_tree.Root();
    for (uint32_t i = 0; i < topLevel.size(); ++i)
    {
        SourceRecord* const record = topLevel[i];
        // Keep the record's own expansion; everything else is recomputed for the empty spec.
        record->Shape.Flags = (record->Shape.Flags & (ShapeFlags::Expanded | ShapeFlags::OverlayCollapsed)) | ShapeFlags::PassesFilter;
        record->Shape.SortKeys.clear();
        record->Shape.ShapedChildren.clear();
        record->Shape.Group = &root;
        record->IndexInParent = i;
        record->ChildRows.MarkStale();
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

void ShapingEngine::Clear()
{
    m_tree.Clear();
    m_groupsByIdentity.clear();
    m_retired.clear();

    for (SourceRecord* const record : m_source.TopLevel())
    {
        record->Shape.SortKeys.clear();
        record->Shape.ShapedChildren.clear();
        record->Shape.Group = nullptr;
    }

    m_applied = ShapeSpec{};
}

void ShapingEngine::ReleaseRetired() noexcept
{
    m_retired.clear();
}
