// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "SelectionNode.h"
#include "SelectionTreeHelper.h"
#include "IndexPath.h"

// static
void SelectionTreeHelper::TraverseIndexPath(
    std::shared_ptr<SelectionNode> root,
    const winrt::IndexPath& path,
    bool realizeChildren,
    std::function<void(std::shared_ptr<SelectionNode>, const winrt::IndexPath&, int /*depth*/, int /*childIndex*/)> nodeAction)
{
    auto node = root;
    for (int depth = 0; depth < path.GetSize(); depth++)
    {
        const int childIndex = path.GetAt(depth);
        nodeAction(node, path, depth, childIndex);

        if (depth < path.GetSize() - 1)
        {
            node = node->GetAt(childIndex, realizeChildren);
        }
    }
}

// static 
void SelectionTreeHelper::Traverse(
    std::shared_ptr<SelectionNode> root,
    bool realizeChildren,
    std::function<void(const TreeWalkNodeInfo&)> nodeAction)
{
    auto pendingNodes = std::vector<TreeWalkNodeInfo>();
    auto current = winrt::make<IndexPath>(nullptr);
    pendingNodes.push_back(TreeWalkNodeInfo(root, current));

    while (pendingNodes.size() > 0)
    {
        const auto nextNode = pendingNodes.back();
        pendingNodes.pop_back();
        const int count = realizeChildren ? nextNode.Node->DataCount() : nextNode.Node->ChildrenNodeCount();
        for (int i = count - 1; i >= 0; i--)
        {
            std::shared_ptr<SelectionNode> child = nextNode.Node->GetAt(i, realizeChildren);
            auto childPath = winrt::get_self<IndexPath>(nextNode.Path)->CloneWithChildIndex(i);
            if (child != nullptr)
            {
                pendingNodes.push_back(TreeWalkNodeInfo(child, childPath, nextNode.Node));
            }
        }

        // Queue the children first and then perform the action. This way
        // the action can remove the children in the action if necessary
        nodeAction(nextNode);
    }
}


// static 
void SelectionTreeHelper::TraverseRangeRealizeChildren(
    std::shared_ptr<SelectionNode> root,
    const winrt::IndexPath& start,
    const winrt::IndexPath& end,
    std::function<void(const TreeWalkNodeInfo&)> nodeAction)
{
    MUX_ASSERT(start.CompareTo(end) == -1);

    auto pendingNodes = std::vector<TreeWalkNodeInfo>();
    winrt::IndexPath current = start;

    // Build up the stack to account for the depth first walk up to the 
    // start index path.
    TraverseIndexPath(
        root,
        start,
        true, /* realizeChildren */
        [&start, &end, &pendingNodes](std::shared_ptr<SelectionNode> node, const winrt::IndexPath& path, int depth, int childIndex)
    {
        const auto currentPath = StartPath(path, depth);
        const bool isStartPath = IsSubSet(start, currentPath);
        const bool isEndPath = IsSubSet(end, currentPath);

        const int startIndex = depth < start.GetSize() && isStartPath ? std::max(0, start.GetAt(depth)) : 0;
        const int endIndex = depth < end.GetSize() && isEndPath ? std::min(node->DataCount() - 1, end.GetAt(depth)) : node->DataCount() - 1;

        for (int i = endIndex; i >= startIndex; i--)
        {
            auto child = node->GetAt(i, true /* realizeChild */);
            if (child)
            {
                auto childPath = winrt::get_self<IndexPath>(currentPath)->CloneWithChildIndex(i);
                pendingNodes.push_back(TreeWalkNodeInfo(child, childPath, node));
            }
        }
    });

    // From the start index path, do a depth first walk as long as the
    // current path is less than the end path.
    while (pendingNodes.size() > 0)
    {
        const auto info = pendingNodes.back();
        pendingNodes.pop_back();
        const int depth = info.Path.GetSize();
        const bool isStartPath = IsSubSet(start, info.Path);
        const bool isEndPath = IsSubSet(end, info.Path);
        const int startIndex = depth < start.GetSize() && isStartPath ? start.GetAt(depth) : 0;
        const int endIndex = depth < end.GetSize() && isEndPath ? end.GetAt(depth) : info.Node->DataCount() - 1;
        for (int i = endIndex; i >= startIndex; i--)
        {
            const auto child = info.Node->GetAt(i, true /* realizeChild */);
            if (child)
            {
                auto childPath = winrt::get_self<IndexPath>(info.Path)->CloneWithChildIndex(i);
                pendingNodes.push_back(TreeWalkNodeInfo(child, childPath, info.Node));
            }
        }

        nodeAction(info);

        if (info.Path.CompareTo(end) == 0)
        {
            // We reached the end index path. stop iterating.
            break;
        }
    }
}

// static 
bool SelectionTreeHelper::IsSubSet(const winrt::IndexPath& path, const winrt::IndexPath& subset)
{
    const auto subsetSize = subset.GetSize();
    if (path.GetSize() < subsetSize)
    {
        return false;
    }

    for (int i = 0; i < subsetSize; i++)
    {
        if (path.GetAt(i) != subset.GetAt(i))
        {
            return false;
        }
    }

    return true;
}

// static 
winrt::IndexPath SelectionTreeHelper::StartPath(const winrt::IndexPath& path, int length)
{
    std::vector<int> subPath;
    for (int i = 0; i < length; i++)
    {
        subPath.emplace_back(path.GetAt(i));
    }

    return winrt::make<IndexPath>(subPath);
}
