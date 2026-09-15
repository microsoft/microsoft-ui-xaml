// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

namespace DirectUI { namespace Components {

    // Projects the intermediate remove/insert states of a Move from its already-updated
    // source. Only indices are stored; preparing a move never enumerates the source.
    class CollectionMoveView
    {
    public:
        static bool IsValid(
            std::uint32_t sourceSize,
            std::int32_t oldIndex,
            std::int32_t newIndex,
            std::uint32_t oldCount,
            std::uint32_t newCount)
        {
            return oldCount != 0 &&
                oldCount == newCount &&
                oldCount <= sourceSize &&
                oldIndex >= 0 &&
                newIndex >= 0 &&
                static_cast<std::uint32_t>(oldIndex) <= sourceSize - oldCount &&
                static_cast<std::uint32_t>(newIndex) <= sourceSize - oldCount;
        }

        CollectionMoveView(
            std::uint32_t sourceSize,
            std::uint32_t oldIndex,
            std::uint32_t newIndex,
            std::uint32_t count)
            : m_sourceSize(sourceSize)
            , m_newIndex(newIndex)
            , m_count(count)
            , m_viewIndex(oldIndex)
            , m_visibleCount(count)
        {
        }

        std::uint32_t GetSize() const
        {
            return m_sourceSize - m_count + m_visibleCount;
        }

        std::uint32_t GetSourceIndex(std::uint32_t index) const
        {
            if (index >= m_viewIndex && index - m_viewIndex < m_visibleCount)
            {
                return m_newIndex + m_visibleOffset + (index - m_viewIndex);
            }

            // Index in the sequence with the entire moved range omitted.
            const auto remainingIndex = index < m_viewIndex ? index : index - m_visibleCount;
            return remainingIndex < m_newIndex ? remainingIndex : remainingIndex + m_count;
        }

        void RemoveNext()
        {
            ++m_visibleOffset;
            --m_visibleCount;
        }

        void InsertNext()
        {
            m_viewIndex = m_newIndex;
            m_visibleOffset = 0;
            ++m_visibleCount;
        }

    private:
        std::uint32_t m_sourceSize;
        std::uint32_t m_newIndex;
        std::uint32_t m_count;
        std::uint32_t m_viewIndex;
        std::uint32_t m_visibleCount;
        std::uint32_t m_visibleOffset = 0;
    };

} }
