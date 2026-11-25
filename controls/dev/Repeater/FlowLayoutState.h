// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FlowLayoutState.g.h"
#include "FlowLayoutAlgorithm.h"

class FlowLayoutState :
    public ReferenceTracker<FlowLayoutState, winrt::implementation::FlowLayoutStateT, winrt::composing>
{
public:
    void InitializeForContext(
        const winrt::VirtualizingLayoutContext& context,
        IFlowLayoutAlgorithmDelegates* callbacks);
    void UninitializeForContext(const winrt::VirtualizingLayoutContext& context);
    void OnLineArranged(int startIndex, int countInLine, double lineSize, const winrt::VirtualizingLayoutContext& context);

    ::FlowLayoutAlgorithm& FlowAlgorithm() { return m_flowAlgorithm; }
    double TotalLineSize() const { return m_totalLineSize; }
    int TotalLinesMeasured() const { return m_totalLinesMeasured; }
    double TotalItemsPerLine() const { return m_totalItemsPerLine; }

    winrt::Size SpecialElementDesiredSize() const { return m_specialElementDesiredSize; }
    void SpecialElementDesiredSize(winrt::Size value) { m_specialElementDesiredSize = value; }

private:
    ::FlowLayoutAlgorithm m_flowAlgorithm{ this };
    std::vector<double> m_lineSizeEstimationBuffer{};
    std::vector<double> m_itemsPerLineEstimationBuffer{};
    double m_totalLineSize{};
    int m_totalLinesMeasured{};
    double m_totalItemsPerLine{};
    winrt::Size m_specialElementDesiredSize{};
    static const int BufferSize = 100;

    // Sentinel for slots that have not been measured yet. It cannot be 0.0, which is a size a line made up entirely of
    // collapsed elements legitimately reports: such a slot would then be counted as a newly measured line on every
    // re-arrange, inflating m_totalLinesMeasured while adding nothing to m_totalLineSize.
    static constexpr double c_unmeasuredLineSize = -1.0;
};