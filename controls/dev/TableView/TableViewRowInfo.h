// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <winrt/Windows.Foundation.h>

enum class TableViewRowKind
{
    Data = 0,
    GroupHeader = 1,
};

struct TableViewRowInfo
{
    TableViewRowKind Kind{ TableViewRowKind::Data };
    // GroupHeader: 0 for a top-level group, one deeper per subgroup level. Data row under groups: one
    // deeper than its header. Otherwise 0.
    int32_t Level{ 0 };
    bool IsExpandable{ false };
    bool IsExpanded{ false };
    // GroupHeader: group item count (0 presents a leaf). Otherwise 0.
    int32_t ChildCount{ 0 };
};

using TableViewRowItemKeySelector = std::function<winrt::hstring(winrt::IInspectable const&)>;

// A group header's expansion identity (what Expand/Collapse take), its GroupBy key, and its state.
struct TableViewGroupExpansionInfo
{
    winrt::hstring Identity;
    winrt::Windows::Foundation::IInspectable Key{ nullptr };
    bool IsExpanded{ false };
};

struct ITableViewRowMetadataProvider
{
    virtual ~ITableViewRowMetadataProvider() = default;

    virtual TableViewRowInfo GetRowInfo(int32_t index) = 0;
    virtual winrt::hstring GetIdentity(int32_t index) = 0;
    // Reverse of GetIdentity. Owned here because this is the only type that knows how a row's
    // identity is derived; consumers that needed it were each scanning every row and calling
    // GetIdentity until one matched.
    virtual bool TryGetIndexForIdentity(winrt::hstring const& identity, int32_t& index) = 0;
    // All three return true when the state changed (collapse included), not the resulting state.
    virtual bool Expand(winrt::hstring const& key) = 0;
    virtual bool Collapse(winrt::hstring const& key) = 0;
    virtual bool Toggle(winrt::hstring const& key) = 0;
    // Whether Expand (desired true), Collapse (false) or Toggle (empty) would change the state; true when unsure.
    virtual bool WouldChangeExpansion(winrt::hstring const& key, std::optional<bool> desired) noexcept = 0;

    virtual void ExpandAllGroups() = 0;
    virtual void CollapseAllGroups() = 0;

    // Every group in the projection, in display order: O(groups). Empty when the source is not grouped.
    virtual std::vector<TableViewGroupExpansionInfo> GetGroupExpansionInfos() { return {}; }
    // The expansion identity of the group with this stable identity. Empty when it has none.
    virtual winrt::hstring GetGroupExpansionIdentity(winrt::hstring const& /*groupIdentity*/) { return {}; }
    // The group a group expansion identity names. False for unknown groups.
    virtual bool TryGetGroupExpansionInfo(winrt::hstring const& /*identity*/, TableViewGroupExpansionInfo& /*info*/) { return false; }

    // 1-based position among data rows between adjacent header bands, and that count. Returns false
    // if `rows` isn't the provider's projection, or for flat sources, headers, or out of range.
    virtual bool TryGetGroupMembership(
        winrt::Windows::Foundation::IInspectable const& /*rows*/,
        int32_t /*index*/,
        int32_t& /*positionInGroup*/,
        int32_t& /*groupSize*/)
    {
        return false;
    }
};

using TableViewRowMetadataProvider = std::shared_ptr<ITableViewRowMetadataProvider>;
