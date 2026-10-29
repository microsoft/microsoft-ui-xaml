// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "View/ShapedRowMetadata.h"
#include "View/ShapedRowQueries.h"
#include "Shaping/IShapedExpansion.h"

namespace
{
    bool InRange(ShapedRowQueries const* rows, int32_t index) noexcept
    {
        return rows && index >= 0 && static_cast<uint32_t>(index) < rows->RowCount();
    }
}

ShapedRowMetadata::ShapedRowMetadata(ShapedRowQueries const& rows, IShapedExpansion& expansion) noexcept
    : m_rows(&rows)
    , m_expansion(&expansion)
{
}

TableViewRowInfo ShapedRowMetadata::GetRowInfo(int32_t index)
{
    return InRange(m_rows, index) ? m_rows->RowInfo(static_cast<uint32_t>(index)) : TableViewRowInfo{};
}

winrt::hstring ShapedRowMetadata::GetIdentity(int32_t index)
{
    return InRange(m_rows, index) ? m_rows->KeyAt(static_cast<uint32_t>(index)) : winrt::hstring{};
}

bool ShapedRowMetadata::TryGetIndexForIdentity(winrt::hstring const& identity, int32_t& index)
{
    if (!m_rows)
    {
        return false;
    }

    auto const found = m_rows->IndexOfKey(identity);
    if (!found)
    {
        return false;
    }

    index = static_cast<int32_t>(*found);
    return true;
}

bool ShapedRowMetadata::Expand(winrt::hstring const& key)
{
    ShapedNode* const node = m_rows ? m_rows->TryResolveKey(key) : nullptr;
    return node && m_expansion->SetExpanded(node, true);
}

bool ShapedRowMetadata::Collapse(winrt::hstring const& key)
{
    ShapedNode* const node = m_rows ? m_rows->TryResolveKey(key) : nullptr;
    return node && m_expansion->SetExpanded(node, false);
}

bool ShapedRowMetadata::Toggle(winrt::hstring const& key)
{
    ShapedNode* const node = m_rows ? m_rows->TryResolveKey(key) : nullptr;
    return node && m_expansion->SetExpanded(node, std::nullopt);
}

void ShapedRowMetadata::ExpandAllGroups()
{
    if (m_expansion)
    {
        m_expansion->SetAllExpanded(ExpansionAxis::Groups, true);
    }
}

void ShapedRowMetadata::CollapseAllGroups()
{
    if (m_expansion)
    {
        m_expansion->SetAllExpanded(ExpansionAxis::Groups, false);
    }
}

void ShapedRowMetadata::ExpandAllRows()
{
    if (m_expansion)
    {
        m_expansion->SetAllExpanded(ExpansionAxis::Rows, true);
    }
}

void ShapedRowMetadata::CollapseAllRows()
{
    if (m_expansion)
    {
        m_expansion->SetAllExpanded(ExpansionAxis::Rows, false);
    }
}

void ShapedRowMetadata::ExpandSubtree(winrt::hstring const& key)
{
    if (!m_rows || !m_rows->IsHierarchical())
    {
        return;
    }

    ShapedNode* const node = m_rows->TryResolveKey(key);
    if (node && !node->IsGroup())
    {
        m_expansion->ExpandSubtree(node);
    }
}

int32_t ShapedRowMetadata::Reveal(winrt::hstring const& key)
{
    ShapedNode* const node = m_rows ? m_rows->TryResolveKey(key) : nullptr;
    if (!node)
    {
        return -1;
    }

    auto const index = m_expansion->Reveal(node);
    return index ? static_cast<int32_t>(*index) : -1;
}

bool ShapedRowMetadata::IsHierarchicalSource() const
{
    return m_rows && m_rows->IsHierarchical();
}

bool ShapedRowMetadata::TryGetGroupMembership(
    winrt::Windows::Foundation::IInspectable const& rows,
    int32_t index,
    int32_t& positionInGroup,
    int32_t& groupSize)
{
    // The "rows is this source's view" identity check is completed in Milestone 4 (grouping).
    if (!m_rows || !rows || index < 0)
    {
        return false;
    }

    uint32_t position = 0;
    uint32_t size = 0;
    if (!m_rows->TryGetGroupMembership(static_cast<uint32_t>(index), position, size))
    {
        return false;
    }

    positionInGroup = static_cast<int32_t>(position);
    groupSize = static_cast<int32_t>(size);
    return true;
}

void ShapedRowMetadata::Detach() noexcept
{
    m_rows = nullptr;
    m_expansion = nullptr;
}
