// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <winrt/Windows.Foundation.h>

enum class TableViewRowKind
{
    Data = 0,
    GroupHeader = 1,
};

struct TableViewRowInfo
{
    TableViewRowKind Kind{ TableViewRowKind::Data };
    int32_t Level{ 0 };
    bool IsExpandable{ false };
    bool IsExpanded{ false };
    // GroupHeader: group item count (0 presents a leaf). Hierarchical data row: child count.
    // Otherwise 0.
    int32_t ChildCount{ 0 };

    // Hierarchical data rows only. PositionInSet is 1-based within the sibling set (a root's group
    // bucket under grouping); ParentIdentity is empty for roots.
    winrt::hstring ParentIdentity;
    uint32_t PositionInSet{ 0 };
    uint32_t SizeOfSet{ 0 };
};

using TableViewRowItemKeySelector = std::function<winrt::hstring(winrt::IInspectable const&)>;

struct ITableViewRowMetadataProvider
{
    virtual ~ITableViewRowMetadataProvider() = default;

    virtual TableViewRowInfo GetRowInfo(int32_t index) = 0;
    virtual winrt::hstring GetIdentity(int32_t index) = 0;
    // Reverse of GetIdentity. Owned here because this is the only type that knows how a row's
    // identity is derived; consumers that needed it were each scanning every row and calling
    // GetIdentity until one matched.
    virtual bool TryGetIndexForIdentity(winrt::hstring const& identity, int32_t& index) = 0;
    virtual void Expand(winrt::hstring const& key) = 0;
    virtual void Collapse(winrt::hstring const& key) = 0;
    // True when the state changed (collapse included), not the resulting state.
    virtual bool Toggle(winrt::hstring const& key) = 0;

    // One verb per axis: expanding group headers must not materialize the whole tree.
    virtual void ExpandAllGroups() = 0;
    virtual void CollapseAllGroups() = 0;
    virtual void ExpandAllRows() = 0;
    virtual void CollapseAllRows() = 0;

    // True for tree projections. Leaf roots must still reserve chevron width, which row info alone
    // cannot distinguish from grouped data rows.
    virtual bool IsHierarchicalSource() const { return false; }

    // Expands a tree node and all descendants. No-op for group keys or non-hierarchical sources.
    virtual void ExpandSubtree(winrt::hstring const&) {}
};

using TableViewRowMetadataProvider = std::shared_ptr<ITableViewRowMetadataProvider>;
