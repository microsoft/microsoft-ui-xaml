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
    [[nodiscard]] double TotalLineSize() const { return m_totalLineSize; }
    [[nodiscard]] int TotalLinesMeasured() const { return m_totalLinesMeasured; }
    [[nodiscard]] double TotalItemsPerLine() const { return m_totalItemsPerLine; }

    [[nodiscard]] winrt::Size SpecialElementDesiredSize() const { return m_specialElementDesiredSize; }
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
};