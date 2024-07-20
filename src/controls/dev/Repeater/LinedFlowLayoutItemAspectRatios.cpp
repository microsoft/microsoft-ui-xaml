// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "LinedFlowLayoutItemAspectRatios.h"

// Returns True when there is at least one stored ratio in the provided index range,
// with a strictly positive weight lower than the provided number.
bool LinedFlowLayoutItemAspectRatios::HasLowerWeight(
    int firstItemIndex,
    int lastItemIndex,
    int weight) const
{
    for (std::shared_ptr<ItemAspectRatioBlock> itemAspectRatioBlock : m_itemAspectRatioBlocks)
    {
        if (itemAspectRatioBlock->HasLowerWeight(firstItemIndex, lastItemIndex, weight))
        {
            LINEDFLOWLAYOUT_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstItemIndex", firstItemIndex);
            LINEDFLOWLAYOUT_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastItemIndex", lastItemIndex);
            LINEDFLOWLAYOUT_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"returns True");

            return true;
        }
    }

    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstItemIndex", firstItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastItemIndex", lastItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"returns False");

    return false;
}

// Returns False when at least one aspect ratio is being tracked.
bool LinedFlowLayoutItemAspectRatios::IsEmpty() const
{
    for (std::shared_ptr<ItemAspectRatioBlock> itemAspectRatioBlock : m_itemAspectRatioBlocks)
    {
        if (itemAspectRatioBlock->StartIndex() != -1)
        {
            // StartIndex() != -1 indicates that this block tracks at least one aspect ratio.
            return false;
        }
    }

    return true;
}

// Returns the average of the weighted aspect ratios. Items outside the provided index range must
// have a weight equal to the provided number to be included.
float LinedFlowLayoutItemAspectRatios::GetAverageAspectRatio(
    int firstItemIndex,
    int lastItemIndex,
    int weight) const
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstItemIndex", firstItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastItemIndex", lastItemIndex);

    MUX_ASSERT(!IsEmpty());

    float totalAspectRatios{ 0.0f };
    int totalWeights{ 0 };

    for (std::shared_ptr<ItemAspectRatioBlock> itemAspectRatioBlock : m_itemAspectRatioBlocks)
    {
        const ItemAspectRatio itemAspectRatioBlockTotals = itemAspectRatioBlock->GetTotals(firstItemIndex, lastItemIndex, weight);

        totalAspectRatios += itemAspectRatioBlockTotals.m_aspectRatio;
        totalWeights += itemAspectRatioBlockTotals.m_weight;
    }

    if (totalAspectRatios > 0.0f && totalWeights > 0)
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"totalAspectRatios", totalAspectRatios);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"totalWeights", totalWeights);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"returned value", totalAspectRatios / totalWeights);

        return totalAspectRatios / totalWeights;
    }

    return 0.0f;
}

// Returns the ItemAspectRatio stored for the provided item index, or s_emptyItemAspectRatio
// when no storage was done for that index.
LinedFlowLayoutItemAspectRatios::ItemAspectRatio LinedFlowLayoutItemAspectRatios::GetAt(
    int itemIndex) const
{
    for (std::shared_ptr<ItemAspectRatioBlock> itemAspectRatioBlock : m_itemAspectRatioBlocks)
    {
        if (itemAspectRatioBlock->IncludesItemIndex(itemIndex))
        {
            return itemAspectRatioBlock->GetAt(itemIndex);
        }
    }

    return s_emptyItemAspectRatio;
}

// Stores the ItemAspectRatio for the specified index.
void LinedFlowLayoutItemAspectRatios::SetAt(int itemIndex, ItemAspectRatio itemAspectRatio)
{
    // Look through the existing blocks to see if one already covers the provided index.
    for (std::shared_ptr<ItemAspectRatioBlock> itemAspectRatioBlock : m_itemAspectRatioBlocks)
    {
        if (itemAspectRatioBlock->IncludesItemIndex(itemIndex))
        {
            // Found a match for the provided index. Use that existing block to store the ItemAspectRatio.
            itemAspectRatioBlock->SetAt(itemIndex, itemAspectRatio);
            return;
        }
    }

    // Get a block to store the ItemAspectRatio. GetEmptyOrFurthestBlock either returns a still empty block or the furthest away block from itemIndex.
    std::shared_ptr<ItemAspectRatioBlock> itemAspectRatioBlockToReuse = GetEmptyOrFurthestBlock(itemIndex);

    MUX_ASSERT(itemAspectRatioBlockToReuse != nullptr);

    if (itemAspectRatioBlockToReuse->StartIndex() != -1)
    {
        // Clear the furthest block from itemIndex.
        itemAspectRatioBlockToReuse->Clear();
    }

    // Use it to store the provided ItemAspectRatio.
    // Blocks are such that m_startIndex has to be a multiple of c_blockSize: 0, 64, 128, etc... to avoid having two blocks covering the same item index.
    itemAspectRatioBlockToReuse->StartIndex((itemIndex / ItemAspectRatioBlock::BlockSize()) * ItemAspectRatioBlock::BlockSize());
    itemAspectRatioBlockToReuse->SetAt(itemIndex, itemAspectRatio);
}

// Keeps 4 cleared blocks at most for future use and deletes all the others.
void LinedFlowLayoutItemAspectRatios::Clear()
{
    const int c_minBlockCount{ 4 };

    // Keep at most c_minBlockCount ItemAspectRatioBlock instances allocated for future use, to avoid reallocations.
    while (m_itemAspectRatioBlocks.size() > c_minBlockCount)
    {
        m_itemAspectRatioBlocks.erase(m_itemAspectRatioBlocks.begin());
    }

    // Each remaining ItemAspectRatioBlock is cleared for future reuse.
    for (std::shared_ptr<ItemAspectRatioBlock> itemAspectRatioBlock : m_itemAspectRatioBlocks)
    {
        itemAspectRatioBlock->Clear();
    }

    MUX_ASSERT(IsEmpty());
}

// Allocates enough blocks to store 'size' ItemAspectRatio instances. When the required number of blocks
// is smaller than the already existing ones, the furthest away block(s) from 'referenceItemIndex' are deleted.
void LinedFlowLayoutItemAspectRatios::Resize(
    int size,
    int referenceItemIndex)
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, size, referenceItemIndex);

    MUX_ASSERT(size >= 1);
    MUX_ASSERT(referenceItemIndex >= 0);

    const int requiredBlocks = static_cast<int>(std::ceil(static_cast<double>(size) / ItemAspectRatioBlock::BlockSize()));
    const int existingBlocks = static_cast<int>(m_itemAspectRatioBlocks.size());

    if (requiredBlocks == existingBlocks)
    {
        return;
    }

    LINEDFLOWLAYOUT_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"size", size);
    LINEDFLOWLAYOUT_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"referenceItemIndex", referenceItemIndex);
    LINEDFLOWLAYOUT_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"requiredBlocks", requiredBlocks);
    LINEDFLOWLAYOUT_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"existingBlocks", existingBlocks);

    if (requiredBlocks > existingBlocks)
    {
        for (int block = existingBlocks + 1; block <= requiredBlocks; block++)
        {
            std::shared_ptr<ItemAspectRatioBlock> newItemAspectRatioBlock =
                std::make_shared<ItemAspectRatioBlock>();

            m_itemAspectRatioBlocks.insert(newItemAspectRatioBlock);
        }
    }
    else
    {
        for (int block = requiredBlocks; block < existingBlocks; block++)
        {
            std::shared_ptr<ItemAspectRatioBlock> itemAspectRatioBlockToRemove = GetEmptyOrFurthestBlock(referenceItemIndex);

            MUX_ASSERT(itemAspectRatioBlockToRemove != nullptr);

            m_itemAspectRatioBlocks.erase(itemAspectRatioBlockToRemove);
        }
    }

    MUX_ASSERT(requiredBlocks == static_cast<int>(m_itemAspectRatioBlocks.size()));
}

// Returns an empty block if one still exists, or else the furthest block away from 'fromItemIndex'.
std::shared_ptr<LinedFlowLayoutItemAspectRatios::ItemAspectRatioBlock> LinedFlowLayoutItemAspectRatios::GetEmptyOrFurthestBlock(
    int fromItemIndex)
{
    std::shared_ptr<ItemAspectRatioBlock> furthestItemAspectRatioBlock{ nullptr };

    for (std::shared_ptr<ItemAspectRatioBlock> itemAspectRatioBlock : m_itemAspectRatioBlocks)
    {
        if (itemAspectRatioBlock->StartIndex() == -1)
        {
            return itemAspectRatioBlock;
        }
        else if (furthestItemAspectRatioBlock == nullptr ||
            std::abs((furthestItemAspectRatioBlock->StartIndex() + furthestItemAspectRatioBlock->EndIndex()) / 2 - fromItemIndex) < std::abs((itemAspectRatioBlock->StartIndex() + itemAspectRatioBlock->EndIndex()) / 2 - fromItemIndex))
        {
            furthestItemAspectRatioBlock = itemAspectRatioBlock;
        }
    }

    return furthestItemAspectRatioBlock;
}

// Returns True when there is at least one stored ratio in the block, within the provided
// index range, with a strictly positive weight lower than the provided number.
bool LinedFlowLayoutItemAspectRatios::ItemAspectRatioBlock::HasLowerWeight(
    int firstItemIndex,
    int lastItemIndex,
    int weight) const
{
    if (m_startIndex == -1)
    {
        return false;
    }

    for (int itemAspectRatioIndex = 0; itemAspectRatioIndex < c_blockSize; itemAspectRatioIndex++)
    {
        const int currentWeight = m_aspectRatios[itemAspectRatioIndex].m_weight;
        const int currentIndex = m_startIndex + itemAspectRatioIndex;

        if (currentWeight > 0 && currentWeight < weight &&
            currentIndex >= firstItemIndex && currentIndex <= lastItemIndex)
        {
            return true;
        }
    }

    return false;
}

bool LinedFlowLayoutItemAspectRatios::ItemAspectRatioBlock::IncludesItemIndex(
    int itemIndex) const
{
    return StartIndex() <= itemIndex && EndIndex() >= itemIndex;
}

// Returns an ItemAspectRatio for which the ratio is the total of the block's weighted ratios, and the weight is the total of
// the block's weights. Items outside the provided index range must have a weight equal to the provided number to be included.
LinedFlowLayoutItemAspectRatios::ItemAspectRatio LinedFlowLayoutItemAspectRatios::ItemAspectRatioBlock::GetTotals(
    int firstItemIndex,
    int lastItemIndex,
    int weight) const
{
    if (m_startIndex == -1)
    {
        return LinedFlowLayoutItemAspectRatios::s_emptyItemAspectRatio;
    }

    float totalAspectRatios{ 0.0f };
    int totalWeights{ 0 };

    for (int itemAspectRatioIndex = 0; itemAspectRatioIndex < c_blockSize; itemAspectRatioIndex++)
    {
        const int currentWeight = m_aspectRatios[itemAspectRatioIndex].m_weight;
        const int currentIndex = m_startIndex + itemAspectRatioIndex;

        if (currentWeight == weight || (currentIndex >= firstItemIndex && currentIndex <= lastItemIndex))
        {
            LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemIndex included", currentIndex);
            LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"weight included", currentWeight);
            LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"aspect ratio included", m_aspectRatios[itemAspectRatioIndex].m_aspectRatio);

            totalAspectRatios += m_aspectRatios[itemAspectRatioIndex].m_aspectRatio * currentWeight;
            totalWeights += currentWeight;
        }
#ifdef DBG
        else if (currentWeight != 0)
        {
            LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemIndex excluded", currentIndex);
            LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"weight excluded", currentWeight);
            LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"aspect ratio excluded", m_aspectRatios[itemAspectRatioIndex].m_aspectRatio);
        }
#endif
    }

    return { totalAspectRatios, totalWeights };
}

// Returns the ItemAspectRatio stored in this block for the provided item index.
LinedFlowLayoutItemAspectRatios::ItemAspectRatio LinedFlowLayoutItemAspectRatios::ItemAspectRatioBlock::GetAt(
    int itemIndex) const
{
    MUX_ASSERT(m_startIndex >= 0);
    MUX_ASSERT(itemIndex >= m_startIndex);
    MUX_ASSERT(itemIndex <= m_startIndex + c_blockSize - 1);

    return m_aspectRatios[itemIndex - m_startIndex];
}

// Sets the ItemAspectRatio in this block for the provided item index.
void LinedFlowLayoutItemAspectRatios::ItemAspectRatioBlock::SetAt(int itemIndex, ItemAspectRatio itemAspectRatio)
{
    MUX_ASSERT(m_startIndex >= 0);
    MUX_ASSERT(itemIndex >= m_startIndex);
    MUX_ASSERT(itemIndex <= m_startIndex + c_blockSize - 1);

    m_aspectRatios[itemIndex - m_startIndex].m_aspectRatio = itemAspectRatio.m_aspectRatio;
    m_aspectRatios[itemIndex - m_startIndex].m_weight = itemAspectRatio.m_weight;
}

// Clears the block for future reuse.
void LinedFlowLayoutItemAspectRatios::ItemAspectRatioBlock::Clear()
{
    m_startIndex = -1;

    for (int itemAspectRatioIndex = 0; itemAspectRatioIndex < c_blockSize; itemAspectRatioIndex++)
    {
        m_aspectRatios[itemAspectRatioIndex].m_aspectRatio = 0.0f;
        m_aspectRatios[itemAspectRatioIndex].m_weight = 0;
    }
}

#ifdef DBG
void LinedFlowLayoutItemAspectRatios::LogItemAspectRatiosDbg()
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Block count", m_itemAspectRatioBlocks.size());

    for (std::shared_ptr<ItemAspectRatioBlock> itemAspectRatioBlock : m_itemAspectRatioBlocks)
    {
        itemAspectRatioBlock->LogItemAspectRatioBlockDbg();
    }
}

void LinedFlowLayoutItemAspectRatios::ItemAspectRatioBlock::LogItemAspectRatioBlockDbg()
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_startIndex", m_startIndex);

    for (int itemAspectRatioIndex = 0; itemAspectRatioIndex < c_blockSize; itemAspectRatioIndex++)
    {
        const float aspectRatio = m_aspectRatios[itemAspectRatioIndex].m_aspectRatio;
        const int weight = m_aspectRatios[itemAspectRatioIndex].m_weight;

        MUX_ASSERT(!(m_startIndex == -1 && aspectRatio != 0.0f));
        MUX_ASSERT(!(m_startIndex == -1 && weight != 0));

        if (weight != 0)
        {
            LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemIndex", m_startIndex + itemAspectRatioIndex);
            LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"aspectRatio", aspectRatio);
            LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"weight", weight);
        }
    }
}
#endif
