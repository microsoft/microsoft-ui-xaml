// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "GroupedEntry.h"

GroupedEntry::GroupedEntry(
    winrt::hstring const& identity,
    winrt::IInspectable const& key,
    int32_t groupItemCount,
    bool isExpanded,
    int32_t level)
    : m_identity(identity)
    , m_key(key)
    , m_groupItemCount(groupItemCount)
    , m_isExpanded(isExpanded)
    , m_level(level)
{
}
