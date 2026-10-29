// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/ShapedTree.h"

namespace
{
    // F4: a group that leaves the tree (not a move) drops its expansion and its subgroups'.
    void EraseGroupExpansion(GroupExpansion& expansion, GroupNode* group) noexcept
    {
        expansion.Erase(group->Identity);
        for (ShapedNode* const child : group->Children)
        {
            if (GroupNode* const subgroup = child->AsGroup())
            {
                EraseGroupExpansion(expansion, subgroup);
            }
        }
    }
}

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

    if (m_cursorNode)
    {
        if (index == m_cursorIndex)
        {
            return m_cursorNode;
        }

        ShapedNode* step = nullptr;
        if (index == m_cursorIndex + 1)
        {
            step = NextShown(m_cursorNode);
        }
        else if (index + 1 == m_cursorIndex)
        {
            step = PreviousShown(m_cursorNode);
        }

        if (step)
        {
            m_cursorNode = step;
            m_cursorIndex = index;
            return step;
        }
    }

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

uint32_t ShapedTree::FlatIndexOf(ShapedNode* node) const
{
    MUX_ASSERT(IsShown(node));

    uint32_t index = 0;
    for (ShapedNode* current = node; ShapedNode* const parent = current->ShapedParent(); current = parent)
    {
        index += CurrentChildRows(parent).PrefixSum(current->IndexInParent) + (parent == &m_root ? 0 : 1);
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

void ShapedTree::SetAllRowsExpanded(bool expanded)
{
    // Compute: every record in the shaped tree (allocation is the only failure).
    std::vector<SourceRecord*> records;
    std::vector<ShapedNode*> pending{ &m_root };
    while (!pending.empty())
    {
        ShapedNode* const node = pending.back();
        pending.pop_back();
        uint32_t const count = node->ShapedChildCount();
        for (uint32_t j = 0; j < count; ++j)
        {
            ShapedNode* const child = node->ShapedChildAt(j);
            if (SourceRecord* const record = child->AsRecord())
            {
                records.push_back(record);
            }
            pending.push_back(child);
        }
    }

    // Write: nothing below throws.
    BeginReset();
    for (SourceRecord* const record : records)
    {
        SetExpanded(record, expanded);
    }

    ComputeSubtreeCounts(&m_root);
    InvalidateCursor();
    if (m_listener)
    {
        m_listener->OnRowsReset();
    }
}

void ShapedTree::SetAllGroupsExpanded(bool expanded)
{
    // Ungrouped: the baseline is kept for the next GroupBy, and no row changes.
    if (m_groupLevelCount == 0)
    {
        m_groupExpansion.SetAll(expanded);
        return;
    }

    BeginReset();
    m_groupExpansion.SetAll(expanded);
    ComputeSubtreeCounts(&m_root);
    InvalidateCursor();
    if (m_listener)
    {
        m_listener->OnRowsReset();
    }
}

bool ShapedTree::Expand(ShapedNode* node)
{
    if (!IsLinked(node) || node->ShapedChildCount() == 0 || IsExpanded(node))
    {
        return false;
    }

    // Compute: the children's counts are valid (P6); a stale list is rebuilt here.
    uint32_t const count = CurrentChildRows(node).Total();

    SetExpanded(node, true);
    node->VisibleDescendantCount = count;
    ApplyCountDelta(node, static_cast<int64_t>(count));
    InvalidateCursor();

    if (count > 0)
    {
        if (auto const shownAt = IndexOf(node))
        {
            RaiseInserted(*shownAt + 1, count, false);
        }
    }
    return true;
}

bool ShapedTree::Collapse(ShapedNode* node)
{
    if (!IsLinked(node) || node->ShapedChildCount() == 0 || !IsExpanded(node))
    {
        return false;
    }

    // Position first, while the rows are still counted. ChildRows is kept for the next Expand.
    auto const shownAt = IndexOf(node);
    uint32_t const count = node->VisibleDescendantCount;

    SetExpanded(node, false);
    node->VisibleDescendantCount = 0;
    ApplyCountDelta(node, -static_cast<int64_t>(count));
    InvalidateCursor();

    if (shownAt && count > 0)
    {
        RaiseRemoved(*shownAt + 1, count, false);
    }
    return true;
}

bool ShapedTree::ExpandSubtree(ShapedNode* node)
{
    if (!IsLinked(node) || node->IsGroup())
    {
        return false;
    }

    return ExpandSubtreeFrom(node);
}

bool ShapedTree::Reveal(ShapedNode* node)
{
    // Compute: the collapsed ancestors, bottom-up. A node whose walk does not reach this root is
    // not linked.
    std::vector<ShapedNode*> collapsed;
    ShapedNode* current = node;
    for (ShapedNode* parent = node->ShapedParent(); parent; parent = parent->ShapedParent())
    {
        if (parent != &m_root && !IsExpanded(parent))
        {
            collapsed.push_back(parent);
        }
        current = parent;
    }

    if (current != &m_root || node == &m_root || collapsed.empty())
    {
        return false;
    }

    // Every one but the topmost is hidden: flag and count only, no event (P6).
    for (size_t i = 0; i + 1 < collapsed.size(); ++i)
    {
        ShapedNode* const ancestor = collapsed[i];
        uint32_t const count = CurrentChildRows(ancestor).Total();
        SetExpanded(ancestor, true);
        ancestor->VisibleDescendantCount = count;
        ApplyCountDelta(ancestor, static_cast<int64_t>(count));
    }

    // The topmost is shown: one insert for every revealed row.
    Expand(collapsed.back());
    InvalidateCursor();
    return true;
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

void ShapedTree::Link(ShapedNode* node, ShapedNode* parent, uint32_t index, bool isMove) noexcept
{
    // An engine-owned list takes node here (capacity reserved by the engine, E4); a source-owned
    // list (a record's Children while ChildrenShaped is clear) already holds it.
    if (GroupNode* const group = parent->AsGroup())
    {
        MUX_ASSERT(index <= group->Children.size());
        group->Children.insert(group->Children.begin() + index, node);
    }
    else if (SourceRecord* const record = parent->AsRecord();
             (record->Shape.Flags & ShapeFlags::ChildrenShaped) != ShapeFlags::None)
    {
        auto& list = record->Shape.ShapedChildren;
        list.insert(list.begin() + index, node->AsRecord());
    }
    MUX_ASSERT(parent->ShapedChildAt(index) == node);

    Renumber(parent, index);
    parent->ChildRows.MarkStale();
    if (!isMove)
    {
        ComputeSubtreeCounts(node);
    }

    uint32_t const span = 1 + node->VisibleDescendantCount;
    PropagateCount(parent, static_cast<int64_t>(span));
    InvalidateCursor();

    // IndexOf rebuilds the stale lists on the way up, so a shown parent's ChildRows is current
    // again when the event is raised.
    if (auto const shownAt = IndexOf(node))
    {
        RaiseInserted(*shownAt, span, isMove);
    }
}

void ShapedTree::Unlink(ShapedNode* node, bool isMove) noexcept
{
    Unlink(node, node->ShapedParent(), node->IndexInParent, isMove);
}

void ShapedTree::Unlink(ShapedNode* node, ShapedNode* parent, uint32_t index, bool isMove) noexcept
{
    uint32_t const span = 1 + node->VisibleDescendantCount;

    // Position first. Only the rows of the siblings before index matter. A current ChildRows is
    // in the old order. A stale one is rebuilt from the list as it is now: SourceTable may have
    // erased node (remove, reparent: the siblings before index are unchanged) or moved it within
    // this source-owned list. In that last case the later siblings still carry their old
    // IndexInParent (Renumber has not run), so the sibling now at index tells the direction: one
    // that was at index - 1 means node moved before index, and its own row is counted in
    // PrefixSum(index + 1).
    std::optional<uint32_t> shownAt;
    bool const parentIsRoot = parent == &m_root;

    // The same correction applies higher up: a reparent into an ancestor of parent has already put
    // node into that ancestor's source-owned list, so when its sums are stale they are rebuilt with
    // node's span in the new place and the ancestor's child on the way to parent has shifted by
    // one (its IndexInParent is still the old one).
    auto const parentIndex = [&]() -> std::optional<uint32_t> {
        uint32_t flat = 0;
        ShapedNode* const newParent = node->ShapedParent();
        ShapedNode* current = parent;
        while (ShapedNode* const up = current->ShapedParent())
        {
            bool const upIsRoot = up == &m_root;
            if (!upIsRoot && !IsExpanded(up))
            {
                return std::nullopt;
            }

            bool const wasStale = up->ChildRows.IsStale();
            FenwickTree& rows = CurrentChildRows(up);
            uint32_t const at = current->IndexInParent;
            uint32_t before = rows.PrefixSum(at);
            if (wasStale && up == newParent && at + 1 < up->ShapedChildCount() && up->ShapedChildAt(at + 1) == current)
            {
                before = rows.PrefixSum(at + 1) - span;
            }
            flat += before + (upIsRoot ? 0 : 1);
            current = up;
        }

        if (current != &m_root)
        {
            return std::nullopt;
        }
        return flat;
    };
    std::optional<uint32_t> const parentAt = parentIsRoot ? std::optional<uint32_t>{ 0 } : parentIndex();
    if (parentAt && (parentIsRoot || IsExpanded(parent)))
    {
        bool const wasStale = parent->ChildRows.IsStale();
        FenwickTree& rows = CurrentChildRows(parent);
        uint32_t before = rows.PrefixSum(index);
        if (wasStale && index < parent->ShapedChildCount())
        {
            ShapedNode* const atIndex = parent->ShapedChildAt(index);
            if (atIndex != node && node->ShapedParent() == parent && atIndex->IndexInParent + 1 == index)
            {
                before = rows.PrefixSum(index + 1) - span;
            }
        }
        shownAt = *parentAt + (parentIsRoot ? 0 : 1) + before;
    }

    if (GroupNode* const group = parent->AsGroup())
    {
        MUX_ASSERT(index < group->Children.size() && group->Children[index] == node);
        group->Children.erase(group->Children.begin() + index);
    }
    else if (SourceRecord* const record = parent->AsRecord();
             (record->Shape.Flags & ShapeFlags::ChildrenShaped) != ShapeFlags::None)
    {
        auto& list = record->Shape.ShapedChildren;
        list.erase(list.begin() + index);
    }

    Renumber(parent, index);
    parent->ChildRows.MarkStale();
    PropagateCount(parent, -static_cast<int64_t>(span));
    if (!isMove)
    {
        if (GroupNode* const group = node->AsGroup())
        {
            EraseGroupExpansion(m_groupExpansion, group);
        }
    }
    InvalidateCursor();

    if (shownAt)
    {
        RaiseRemoved(*shownAt, span, isMove);
    }
}

void ShapedTree::PropagateCount(ShapedNode* parent, int64_t delta) noexcept
{
    // The caller has updated (or marked stale) parent's own ChildRows for the changed child.
    ShapedNode* node = parent;
    while (node)
    {
        // A collapsed node keeps 0 and hides the change from everything above it.
        if (node != &m_root && !IsExpanded(node))
        {
            return;
        }

        node->VisibleDescendantCount = static_cast<uint32_t>(static_cast<int64_t>(node->VisibleDescendantCount) + delta);
        ShapedNode* const up = node->ShapedParent();
        if (up && !up->ChildRows.IsStale())
        {
            up->ChildRows.Add(node->IndexInParent, static_cast<int32_t>(delta));
        }
        node = up;
    }
}

void ShapedTree::Renumber(ShapedNode* parent, uint32_t index) noexcept
{
    uint32_t const count = parent->ShapedChildCount();
    for (uint32_t j = index; j < count; ++j)
    {
        parent->ShapedChildAt(j)->IndexInParent = j;
    }
}

void ShapedTree::SetExpanded(ShapedNode* node, bool expanded)
{
    if (GroupNode* const group = node->AsGroup())
    {
        m_groupExpansion.SetExpanded(group->Identity, expanded);
        return;
    }

    auto& flags = node->AsRecord()->Shape.Flags;
    if ((flags & ShapeFlags::PassesFilter) != ShapeFlags::None)
    {
        flags = expanded ? (flags | ShapeFlags::Expanded) : (flags & ~ShapeFlags::Expanded);
    }
    else
    {
        // A context row (F5): expanded unless OverlayCollapsed.
        flags = expanded ? (flags & ~ShapeFlags::OverlayCollapsed) : (flags | ShapeFlags::OverlayCollapsed);
    }
}

bool ShapedTree::IsLinked(ShapedNode* node) const noexcept
{
    if (!node || node == &m_root)
    {
        return false;
    }

    ShapedNode* current = node;
    while (ShapedNode* const parent = current->ShapedParent())
    {
        current = parent;
    }
    return current == &m_root;
}

void ShapedTree::ApplyCountDelta(ShapedNode* node, int64_t delta) noexcept
{
    if (delta == 0)
    {
        return;
    }

    ShapedNode* const parent = node->ShapedParent();
    if (!parent->ChildRows.IsStale())
    {
        parent->ChildRows.Add(node->IndexInParent, static_cast<int32_t>(delta));
    }
    PropagateCount(parent, delta);
}

bool ShapedTree::ExpandSubtreeFrom(ShapedNode* node)
{
    if (!IsExpanded(node))
    {
        if (node->ShapedChildCount() == 0)
        {
            // A leaf has no rows to show; its flag is set so it opens once it has children.
            SetExpanded(node, true);
            return true;
        }

        // The topmost collapsed node of this branch: flags over its subtree, then one insert for
        // the revealed rows (contiguous after node).
        std::vector<ShapedNode*> nodes{ node };
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            ShapedNode* const current = nodes[i];
            uint32_t const childCount = current->ShapedChildCount();
            for (uint32_t j = 0; j < childCount; ++j)
            {
                nodes.push_back(current->ShapedChildAt(j));
            }
        }
        auto const shownAt = IndexOf(node);

        for (ShapedNode* const current : nodes)
        {
            SetExpanded(current, true);
        }
        ComputeSubtreeCounts(node);
        uint32_t const count = node->VisibleDescendantCount;
        ApplyCountDelta(node, static_cast<int64_t>(count));
        InvalidateCursor();

        if (shownAt && count > 0)
        {
            RaiseInserted(*shownAt + 1, count, false);
        }
        return true;
    }

    // Already expanded: its shown rows stay; each collapsed branch below raises its own insert.
    bool changed = false;
    uint32_t const childCount = node->ShapedChildCount();
    for (uint32_t j = 0; j < childCount; ++j)
    {
        changed = ExpandSubtreeFrom(node->ShapedChildAt(j)) || changed;
    }
    return changed;
}

ShapedNode* ShapedTree::NextShown(ShapedNode* node) const noexcept
{
    if (node->ShapedChildCount() > 0 && IsExpanded(node))
    {
        return node->ShapedChildAt(0);
    }

    for (ShapedNode* current = node; current != &m_root;)
    {
        ShapedNode* const parent = current->ShapedParent();
        if (!parent)
        {
            return nullptr;
        }

        if (current->IndexInParent + 1 < parent->ShapedChildCount())
        {
            return parent->ShapedChildAt(current->IndexInParent + 1);
        }
        current = parent;
    }
    return nullptr;
}

ShapedNode* ShapedTree::PreviousShown(ShapedNode* node) const noexcept
{
    ShapedNode* const parent = node->ShapedParent();
    if (!parent)
    {
        return nullptr;
    }

    if (node->IndexInParent == 0)
    {
        return parent == &m_root ? nullptr : parent;
    }

    ShapedNode* previous = parent->ShapedChildAt(node->IndexInParent - 1);
    while (uint32_t const count = previous->ShapedChildCount())
    {
        if (!IsExpanded(previous))
        {
            break;
        }
        previous = previous->ShapedChildAt(count - 1);
    }
    return previous;
}

void ShapedTree::RaiseRemoved(uint32_t index, uint32_t count, bool isMove) const noexcept
{
    if (m_listener)
    {
        m_listener->OnRowsRemoved(index, count, isMove);
    }
}

void ShapedTree::RaiseInserted(uint32_t index, uint32_t count, bool isMove) const noexcept
{
    if (m_listener)
    {
        m_listener->OnRowsInserted(index, count, isMove);
    }
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
