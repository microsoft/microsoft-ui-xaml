// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class SelectionTreeHelper
{
public:
    struct TreeWalkNodeInfo
    {
        TreeWalkNodeInfo(std::shared_ptr<SelectionNode> node, const winrt::IndexPath& indexPath, std::shared_ptr<SelectionNode> parent)
            : Node(node), Path(indexPath), ParentNode(parent) {}
        TreeWalkNodeInfo(std::shared_ptr<SelectionNode> node, const winrt::IndexPath& indexPath)
            : Node(node), Path(indexPath), ParentNode(nullptr) {}

        std::shared_ptr<SelectionNode> Node;
        winrt::IndexPath Path{}{};
        std::shared_ptr<SelectionNode> ParentNode;
    };

    static void TraverseIndexPath(
        std::shared_ptr<SelectionNode> root,
        const winrt::IndexPath& path,
        bool realizeChildren,
        std::function<void(std::shared_ptr<SelectionNode>, const winrt::IndexPath&, int /*depth*/, int /*childIndex*/)> nodeAction);

    static void Traverse(
        std::shared_ptr<SelectionNode> root,
        bool realizeChildren,
        std::function<void(const TreeWalkNodeInfo&)> nodeAction);

    static void TraverseRangeRealizeChildren(
        std::shared_ptr<SelectionNode> root,
        const winrt::IndexPath& start,
        const winrt::IndexPath& end,
        std::function<void(const TreeWalkNodeInfo&)> nodeAction);

private:
    static bool IsSubSet(const winrt::IndexPath& path, const winrt::IndexPath& subset);
    static winrt::IndexPath StartPath(const winrt::IndexPath& path, int length);
};
