// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/ShapeState.h"
#include "Engine/ShapedTree.h"
#include "Source/SourceRecord.h"

namespace
{
    bool AreChildrenShaped(SourceRecord const& record) noexcept
    {
        return (record.Shape.Flags & ShapeFlags::ChildrenShaped) != ShapeFlags::None;
    }
}

SourceRecord* ShapedNode::AsRecord() noexcept
{
    return IsGroup() ? nullptr : static_cast<SourceRecord*>(this);
}

SourceRecord const* ShapedNode::AsRecord() const noexcept
{
    return IsGroup() ? nullptr : static_cast<SourceRecord const*>(this);
}

GroupNode* ShapedNode::AsGroup() noexcept
{
    return IsGroup() ? static_cast<GroupNode*>(this) : nullptr;
}

GroupNode const* ShapedNode::AsGroup() const noexcept
{
    return IsGroup() ? static_cast<GroupNode const*>(this) : nullptr;
}

ShapedNode* ShapedNode::ShapedParent() const noexcept
{
    if (auto const group = AsGroup())
    {
        return group->Parent;
    }

    // A nested record's shaped parent is its source parent; a top-level record's is the group (or
    // shaped root) the engine placed it in, null while it is not in the shaped tree.
    auto const record = AsRecord();
    if (record->Parent)
    {
        return record->Parent;
    }

    return record->Shape.Group;
}

uint32_t ShapedNode::ShapedChildCount() const noexcept
{
    if (auto const group = AsGroup())
    {
        return static_cast<uint32_t>(group->Children.size());
    }

    auto const record = AsRecord();
    return static_cast<uint32_t>(AreChildrenShaped(*record) ? record->Shape.ShapedChildren.size() : record->Children.size());
}

ShapedNode* ShapedNode::ShapedChildAt(uint32_t j) const noexcept
{
    MUX_ASSERT(j < ShapedChildCount());

    if (auto const group = AsGroup())
    {
        return group->Children[j];
    }

    auto const record = AsRecord();
    return AreChildrenShaped(*record) ? record->Shape.ShapedChildren[j] : record->Children[j];
}
