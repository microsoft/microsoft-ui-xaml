// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LinedFlowLayoutTrace.h"

// This class is used to track weighted aspect ratios of some LinedFlowLayout items. Blocks tracking 64 contiguous items
// are allocated. To minimize allocations, those blocks are re-used when a different area of the items collection needs
// to be tracked.
//
// Example:
// Initially blocks [0,63][64,127] track the first items of 5700 items i the collection.  The user uses the PageDown key a 
// few times and the tracking blocks are extended to [0,63][64,127][128,191][192,255].  Then the user hits the End key 
// causing the blocks to track [0,63][64,127][128,191][192,255][5632,5695][5696,5759].  Finally the user keeps hitting the
// PageUp key to the middle of the collection and the blocks become:
// [2624,2687][2688,2751][2752,2815][2816,2879][2880,2943][2944,3007][3008,3071].
//
// The consumer of this class is in charge of setting a limit to the number of items being tracked. In this case the LinedFlowLayout
// sets the limit to 10 scrolling viewports worth of items. If on average a scrolling viewport can hold 40 items, no more than
// 7 blocks (i.e. 400 / 64) will be allocated to store aspect ratios of 400 items at a time.
//
// As the user jumps from area to area in the items collection, the closest existing blocks from the new area are preserved,
// while the furthest away are recycled.

class LinedFlowLayoutItemAspectRatios
{
public:
    LinedFlowLayoutItemAspectRatios() {};

    struct ItemAspectRatio
    {
    public:
        float m_aspectRatio{};
        int m_weight{};
    };

    static constexpr ItemAspectRatio s_emptyItemAspectRatio
    {
        0.0f /*aspectRatio*/,
        0 /*weight*/
    };

    bool HasLowerWeight(
        int firstItemIndex,
        int lastItemIndex,
        int weight) const;

    bool IsEmpty() const;

    float GetAverageAspectRatio(
        int firstItemIndex,
        int lastItemIndex,
        int weight) const;

    ItemAspectRatio GetAt(
        int itemIndex) const;
    void SetAt(
        int itemIndex, ItemAspectRatio itemAspectRatio);

    void Clear();

    void Resize(
        int size,
        int referenceItemIndex);

#ifdef DBG
    void LogItemAspectRatiosDbg();
#endif

private:
    class ItemAspectRatioBlock
    {
    public:
        ItemAspectRatioBlock() {};

        ItemAspectRatio GetTotals(int firstItemIndex,
            int lastItemIndex,
            int weight) const;

        bool HasLowerWeight(
            int firstItemIndex,
            int lastItemIndex,
            int weight) const;

        bool IncludesItemIndex(
            int itemIndex) const;

        ItemAspectRatio GetAt(
            int itemIndex) const;
        void SetAt(
            int itemIndex, ItemAspectRatio itemAspectRatio);

        int StartIndex() const
        {
            return m_startIndex;
        }

        void StartIndex(
            int startIndex)
        {
            m_startIndex = startIndex;
        }

        int EndIndex() const
        {
            return m_startIndex == -1 ? -1 : m_startIndex + c_blockSize - 1;
        }

        static int BlockSize()
        {
            return c_blockSize;
        }

        void Clear();

#ifdef DBG
        void LogItemAspectRatioBlockDbg();
#endif

    private:
        static constexpr int c_blockSize{ 64 };

        ItemAspectRatio m_aspectRatios[c_blockSize];
        int m_startIndex{ -1 };
    };

    std::shared_ptr<ItemAspectRatioBlock> GetEmptyOrFurthestBlock(
        int fromItemIndex);

    std::set<std::shared_ptr<ItemAspectRatioBlock>> m_itemAspectRatioBlocks{};
};
