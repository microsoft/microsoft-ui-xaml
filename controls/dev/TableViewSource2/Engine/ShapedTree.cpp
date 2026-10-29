// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/ShapedTree.h"

void ShapedTree::SetListener(IShapedRowsListener* listener) noexcept
{
    m_listener = listener;
}

GroupNode& ShapedTree::Root() noexcept
{
    return m_root;
}

GroupNode const& ShapedTree::Root() const noexcept
{
    return m_root;
}

uint16_t ShapedTree::GroupLevelCount() const noexcept
{
    return m_groupLevelCount;
}

uint32_t ShapedTree::Size() const noexcept
{
    // The root is always expanded, so its count is the visible list's length.
    return m_root.VisibleDescendantCount;
}

ShapedNode* ShapedTree::GetAt(uint32_t index) const
{
    MUX_ASSERT(index < Size());

    if (m_cursorNode && index == m_cursorIndex)
    {
        return m_cursorNode;
    }

    // Milestone 7 adds the index +/- 1 step through NextShown / PreviousShown.
    ShapedNode* node = const_cast<GroupNode*>(&m_root);
    uint32_t r = index;
    for (;;)
    {
        auto const hit = CurrentChildRows(node).Find(r);
        ShapedNode* const child = node->ShapedChildAt(hit.Child);
        r -= hit.RowsBefore;
        if (r == 0)
        {
            m_cursorNode = child;
            m_cursorIndex = index;
            return child;
        }

        // Skip the child's own row and descend into its rows.
        r -= 1;
        node = child;
    }
}

std::optional<uint32_t> ShapedTree::IndexOf(ShapedNode* node) const
{
    if (!node || node == &m_root)
    {
        return std::nullopt;
    }

    uint32_t index = 0;
    ShapedNode* current = node;
    while (ShapedNode* const parent = current->ShapedParent())
    {
        bool const parentIsRoot = parent == &m_root;
        if (!parentIsRoot && !IsExpanded(parent))
        {
            return std::nullopt;
        }

        index += CurrentChildRows(parent).PrefixSum(current->IndexInParent);
        if (!parentIsRoot)
        {
            index += 1;
        }
        current = parent;
    }

    // A node whose upward walk does not end at this root is not linked.
    if (current != &m_root)
    {
        return std::nullopt;
    }

    return index;
}

bool ShapedTree::IsShown(ShapedNode* node) const
{
    for (ShapedNode* parent = node->ShapedParent(); parent && parent != &m_root; parent = parent->ShapedParent())
    {
        if (!IsExpanded(parent))
        {
            return false;
        }
    }

    return true;
}

bool ShapedTree::IsExpanded(ShapedNode* node) const
{
    if (node == &m_root)
    {
        return true;
    }

    if (auto const group = node->AsGroup())
    {
        return m_groupExpansion.IsExpanded(group->Identity);
    }

    auto const flags = node->AsRecord()->Shape.Flags;
    bool const passesFilter = (flags & ShapeFlags::PassesFilter) != ShapeFlags::None;
    if (!passesFilter)
    {
        // A context row (in the shaped tree, failing the filter) shows expanded unless collapsed
        // through the overlay (F5).
        return (flags & ShapeFlags::OverlayCollapsed) == ShapeFlags::None;
    }

    return (flags & ShapeFlags::Expanded) != ShapeFlags::None;
}

uint32_t ShapedTree::ShownDescendantCount(ShapedNode* node) const
{
    return node->VisibleDescendantCount;
}

void ShapedTree::BeginReset() noexcept
{
    InvalidateCursor();
    if (m_listener)
    {
        m_listener->OnRowsResetting();
    }
}

void ShapedTree::EndReset(
    uint16_t groupLevelCount,
    GroupMap const& liveGroups,
    bool newGroupSelector) noexcept
{
    if (newGroupSelector)
    {
        m_groupExpansion.Clear();
    }
    else
    {
        m_groupExpansion.RetainOnly(liveGroups);
    }

    m_groupLevelCount = groupLevelCount;
    ComputeSubtreeCounts(&m_root);
    InvalidateCursor();

    if (m_listener)
    {
        m_listener->OnRowsReset();
    }
}

void ShapedTree::Clear() noexcept
{
    m_root.Children.clear();
    m_root.ChildRows.Clear();
    m_root.VisibleDescendantCount = 0;
    m_groupLevelCount = 0;
    m_groupExpansion.Clear();
    m_listener = nullptr;
    InvalidateCursor();
}

void ShapedTree::RebuildChildRows(ShapedNode* node) const
{
    node->ChildRows.Build(node->ShapedChildCount(), [node](uint32_t j) {
        return 1 + node->ShapedChildAt(j)->VisibleDescendantCount;
    });
}

FenwickTree& ShapedTree::CurrentChildRows(ShapedNode* node) const
{
    if (node->ChildRows.IsStale())
    {
        RebuildChildRows(node);
    }

    return node->ChildRows;
}

void ShapedTree::ComputeSubtreeCounts(ShapedNode* node) noexcept
{
    uint32_t const childCount = node->ShapedChildCount();
    for (uint32_t j = 0; j < childCount; ++j)
    {
        ComputeSubtreeCounts(node->ShapedChildAt(j));
    }

    if (childCount > 0 && IsExpanded(node))
    {
        RebuildChildRows(node);
        node->VisibleDescendantCount = node->ChildRows.Total();
    }
    else
    {
        // A collapsed list stays stale until it is read (P7).
        node->ChildRows.MarkStale();
        node->VisibleDescendantCount = 0;
    }
}

void ShapedTree::InvalidateCursor() const noexcept
{
    m_cursorNode = nullptr;
    m_cursorIndex = 0;
}
