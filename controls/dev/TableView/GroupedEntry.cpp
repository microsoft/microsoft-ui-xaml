// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "GroupedEntry.h"

GroupedEntry::GroupedEntry(
    winrt::hstring const& identity,
    winrt::IInspectable const& key,
    int32_t groupItemCount,
    int32_t childCount,
    bool isExpanded)
    : m_identity(identity)
    , m_key(key)
    , m_groupItemCount(groupItemCount)
    , m_childCount(childCount)
    , m_isExpanded(isExpanded)
{
}
