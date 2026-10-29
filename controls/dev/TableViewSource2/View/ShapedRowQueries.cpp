// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "View/ShapedRowQueries.h"

#include <string_view>

namespace
{
    // Group row keys start with this marker (V3); record keys start with "0x".
    constexpr std::wstring_view c_groupKeyPrefix{ L"g|" };
}

ShapedRowQueries::ShapedRowQueries(ShapingEngine const& engine, SourceTable const& source) noexcept
    : m_engine(engine)
    , m_source(source)
{
}

uint32_t ShapedRowQueries::RowCount() const noexcept
{
    return m_engine.Tree().Size();
}

ShapedNode* ShapedRowQueries::RowAt(uint32_t index) const
{
    if (index >= RowCount())
    {
        throw winrt::hresult_out_of_bounds();
    }

    return m_engine.Tree().GetAt(index);
}

winrt::hstring ShapedRowQueries::KeyAt(uint32_t index) const
{
    return KeyOf(*RowAt(index));
}

winrt::hstring ShapedRowQueries::KeyOf(ShapedNode const& node)
{
    if (GroupNode const* const group = node.AsGroup())
    {
        return group->Identity;
    }

    return node.AsRecord()->Identity();
}

std::optional<uint32_t> ShapedRowQueries::IndexOfKey(winrt::hstring const& key) const
{
    ShapedNode* const node = TryResolveKey(key);
    return node ? m_engine.Tree().IndexOf(node) : std::nullopt;
}

std::optional<uint32_t> ShapedRowQueries::IndexOfItem(winrt::IInspectable const& value) const
{
    // Group header values (GroupedEntry) resolve through their Identity from Milestone 4.
    SourceRecord* const record = m_source.TryFindByItem(value);
    return record ? m_engine.Tree().IndexOf(record) : std::nullopt;
}

TableViewRowInfo ShapedRowQueries::RowInfo(uint32_t index) const
{
    ShapedTree const& tree = m_engine.Tree();
    ShapedNode* const node = RowAt(index);

    TableViewRowInfo info;
    if (GroupNode const* const group = node->AsGroup())
    {
        info.Kind = TableViewRowKind::GroupHeader;
        info.Level = group->GroupLevel;
        info.IsExpandable = true;
        info.IsExpanded = tree.IsExpanded(node);
        info.ChildCount = static_cast<int32_t>(group->ItemCount);
        return info;
    }

    SourceRecord const* const record = node->AsRecord();
    uint32_t const childCount = node->ShapedChildCount();

    info.Kind = TableViewRowKind::Data;
    info.Level = tree.GroupLevelCount() + record->Depth;
    info.IsExpandable = childCount > 0;
    info.IsExpanded = info.IsExpandable && tree.IsExpanded(node);

    if (IsHierarchical())
    {
        ShapedNode* const parent = node->ShapedParent();
        info.ChildCount = static_cast<int32_t>(childCount);
        info.ParentIdentity = record->Parent ? record->Parent->Identity() : winrt::hstring{};
        info.PositionInSet = node->IndexInParent + 1;
        info.SizeOfSet = parent ? parent->ShapedChildCount() : 0;
    }

    return info;
}

ShapedNode* ShapedRowQueries::TryResolveKey(winrt::hstring const& key) const
{
    if (std::wstring_view{ key }.starts_with(c_groupKeyPrefix))
    {
        auto const& groups = m_engine.GroupsByIdentity();
        auto const it = groups.find(key);
        return it == groups.end() ? nullptr : it->second.get();
    }

    if (auto const address = SourceRecord::TryParseIdentity(key))
    {
        return m_source.TryFindByAddress(*address);
    }

    return nullptr;
}

bool ShapedRowQueries::IsHierarchical() const noexcept
{
    return m_source.HasHierarchy();
}

bool ShapedRowQueries::TryGetGroupMembership(uint32_t index, uint32_t& positionInGroup, uint32_t& groupSize) const
{
    ShapedTree const& tree = m_engine.Tree();
    if (tree.GroupLevelCount() == 0 || index >= RowCount())
    {
        return false;
    }

    ShapedNode* const node = tree.GetAt(index);
    if (node->IsGroup())
    {
        return false;
    }

    ShapedNode* group = node->ShapedParent();
    while (group && !group->IsGroup())
    {
        group = group->ShapedParent();
    }

    auto const groupIndex = group ? tree.IndexOf(group) : std::nullopt;
    if (!groupIndex)
    {
        return false;
    }

    positionInGroup = index - *groupIndex;
    groupSize = tree.ShownDescendantCount(group);
    return true;
}

bool ShapedRowQueries::IsItemInShapedTree(winrt::IInspectable const& item) const
{
    SourceRecord* const record = m_source.TryFindByItem(item);
    return record && ShapingEngine::InShapedTree(record);
}
