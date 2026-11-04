// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "GroupedEntry.h"

GroupedEntry::GroupedEntry(
    winrt::IInspectable const& group,
    int32_t groupItemCount,
    bool isExpanded)
    : m_group(group)
    , m_groupItemCount(groupItemCount)
    , m_isExpanded(isExpanded)
{
}

winrt::IInspectable GroupedEntry::Group() const
{
    return m_group;
}

int32_t GroupedEntry::GroupItemCount() const
{
    return m_groupItemCount;
}

bool GroupedEntry::IsExpanded() const
{
    return m_isExpanded;
}
