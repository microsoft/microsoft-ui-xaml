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
    // Rows the node owns. On GroupHeader rows it is the group's item count and the single source
    // of truth for expandability: an empty group must present a leaf, not a chevron that expands
    // into nothing. On hierarchical data rows it is the node's child count; 0 on other data rows.
    int32_t ChildCount{ 0 };

    // Hierarchical data rows only (empty / 0 otherwise). ParentIdentity is the parent row's
    // identity (as GetIdentity reports it), empty for roots. PositionInSet is 1-based within the
    // row's sibling set -- a root's sibling set is its group bucket under grouping -- and SizeOfSet
    // is that set's size.
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
    virtual bool Toggle(winrt::hstring const& key) = 0;

    // Bulk expansion, one verb per AXIS. Kept separate because "expand everything" means two
    // different things: a group header opens a bucket that is already materialized, while a tree
    // node's expansion adds its visible subtree (from the parent-key index) to the row axis. Routing
    // the group verbs into both axes would make opening the headers of a grouped tree materialize
    // the whole tree.
    virtual void ExpandAllGroups() = 0;
    virtual void CollapseAllGroups() = 0;
    virtual void ExpandAllRows() = 0;
    virtual void CollapseAllRows() = 0;

    // True when the rows come from a tree projection. A LEAF root reports Level 1 and
    // IsExpandable false, which is indistinguishable per row from a grouped data row, yet it still
    // has to reserve the chevron's width or its text would sit left of its expandable siblings'.
    // Only the source knows the difference, so the answer lives here and not in the row info.
    virtual bool IsHierarchicalSource() const { return false; }

    // Expands a tree node and every descendant that has children. No-op for a group key or a
    // non-hierarchical source.
    virtual void ExpandSubtree(winrt::hstring const&) {}
};

using TableViewRowMetadataProvider = std::shared_ptr<ITableViewRowMetadataProvider>;
