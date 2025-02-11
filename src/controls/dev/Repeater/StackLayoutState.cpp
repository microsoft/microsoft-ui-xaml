// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "FlowLayoutAlgorithm.h"
#include "StackLayoutState.h"

#include "StackLayoutState.properties.cpp"

void StackLayoutState::InitializeForContext(
    const winrt::VirtualizingLayoutContext& context,
    IFlowLayoutAlgorithmDelegates* callbacks)
{
    m_flowAlgorithm.InitializeForContext(context, callbacks);
    if (m_estimationBuffer.size() == 0)
    {
        m_estimationBuffer.resize(BufferSize, 0.0);
    }

    context.LayoutStateCore(*this);
}

void StackLayoutState::UninitializeForContext(const winrt::VirtualizingLayoutContext& context)
{
    m_flowAlgorithm.UninitializeForContext(context);
}

void StackLayoutState::OnElementMeasured(int elementIndex, double majorSize, double minorSize)
{
    const int estimationBufferIndex = elementIndex % m_estimationBuffer.size();
    const bool alreadyMeasured = m_estimationBuffer[estimationBufferIndex] != 0;
    const double lastElementSize = m_lastElementSize;

    if (!alreadyMeasured)
    {
        m_totalElementsMeasured++;
    }

    m_totalElementSize -= m_estimationBuffer[estimationBufferIndex];
    m_totalElementSize += majorSize;
    m_lastElementSize = majorSize;
    m_estimationBuffer[estimationBufferIndex] = majorSize;

    if (m_areElementsMeasuredRegular && lastElementSize != 0.0 && lastElementSize != m_lastElementSize)
    {
        // Elements in the StackLayout are declared irregular as two elements have different desired major sizes.
        m_areElementsMeasuredRegular = false;
    }

    m_maxArrangeBounds = std::max(m_maxArrangeBounds, minorSize);
}

void StackLayoutState::OnMeasureStart()
{
    m_maxArrangeBounds = 0.0;
}

// Invoked when the StackLayout's source is reset with a NotifyCollectionChangedAction::Reset.
void StackLayoutState::OnElementSizesReset()
{
    // Assume the new elements are regular again.
    m_areElementsMeasuredRegular = true;
    m_totalElementsMeasured = 0;
    m_totalElementSize = 0.0;
    m_lastElementSize = 0.0;
    m_estimationBuffer.clear();
    m_estimationBuffer.resize(BufferSize, 0.0);
}
