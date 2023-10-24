// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "FlowLayoutAlgorithm.h"
#include "FlowLayoutState.h"

#include "FlowLayoutState.properties.cpp"

void FlowLayoutState::InitializeForContext(
    const winrt::VirtualizingLayoutContext& context,
    IFlowLayoutAlgorithmDelegates* callbacks)
{
    m_flowAlgorithm.InitializeForContext(context, callbacks);
    
    if (m_lineSizeEstimationBuffer.size() == 0)
    {
        m_lineSizeEstimationBuffer.resize(BufferSize, 0.0f);
        m_itemsPerLineEstimationBuffer.resize(BufferSize, 0.0f);
    }

    context.LayoutStateCore(*this);
}

void FlowLayoutState::UninitializeForContext(const winrt::VirtualizingLayoutContext& context)
{
    m_flowAlgorithm.UninitializeForContext(context);
}

void FlowLayoutState::OnLineArranged(int startIndex, int countInLine, double lineSize, const winrt::VirtualizingLayoutContext& context)
{
    // If we do not have any estimation information, use the line for estimation. 
    // If we do have some estimation information, don't account for the last line which is quite likely
    // different from the rest of the lines and can throw off estimation.
    if (m_totalLinesMeasured == 0 || startIndex + countInLine != context.ItemCount())
    {
        const int estimationBufferIndex = startIndex % m_lineSizeEstimationBuffer.size();
        const bool alreadyMeasured = m_lineSizeEstimationBuffer[estimationBufferIndex] != 0;

        if (!alreadyMeasured)
        {
            ++m_totalLinesMeasured;
        }

        m_totalLineSize -= m_lineSizeEstimationBuffer[estimationBufferIndex];
        m_totalLineSize += lineSize;
        m_lineSizeEstimationBuffer[estimationBufferIndex] = lineSize;

        m_totalItemsPerLine -= m_itemsPerLineEstimationBuffer[estimationBufferIndex];
        m_totalItemsPerLine += countInLine;
        m_itemsPerLineEstimationBuffer[estimationBufferIndex] = countInLine;
    }
}
