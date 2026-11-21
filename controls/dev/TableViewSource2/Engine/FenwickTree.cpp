// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Engine/FenwickTree.h"

#include <bit>

namespace
{
    // Lowest set bit of i: the length of the range node i covers.
    constexpr uint32_t LowBit(uint32_t i) noexcept
    {
        return i & (0u - i);
    }
}

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
    MUX_ASSERT(!m_stale);
    MUX_ASSERT(j < Size());
    MUX_ASSERT(delta >= 0 || PrefixSum(j + 1) - PrefixSum(j) >= static_cast<uint32_t>(-static_cast<int64_t>(delta)));

    // Unsigned wrap-around applies a negative delta exactly.
    uint32_t const step = static_cast<uint32_t>(delta);
    uint32_t const size = Size();
    for (uint32_t i = j + 1; i <= size; i += LowBit(i))
    {
        m_tree[i] += step;
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
    MUX_ASSERT(!m_stale);
    MUX_ASSERT(j <= Size());

    uint32_t sum = 0;
    for (uint32_t i = j; i > 0; i -= LowBit(i))
    {
        sum += m_tree[i];
    }

    return sum;
}

FenwickTree::Hit FenwickTree::Find(uint32_t r) const noexcept
{
    MUX_ASSERT(!m_stale);
    MUX_ASSERT(r < Total());

    // Descend from the largest power of two within Size(): take a node whenever the rows it covers
    // all lie before r. pos ends as the number of children wholly before r, i.e. the 0-based child
    // that covers r. A 0 entry (ShapedTree's unlinked node) is passed over, never returned.
    uint32_t const size = Size();
    uint32_t pos = 0;
    uint32_t remaining = r;
    for (uint32_t step = std::bit_floor(size); step != 0; step >>= 1)
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
    // Swap with an empty vector to free the storage; clear() would keep the capacity.
    std::vector<uint32_t>{}.swap(m_tree);
    m_stale = true;
}
