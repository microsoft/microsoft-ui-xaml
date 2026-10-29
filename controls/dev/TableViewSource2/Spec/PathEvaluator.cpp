// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Spec/PathEvaluator.h"
#include "SortMemberPathResolver.h"

PathEvaluator::PathEvaluator() noexcept
{
}

PathEvaluator::~PathEvaluator() = default;

winrt::IInspectable PathEvaluator::Value(winrt::hstring const& path, winrt::IInspectable const& item)
{
    if (path.empty() || !item)
    {
        return nullptr;
    }

    auto it = m_resolvers.find(path);
    if (it == m_resolvers.end())
    {
        it = m_resolvers.emplace(path, std::make_unique<SortMemberPathResolver>(path)).first;
    }

    // Resolve catches its own binding failures and gives nullptr (spec, "Misbehaving records").
    return it->second->Resolve(item);
}

void PathEvaluator::Retain(std::span<ShapeSpec const* const> specsInUse)
{
    for (auto it = m_resolvers.begin(); it != m_resolvers.end();)
    {
        bool used = false;
        for (ShapeSpec const* const spec : specsInUse)
        {
            if (!spec)
            {
                continue;
            }
            for (auto const& sort : spec->Sorts)
            {
                used = used || (sort.SortKind == SortSpec::Kind::Path && sort.Path == it->first);
            }
        }
        it = used ? std::next(it) : m_resolvers.erase(it);
    }
}

void PathEvaluator::Clear() noexcept
{
    m_resolvers.clear();
}
