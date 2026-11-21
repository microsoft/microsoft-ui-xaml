// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

namespace DirectUI { namespace Components {

    // Projects the intermediate states of a multi-item Add, Remove or Replace from its
    // already-updated source, so that each per-item notification describes a consistent
    // collection. The view is the source with its new range [index, index + newCount)
    // replaced by the new items applied so far, followed by the old items not yet
    // replaced or removed.
    class CollectionRangeChangeView
    {
    public:
        struct Location
        {
            bool isOldItem;
            std::uint32_t index;
        };

        static bool IsValid(
            std::uint32_t sourceSize,
            std::int32_t index,
            std::uint32_t newCount)
        {
            return index >= 0 &&
                newCount <= sourceSize &&
                static_cast<std::uint32_t>(index) <= sourceSize - newCount;
        }

        CollectionRangeChangeView(
            std::uint32_t sourceSize,
            std::uint32_t index,
            std::uint32_t oldCount,
            std::uint32_t newCount)
            : m_sourceSize(sourceSize)
            , m_index(index)
            , m_oldCount(oldCount)
            , m_newCount(newCount)
        {
        }

        std::uint32_t GetSize() const
        {
            return m_sourceSize - m_newCount + m_appliedCount + GetPendingOldCount();
        }

        Location GetLocation(std::uint32_t index) const
        {
            const auto appliedEnd = m_index + m_appliedCount;
            if (index < appliedEnd)
            {
                return { false, index };
            }

            const auto pendingOffset = index - appliedEnd;
            if (pendingOffset < GetPendingOldCount())
            {
                return { true, m_consumedOldCount + pendingOffset };
            }

            return { false, m_index + m_newCount + pendingOffset - GetPendingOldCount() };
        }

        void ReplaceNext()
        {
            ++m_appliedCount;
            ++m_consumedOldCount;
        }

        void RemoveNext()
        {
            ++m_consumedOldCount;
        }

        void InsertNext()
        {
            ++m_appliedCount;
        }

        enum class IndexOfResult
        {
            Found,
            NotFound,
            // The first match is a new item not yet inserted or an old item already removed,
            // so a later equal item could still be visible.
            Ambiguous,
        };

        // Maps the first matches reported by the source's and the old items' own IndexOf
        // (which apply their own equality) to the view, which lists the source items before
        // the change, then the pending old items, then the source items after the change.
        IndexOfResult ResolveIndexOf(
            bool sourceFound,
            std::uint32_t sourceIndex,
            bool oldItemFound,
            std::uint32_t oldItemIndex,
            std::uint32_t* index) const
        {
            *index = 0;
            const auto appliedEnd = m_index + m_appliedCount;
            if (sourceFound && sourceIndex < appliedEnd)
            {
                *index = sourceIndex;
                return IndexOfResult::Found;
            }
            if (oldItemFound && oldItemIndex >= m_consumedOldCount)
            {
                *index = appliedEnd + oldItemIndex - m_consumedOldCount;
                return IndexOfResult::Found;
            }
            if ((sourceFound && sourceIndex < m_index + m_newCount) || oldItemFound)
            {
                return IndexOfResult::Ambiguous;
            }
            if (sourceFound)
            {
                *index = sourceIndex - m_newCount + m_appliedCount + GetPendingOldCount();
                return IndexOfResult::Found;
            }
            return IndexOfResult::NotFound;
        }

    private:
        std::uint32_t GetPendingOldCount() const
        {
            return m_oldCount - m_consumedOldCount;
        }

        std::uint32_t m_sourceSize;
        std::uint32_t m_index;
        std::uint32_t m_oldCount;
        std::uint32_t m_newCount;
        std::uint32_t m_appliedCount = 0;
        std::uint32_t m_consumedOldCount = 0;
    };

} }
