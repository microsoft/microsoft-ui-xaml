// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/FenwickTree.h"

uint32_t FenwickTree::Size() const noexcept
{
    return m_tree.empty() ? 0 : static_cast<uint32_t>(m_tree.size() - 1);
}

uint32_t FenwickTree::Total() const noexcept
{
    return PrefixSum(Size());
}

void FenwickTree::Add(uint32_t j, int32_t delta) noexcept
{
    MUX_ASSERT(!m_stale && j < Size());
    for (uint32_t i = j + 1; i < m_tree.size(); i += i & (0u - i))
    {
        m_tree[i] = static_cast<uint32_t>(static_cast<int64_t>(m_tree[i]) + delta);
    }
}

void FenwickTree::MarkStale() noexcept
{
    m_stale = true;
}

bool FenwickTree::IsStale() const noexcept
{
    return m_stale;
}

uint32_t FenwickTree::PrefixSum(uint32_t j) const noexcept
{
    MUX_ASSERT(!m_stale && j <= Size());
    uint32_t sum = 0;
    for (uint32_t i = j; i > 0; i -= i & (0u - i))
    {
        sum += m_tree[i];
    }
    return sum;
}

FenwickTree::Hit FenwickTree::Find(uint32_t r) const noexcept
{
    MUX_ASSERT(!m_stale && r < Total());
    uint32_t const size = Size();

    uint32_t step = 1;
    while (step <= size / 2)
    {
        step <<= 1;
    }

    // pos ends as the number of leading children whose rows all lie before r: the child at index
    // pos covers r.
    uint32_t pos = 0;
    uint32_t remaining = r;
    for (; step > 0; step >>= 1)
    {
        uint32_t const next = pos + step;
        if (next <= size && m_tree[next] <= remaining)
        {
            pos = next;
            remaining -= m_tree[next];
        }
    }

    return Hit{ pos, r - remaining };
}

void FenwickTree::Clear() noexcept
{
    m_tree.clear();
    m_tree.shrink_to_fit();
    m_stale = true;
}
