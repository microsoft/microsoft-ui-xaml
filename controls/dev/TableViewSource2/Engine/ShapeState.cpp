// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/ShapeState.h"
#include "Engine/ShapedTree.h"
#include "Source/SourceRecord.h"

SourceRecord* ShapedNode::AsRecord() noexcept
{
    return Kind == ShapedNodeKind::Record ? static_cast<SourceRecord*>(this) : nullptr;
}

SourceRecord const* ShapedNode::AsRecord() const noexcept
{
    return Kind == ShapedNodeKind::Record ? static_cast<SourceRecord const*>(this) : nullptr;
}

GroupNode* ShapedNode::AsGroup() noexcept
{
    return Kind == ShapedNodeKind::Group ? static_cast<GroupNode*>(this) : nullptr;
}

GroupNode const* ShapedNode::AsGroup() const noexcept
{
    return Kind == ShapedNodeKind::Group ? static_cast<GroupNode const*>(this) : nullptr;
}

ShapedNode* ShapedNode::ShapedParent() const noexcept
{
    if (auto const group = AsGroup())
    {
        return group->Parent;
    }

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
    if ((record->Shape.Flags & ShapeFlags::ChildrenShaped) != ShapeFlags::None)
    {
        return static_cast<uint32_t>(record->Shape.ShapedChildren.size());
    }

    return static_cast<uint32_t>(record->Children.size());
}

ShapedNode* ShapedNode::ShapedChildAt(uint32_t j) const noexcept
{
    if (auto const group = AsGroup())
    {
        return group->Children[j];
    }

    auto const record = AsRecord();
    if ((record->Shape.Flags & ShapeFlags::ChildrenShaped) != ShapeFlags::None)
    {
        return record->Shape.ShapedChildren[j];
    }

    return record->Children[j];
}
