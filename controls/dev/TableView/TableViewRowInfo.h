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
    // Rows the node owns. Meaningful on GroupHeader rows, where it is the group's item count and
    // the single source of truth for expandability: an empty group must present a leaf, not a
    // chevron that expands into nothing. Always 0 on Data rows today; it becomes the child count
    // when hierarchical (tree) rows land.
    int32_t ChildCount{ 0 };
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

    virtual void ExpandAllGroups() = 0;
    virtual void CollapseAllGroups() = 0;
};

using TableViewRowMetadataProvider = std::shared_ptr<ITableViewRowMetadataProvider>;
