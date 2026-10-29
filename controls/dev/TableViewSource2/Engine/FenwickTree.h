// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <vector>

// Visible-row index over one sibling list (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Reading rows by position".
//
// A Fenwick (binary indexed) tree over the children of one shaped node: entry j is the number of
// visible rows child j contributes, 1 + its VisibleDescendantCount. The visible list is not stored
// (decided, P1); GetAt walks down the shaped tree and IndexOf walks up it, each asking one of these
// per level. Reads and a single count change are O(log k); a change of membership or order rebuilds
// the whole list in O(k).
//
// Staleness (decided, P7): a list whose membership or order changed is marked stale rather than
// rebuilt at once, and rebuilt on its next read (ShapedTree::CurrentChildRows). The bit lives in the
// tree, so it dies with its node and can never dangle. A default-constructed or cleared tree is
// stale, so a new node needs no marking.
//
// A leaf header: only the standard library. ShapeState.h includes it, so Source/ still depends on
// no engine header. Written only by ShapedTree. UI-thread affine.
class FenwickTree
{
public:
    // Number of entries (the sibling count).
    uint32_t Size() const noexcept;

    // Sum of every entry: the parent's VisibleDescendantCount when the list is expanded and not
    // stale. Precondition: !IsStale().
    uint32_t Total() const noexcept;

    // Replaces every entry: size entries, entry j = countOf(j), and clears the stale bit. O(k)
    // (linear build, not k updates). Called on the first read of a stale list (P7).
    template <typename CountOf>
    void Build(uint32_t size, CountOf&& countOf);

    // Adds delta to entry j: one child's count changed (expand, collapse, a row shown or hidden
    // below it). O(log k). Precondition: !IsStale(), j < Size() and the entry stays non-negative.
    // Callers skip a stale list: its rebuild recomputes the entry from the child's count.
    void Add(uint32_t j, int32_t delta) noexcept;

    // The list's membership or order changed (insert, remove, move, sort): entries no longer line
    // up with the children. O(1); the storage is kept for the rebuild.
    void MarkStale() noexcept;

    // True until the next Build. Reads (Total, PrefixSum, Find, Add) require false.
    bool IsStale() const noexcept;

    // Sum of entries [0, j): the visible rows before child j within this list. O(log k).
    // Precondition: j <= Size().
    uint32_t PrefixSum(uint32_t j) const noexcept;

    // The child whose rows cover offset r (0 <= r < Total()), counted from the first row of this
    // list, and the rows before it: PrefixSum(Child) <= r < PrefixSum(Child + 1). O(log k), one
    // descent over the tree's powers of two.
    struct Hit
    {
        uint32_t Child;
        uint32_t RowsBefore;
    };
    Hit Find(uint32_t r) const noexcept;

    // Drops every entry, frees the storage and marks the tree stale.
    void Clear() noexcept;

private:
    // 1-based Fenwick array; m_tree[0] unused. Empty when Size() is 0, so a leaf record pays only
    // the empty vector.
    std::vector<uint32_t> m_tree;

    bool m_stale{ true };
};

template <typename CountOf>
void FenwickTree::Build(uint32_t size, CountOf&& countOf)
{
    // Linear build: each entry adds itself into the next entry that covers it.
    m_tree.assign(size == 0 ? 0 : size + 1, 0);
    for (uint32_t i = 1; i <= size; ++i)
    {
        m_tree[i] += static_cast<uint32_t>(countOf(i - 1));
        uint32_t const parent = i + (i & (0u - i));
        if (parent <= size)
        {
            m_tree[parent] += m_tree[i];
        }
    }

    m_stale = false;
}
