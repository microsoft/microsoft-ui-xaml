// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/ShapedTree.h"

namespace
{
    bool HasFlag(SourceRecord const& record, ShapeFlags flag) noexcept
    {
        return (record.Shape.Flags & flag) != ShapeFlags::None;
    }

    void WriteFlag(SourceRecord& record, ShapeFlags flag, bool value) noexcept
    {
        if (value)
        {
            record.Shape.Flags |= flag;
        }
        else
        {
            record.Shape.Flags &= ~flag;
        }
    }

    // In the shaped tree but failing the filter: shown only as an ancestor of a match (F5).
    bool IsContextRow(SourceRecord const& record) noexcept
    {
        return !HasFlag(record, ShapeFlags::PassesFilter);
    }

    // The parent's shaped child list is one the engine owns (a group's Children, or a record's
    // ShapedChildren), so Link and Unlink edit it; otherwise it is SourceRecord::Children, which
    // SourceTable has already edited.
    bool IsEngineOwnedList(ShapedNode const* parent) noexcept
    {
        auto const record = parent->AsRecord();
        return !record || HasFlag(*record, ShapeFlags::ChildrenShaped);
    }

    // Next group after g in a pre-order walk of top's group subtree, or null when the walk leaves
    // it. A group's Children are all groups or all records, never mixed, so a group's first child
    // tells which. Uses Parent and IndexInParent, so it allocates nothing.
    GroupNode* NextGroupInSubtree(GroupNode* g, GroupNode const* top) noexcept
    {
        if (!g->Children.empty() && g->Children.front()->IsGroup())
        {
            return g->Children.front()->AsGroup();
        }

        while (g != top)
        {
            auto const parent = g->Parent;
            auto const next = g->IndexInParent + 1;
            if (next < parent->Children.size())
            {
                return parent->Children[next]->AsGroup();
            }

            g = parent;
        }

        return nullptr;
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
    // The root is always expanded, so its count is its ChildRows total, kept current even while
    // that list is stale.
    return m_root.VisibleDescendantCount;
}

ShapedNode* ShapedTree::GetAt(uint32_t index) const
{
    MUX_ASSERT(index < Size());

    ShapedNode* result = nullptr;
    if (m_cursorNode)
    {
        if (index == m_cursorIndex)
        {
            result = m_cursorNode;
        }
        else if (index == m_cursorIndex + 1)
        {
            result = NextShown(m_cursorNode);
        }
        else if (index + 1 == m_cursorIndex)
        {
            result = PreviousShown(m_cursorNode);
        }
    }

    if (!result)
    {
        ShapedNode* node = const_cast<GroupNode*>(&m_root);
        uint32_t r = index;
        for (;;)
        {
            auto const hit = CurrentChildRows(node).Find(r);
            auto const child = node->ShapedChildAt(hit.Child);
            r -= hit.RowsBefore;
            if (r == 0)
            {
                result = child;
                break;
            }

            // Skip the child's own row; the rest of r falls among its descendants.
            r -= 1;
            node = child;
        }
    }

    m_cursorNode = result;
    m_cursorIndex = index;
    return result;
}

std::optional<uint32_t> ShapedTree::IndexOf(ShapedNode* node) const
{
    if (node == &m_root)
    {
        return std::nullopt;
    }

    uint32_t index = 0;
    for (auto n = node; n != &m_root;)
    {
        auto const parent = n->ShapedParent();
        if (n == m_unlinked || !parent)
        {
            return std::nullopt;
        }

        bool const parentIsRoot = parent == &m_root;
        if (!parentIsRoot && !IsExpanded(parent))
        {
            return std::nullopt;
        }

        index += CurrentChildRows(parent).PrefixSum(n->IndexInParent) + (parentIsRoot ? 0 : 1);
        n = parent;
    }

    return index;
}

bool ShapedTree::IsShown(ShapedNode* node) const
{
    if (node == &m_root)
    {
        return false;
    }

    for (auto n = node; n != &m_root;)
    {
        auto const parent = n->ShapedParent();
        if (n == m_unlinked || !parent || (parent != &m_root && !IsExpanded(parent)))
        {
            return false;
        }

        n = parent;
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

    auto const record = node->AsRecord();
    return IsContextRow(*record) ? !HasFlag(*record, ShapeFlags::OverlayCollapsed) : HasFlag(*record, ShapeFlags::Expanded);
}

uint32_t ShapedTree::ShownDescendantCount(ShapedNode* node) const
{
    return node->VisibleDescendantCount;
}

uint32_t ShapedTree::LinkedChildCount(ShapedNode* node) const noexcept
{
    auto const count = node->ShapedChildCount();
    return m_unlinked && m_unlinked->ShapedParent() == node ? count - 1 : count;
}

uint32_t ShapedTree::PositionInParent(ShapedNode* node) const noexcept
{
    MUX_ASSERT(node != m_unlinked);

    // Unlink renumbered the list holding m_unlinked, so both positions are current.
    auto const index = node->IndexInParent;
    bool const after = m_unlinked && m_unlinked->ShapedParent() == node->ShapedParent() && m_unlinked->IndexInParent < index;
    return after ? index - 1 : index;
}

void ShapedTree::Link(ShapedNode* node, ShapedNode* parent, uint32_t index, bool isMove) noexcept
{
    MUX_ASSERT(!m_unlinked || m_unlinked == node);
    if (m_unlinked == node)
    {
        m_unlinked = nullptr;
    }

    if (IsEngineOwnedList(parent))
    {
        // Capacity was reserved in the engine's compute phase (E4), so the insert cannot throw.
        if (auto const group = parent->AsGroup())
        {
            group->Children.insert(group->Children.begin() + index, node);
        }
        else
        {
            MUX_ASSERT(node->AsRecord());
            auto& children = parent->AsRecord()->Shape.ShapedChildren;
            children.insert(children.begin() + index, node->AsRecord());
        }
    }

    MUX_ASSERT(parent->ShapedChildAt(index) == node);
    MUX_ASSERT(node->ShapedParent() == parent);

    Renumber(parent, index);
    parent->ChildRows.MarkStale();

    if (!isMove)
    {
        ComputeSubtreeCounts(node);
    }

    auto const span = 1 + node->VisibleDescendantCount;
    PropagateCount(parent, span);
    InvalidateCursor();

    if (auto const flatIndex = IndexOf(node))
    {
        RaiseInserted(*flatIndex, span, isMove);
    }
}

void ShapedTree::Unlink(ShapedNode* node, bool isMove) noexcept
{
    Unlink(node, node->ShapedParent(), node->IndexInParent, isMove);
}

void ShapedTree::Unlink(ShapedNode* node, ShapedNode* parent, uint32_t index, bool isMove) noexcept
{
    auto const span = 1 + node->VisibleDescendantCount;

    // Where node's rows were, read before anything moves: the rows of the siblings before node's
    // old slot at every level. A current ChildRows is still in the old order. A stale one is
    // rebuilt from its list as it is now, and SourceTable may already have put node into a
    // source-owned list on this path: moved within parent's list, or reparented into an
    // ancestor's. Renumber has not run there, so the siblings keep their old IndexInParent and a
    // shift is visible: if the child expected at slot now sits one later, node was inserted before
    // it and its span is subtracted. A node erased from the list (remove, reparent elsewhere)
    // leaves the siblings before slot unchanged.
    auto const rowsBefore = [&](ShapedNode* owner, uint32_t slot, bool shifted) {
        bool const wasStale = owner->ChildRows.IsStale();
        auto& rows = CurrentChildRows(owner);
        return wasStale && shifted ? rows.PrefixSum(slot + 1) - span : rows.PrefixSum(slot);
    };

    std::optional<uint32_t> flatIndex;
    if (parent == &m_root || IsExpanded(parent))
    {
        // In parent's own list the expected child is node; a shift shows as the sibling now at
        // index being the one that was at index - 1.
        bool const parentShifted =
            index > 0 && index < parent->ShapedChildCount() && parent->ShapedChildAt(index)->IndexInParent + 1 == index;
        uint32_t flat = rowsBefore(parent, index, parentShifted) + (parent == &m_root ? 0 : 1);

        bool shown = true;
        for (auto n = parent; n != &m_root;)
        {
            auto const up = n->ShapedParent();
            if (!up || (up != &m_root && !IsExpanded(up)))
            {
                shown = false;
                break;
            }

            auto const at = n->IndexInParent;
            bool const shifted = at + 1 < up->ShapedChildCount() && up->ShapedChildAt(at) != n && up->ShapedChildAt(at + 1) == n;
            flat += rowsBefore(up, at, shifted) + (up == &m_root ? 0 : 1);
            n = up;
        }

        if (shown)
        {
            flatIndex = flat;
        }
    }

    if (IsEngineOwnedList(parent))
    {
        if (auto const group = parent->AsGroup())
        {
            MUX_ASSERT(group->Children[index] == node);
            group->Children.erase(group->Children.begin() + index);
        }
        else
        {
            auto& children = parent->AsRecord()->Shape.ShapedChildren;
            MUX_ASSERT(children[index] == node);
            children.erase(children.begin() + index);
        }
    }

    Renumber(parent, index);
    parent->ChildRows.MarkStale();
    PropagateCount(parent, -static_cast<int64_t>(span));

    // Drift #90: SourceTable already put node into a source-owned list (moved within parent's, or
    // reparented). Until its Link, node is m_unlinked: 0 rows in that list and not shown. The
    // list is renumbered whole (a move to an earlier slot shifted siblings before index) and
    // marked stale (its ChildRows may predate node's arrival, or was just rebuilt above with it).
    // A group or a top-level record always sits in an engine-owned list, so only a nested record
    // can be here; a removed record has no parent.
    if (auto const now = node->IsGroup() ? nullptr : node->ShapedParent(); now && !IsEngineOwnedList(now))
    {
        MUX_ASSERT(!m_unlinked);
        m_unlinked = node;
        Renumber(now, 0);
        now->ChildRows.MarkStale();
    }

    // F4: a dropped group's expansion must not outlive it (its identity can be reused).
    if (!isMove)
    {
        if (auto const top = node->AsGroup())
        {
            for (auto g = top; g; g = NextGroupInSubtree(g, top))
            {
                m_groupExpansion.Erase(g->Identity);
            }
        }
    }

    InvalidateCursor();

    if (flatIndex)
    {
        RaiseRemoved(*flatIndex, span, isMove);
    }
}

void ShapedTree::BeginReset() noexcept
{
    InvalidateCursor();

    if (m_listener)
    {
        m_listener->OnRowsResetting();
    }
}

void ShapedTree::EndReset(uint16_t groupLevelCount, GroupMap const& liveGroups, bool newGroupSelector) noexcept
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

    // Defensive: a rebuilt tree has no pending Link (#90).
    m_unlinked = nullptr;
    ComputeSubtreeCounts(&m_root);
    InvalidateCursor();

    if (m_listener)
    {
        m_listener->OnRowsReset();
    }
}

bool ShapedTree::Expand(ShapedNode* node)
{
    if (node == &m_root || node->ShapedChildCount() == 0 || !IsLinked(node) || IsExpanded(node))
    {
        return false;
    }

    SetExpanded(node, true);
    auto const count = CurrentChildRows(node).Total();
    node->VisibleDescendantCount = count;
    ApplyCountDelta(node, count);
    InvalidateCursor();

    // Count can be 0 while node's only child is m_unlinked (#90); its Link raises the row.
    if (IsShown(node) && count > 0)
    {
        RaiseInserted(FlatIndexOf(node) + 1, count, false);
    }

    return true;
}

bool ShapedTree::Collapse(ShapedNode* node)
{
    if (node == &m_root || node->ShapedChildCount() == 0 || !IsLinked(node) || !IsExpanded(node))
    {
        return false;
    }

    // Read the position first: the count change below does not move node, but the cursor and the
    // event must describe the rows as they were.
    bool const shown = IsShown(node);
    uint32_t const flatIndex = shown ? FlatIndexOf(node) : 0;
    auto const count = node->VisibleDescendantCount;

    // ChildRows is kept for the next Expand.
    SetExpanded(node, false);
    node->VisibleDescendantCount = 0;
    ApplyCountDelta(node, -static_cast<int64_t>(count));
    InvalidateCursor();

    if (shown && count > 0)
    {
        RaiseRemoved(flatIndex + 1, count, false);
    }

    return true;
}

bool ShapedTree::ExpandSubtree(ShapedNode* node)
{
    MUX_ASSERT(!node->IsGroup());
    if (node->IsGroup() || !IsLinked(node))
    {
        return false;
    }

    bool const changed = ExpandSubtreeFrom(node);
    if (changed)
    {
        InvalidateCursor();
    }

    return changed;
}

bool ShapedTree::Reveal(ShapedNode* node)
{
    if (node == &m_root || !IsLinked(node))
    {
        return false;
    }

    // Collapsed ancestors, bottom-up.
    std::vector<ShapedNode*> collapsed;
    for (auto p = node->ShapedParent(); p != &m_root; p = p->ShapedParent())
    {
        if (!IsExpanded(p))
        {
            collapsed.push_back(p);
        }
    }

    if (collapsed.empty())
    {
        return false;
    }

    // Every one but the topmost stays hidden: flag and count only. Each propagation stops at the
    // next collapsed ancestor, so the topmost's ChildRows already includes the rows revealed below.
    for (auto const ancestor : collapsed)
    {
        SetExpanded(ancestor, true);
        auto const count = CurrentChildRows(ancestor).Total();
        ancestor->VisibleDescendantCount = count;
        ApplyCountDelta(ancestor, count);
    }

    InvalidateCursor();

    // The topmost collapsed ancestor has only expanded ancestors, so it is shown.
    auto const topmost = collapsed.back();
    MUX_ASSERT(IsShown(topmost));
    RaiseInserted(FlatIndexOf(topmost) + 1, topmost->VisibleDescendantCount, false);
    return true;
}

void ShapedTree::SetAllGroupsExpanded(bool expanded)
{
    if (m_groupLevelCount == 0)
    {
        // No group rows: only the baseline for a later GroupBy changes (drift #53).
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

void ShapedTree::SetAllRowsExpanded(bool expanded)
{
    // Compute: every record in the shaped tree, leaves included, so a later first child shows at
    // once. Allocation is the only failure, and it happens before anything is written or raised.
    std::vector<SourceRecord*> records;
    std::vector<ShapedNode*> pending{ &m_root };
    while (!pending.empty())
    {
        auto const node = pending.back();
        pending.pop_back();

        for (uint32_t j = 0, count = node->ShapedChildCount(); j < count; ++j)
        {
            auto const child = node->ShapedChildAt(j);
            if (auto const record = child->AsRecord())
            {
                records.push_back(record);
            }

            pending.push_back(child);
        }
    }

    // Write: nothing below throws.
    BeginReset();
    for (auto const record : records)
    {
        WriteFlag(*record, ShapeFlags::Expanded, expanded);
        if (IsContextRow(*record))
        {
            WriteFlag(*record, ShapeFlags::OverlayCollapsed, !expanded);
        }
    }

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
    InvalidateCursor();
    m_listener = nullptr;
    m_unlinked = nullptr;
}

void ShapedTree::SetExpanded(ShapedNode* node, bool expanded)
{
    MUX_ASSERT(node != &m_root);

    if (auto const group = node->AsGroup())
    {
        m_groupExpansion.SetExpanded(group->Identity, expanded);
        return;
    }

    auto const record = node->AsRecord();
    if (IsContextRow(*record))
    {
        // F5: Expanded keeps the user's state for when the filter is cleared.
        WriteFlag(*record, ShapeFlags::OverlayCollapsed, !expanded);
    }
    else
    {
        WriteFlag(*record, ShapeFlags::Expanded, expanded);
    }
}

bool ShapedTree::IsLinked(ShapedNode* node) const noexcept
{
    if (node == &m_root)
    {
        return false;
    }

    for (auto n = node; n != &m_root;)
    {
        auto const parent = n->ShapedParent();
        if (n == m_unlinked || !parent || n->IndexInParent >= parent->ShapedChildCount() || parent->ShapedChildAt(n->IndexInParent) != n)
        {
            return false;
        }

        n = parent;
    }

    return true;
}

void ShapedTree::ApplyCountDelta(ShapedNode* node, int64_t delta) noexcept
{
    if (delta == 0)
    {
        return;
    }

    auto const parent = node->ShapedParent();
    if (!parent->ChildRows.IsStale())
    {
        parent->ChildRows.Add(node->IndexInParent, static_cast<int32_t>(delta));
    }

    PropagateCount(parent, delta);
}

bool ShapedTree::ExpandSubtreeFrom(ShapedNode* node)
{
    bool changed = false;

    // Pre-order over node's expanded part. A collapsed node starts a branch: its whole subtree is
    // flagged and counted, and its revealed rows are one contiguous insert after it.
    std::vector<ShapedNode*> pending{ node };
    std::vector<ShapedNode*> branch;
    while (!pending.empty())
    {
        auto const top = pending.back();
        pending.pop_back();

        if (IsExpanded(top))
        {
            for (uint32_t j = top->ShapedChildCount(); j > 0; --j)
            {
                if (auto const child = top->ShapedChildAt(j - 1); child != m_unlinked)
                {
                    pending.push_back(child);
                }
            }

            continue;
        }

        // Compute: the branch's nodes, so an allocation failure leaves this branch untouched
        // (branches already done stay consistent).
        branch.assign(1, top);
        for (size_t i = 0; i < branch.size(); ++i)
        {
            auto const n = branch[i];
            for (uint32_t j = 0, count = n->ShapedChildCount(); j < count; ++j)
            {
                if (auto const child = n->ShapedChildAt(j); child != m_unlinked)
                {
                    branch.push_back(child);
                }
            }
        }

        // Write: flag every record in the branch, leaves included (drift #55).
        for (auto const n : branch)
        {
            if (!IsExpanded(n))
            {
                SetExpanded(n, true);
            }
        }

        changed = true;

        // top was collapsed, so its count was 0.
        ComputeSubtreeCounts(top);
        auto const count = top->VisibleDescendantCount;
        if (count > 0)
        {
            ApplyCountDelta(top, count);
            if (IsShown(top))
            {
                RaiseInserted(FlatIndexOf(top) + 1, count, false);
            }
        }
    }

    return changed;
}

void ShapedTree::PropagateCount(ShapedNode* parent, int64_t delta) noexcept
{
    // The caller has updated (or marked stale) the entry for the child below parent. Each expanded
    // ancestor takes the delta into its own count and its entry one level up; the first collapsed
    // one ends the walk with its own count still 0.
    for (auto n = parent; IsExpanded(n);)
    {
        n->VisibleDescendantCount = static_cast<uint32_t>(static_cast<int64_t>(n->VisibleDescendantCount) + delta);
        if (n == &m_root)
        {
            break;
        }

        auto const up = n->ShapedParent();
        if (!up->ChildRows.IsStale())
        {
            up->ChildRows.Add(n->IndexInParent, static_cast<int32_t>(delta));
        }

        n = up;
    }
}

uint32_t ShapedTree::FlatIndexOf(ShapedNode* node) const
{
    MUX_ASSERT(IsShown(node));

    uint32_t index = 0;
    for (auto n = node; n != &m_root;)
    {
        auto const parent = n->ShapedParent();
        index += CurrentChildRows(parent).PrefixSum(n->IndexInParent) + (parent == &m_root ? 0 : 1);
        n = parent;
    }

    return index;
}

void ShapedTree::Renumber(ShapedNode* parent, uint32_t index) noexcept
{
    auto const count = parent->ShapedChildCount();
    for (uint32_t j = index; j < count; ++j)
    {
        parent->ShapedChildAt(j)->IndexInParent = j;
    }
}

void ShapedTree::RebuildChildRows(ShapedNode* node) const
{
    node->ChildRows.Build(node->ShapedChildCount(), [this, node](uint32_t j) {
        auto const child = node->ShapedChildAt(j);
        return child == m_unlinked ? 0u : 1 + child->VisibleDescendantCount;
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
    // Post-order without a stack (no allocation, no recursion, #84): descend to the first leaf,
    // then after each node move to the next sibling's first leaf, or up to the parent once the
    // siblings are done. ShapedParent and IndexInParent are valid for every node below node: the
    // engine writes them while it builds, before the node is linked or the reset ends.
    auto const firstLeaf = [](ShapedNode* n) noexcept {
        while (n->ShapedChildCount() > 0)
        {
            n = n->ShapedChildAt(0);
        }
        return n;
    };

    for (auto n = firstLeaf(node);;)
    {
        if (n->ShapedChildCount() > 0 && IsExpanded(n))
        {
            RebuildChildRows(n);
            n->VisibleDescendantCount = n->ChildRows.Total();
        }
        else
        {
            // P7: a collapsed list is built on its next read (Expand).
            n->ChildRows.MarkStale();
            n->VisibleDescendantCount = 0;
        }

        if (n == node)
        {
            break;
        }

        auto const parent = n->ShapedParent();
        auto const next = n->IndexInParent + 1;
        MUX_ASSERT(parent->ShapedChildAt(n->IndexInParent) == n);
        n = next < parent->ShapedChildCount() ? firstLeaf(parent->ShapedChildAt(next)) : parent;
    }
}

ShapedNode* ShapedTree::NextShown(ShapedNode* node) const noexcept
{
    // m_unlinked is skipped wherever it sits (#90).
    auto const firstFrom = [this](ShapedNode* parent, uint32_t j) noexcept -> ShapedNode* {
        for (auto const count = parent->ShapedChildCount(); j < count; ++j)
        {
            if (auto const child = parent->ShapedChildAt(j); child != m_unlinked)
            {
                return child;
            }
        }
        return nullptr;
    };

    if (IsExpanded(node))
    {
        if (auto const child = firstFrom(node, 0))
        {
            return child;
        }
    }

    for (auto n = node; n != &m_root;)
    {
        auto const parent = n->ShapedParent();
        if (auto const sibling = firstFrom(parent, n->IndexInParent + 1))
        {
            return sibling;
        }

        n = parent;
    }

    return nullptr;
}

ShapedNode* ShapedTree::PreviousShown(ShapedNode* node) const noexcept
{
    // m_unlinked is skipped wherever it sits (#90).
    auto const lastBefore = [this](ShapedNode* parent, uint32_t j) noexcept -> ShapedNode* {
        for (; j > 0; --j)
        {
            if (auto const child = parent->ShapedChildAt(j - 1); child != m_unlinked)
            {
                return child;
            }
        }
        return nullptr;
    };

    auto const parent = node->ShapedParent();
    auto n = lastBefore(parent, node->IndexInParent);
    if (!n)
    {
        return parent == &m_root ? nullptr : parent;
    }

    while (IsExpanded(n))
    {
        auto const last = lastBefore(n, n->ShapedChildCount());
        if (!last)
        {
            break;
        }

        n = last;
    }

    return n;
}

void ShapedTree::InvalidateCursor() const noexcept
{
    m_cursorNode = nullptr;
    m_cursorIndex = 0;
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
