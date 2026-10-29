// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/ShapedTree.h"

bool GroupExpansion::IsExpanded(winrt::hstring const& identity) const
{
    return m_exceptions.count(identity) ? !m_baselineExpanded : m_baselineExpanded;
}

void GroupExpansion::SetExpanded(winrt::hstring const& identity, bool expanded)
{
    if (expanded == m_baselineExpanded)
    {
        m_exceptions.erase(identity);
    }
    else
    {
        m_exceptions.insert(identity);
    }
}

void GroupExpansion::SetAll(bool expanded)
{
    m_baselineExpanded = expanded;
    m_exceptions.clear();
}

void GroupExpansion::Erase(winrt::hstring const& identity)
{
    m_exceptions.erase(identity);
}

void GroupExpansion::RetainOnly(std::unordered_map<winrt::hstring, std::unique_ptr<GroupNode>> const& liveGroups)
{
    std::erase_if(m_exceptions, [&](winrt::hstring const& identity) { return liveGroups.count(identity) == 0; });
}

void GroupExpansion::Clear()
{
    m_exceptions.clear();
}
