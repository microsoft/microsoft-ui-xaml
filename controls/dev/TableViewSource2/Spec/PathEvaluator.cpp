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
    if (path.empty() || !item || m_isResolving)
    {
        return nullptr;
    }

    auto it = m_resolvers.find(path);
    if (it == m_resolvers.end())
    {
        it = m_resolvers.emplace(path, std::make_unique<SortMemberPathResolver>(path)).first;
    }

    // Resolve runs app code. Pin the resolver: Retain skips and Clear defers until this returns.
    SortMemberPathResolver& resolver = *it->second;
    m_isResolving = true;
    auto endResolve = wil::scope_exit([this]() noexcept
    {
        m_isResolving = false;
        if (m_clearPending)
        {
            m_clearPending = false;
            m_resolvers.clear();
        }
    });

    // Resolve catches its own binding failures and gives nullptr (spec, "Misbehaving records").
    return resolver.Resolve(item);
}

void PathEvaluator::Retain(std::span<ShapeSpec const* const> specsInUse)
{
    if (m_isResolving)
    {
        return;
    }

    // Nested scan with early exit; allocates nothing. A few resolvers, at most two specs and a few
    // sort levels each, so this is cheaper than building a set.
    auto const isUsed = [specsInUse](winrt::hstring const& path) noexcept
    {
        for (ShapeSpec const* const spec : specsInUse)
        {
            if (!spec)
            {
                continue;
            }
            for (SortSpec const& sort : spec->Sorts)
            {
                if (sort.SortKind == SortSpec::Kind::Path && sort.Path == path)
                {
                    return true;
                }
            }
        }
        return false;
    };

    for (auto it = m_resolvers.begin(); it != m_resolvers.end();)
    {
        it = isUsed(it->first) ? std::next(it) : m_resolvers.erase(it);
    }
}

void PathEvaluator::Clear() noexcept
{
    if (m_isResolving)
    {
        m_clearPending = true;
        return;
    }

    m_resolvers.clear();
}
