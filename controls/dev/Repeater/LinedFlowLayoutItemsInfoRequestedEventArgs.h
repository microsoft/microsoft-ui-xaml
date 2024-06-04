// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LinedFlowLayoutItemsInfoRequestedEventArgs.g.h"

class LinedFlowLayoutItemsInfoRequestedEventArgs :
    public ReferenceTracker<LinedFlowLayoutItemsInfoRequestedEventArgs, winrt::implementation::LinedFlowLayoutItemsInfoRequestedEventArgsT, winrt::composable, winrt::composing>
{
public:
    LinedFlowLayoutItemsInfoRequestedEventArgs(
        winrt::LinedFlowLayout const& linedFlowLayout,
        int itemsRangeStartIndex,
        int itemsRangeRequestedLength);

    ~LinedFlowLayoutItemsInfoRequestedEventArgs();

#pragma region ILinedFlowLayoutItemsInfoRequestedEventArgs
    int32_t ItemsRangeStartIndex() { return m_itemsRangeStartIndex; }
    int32_t ItemsRangeRequestedLength() { return m_itemsRangeRequestedLength; }
    double MinWidth() { return m_minWidth; }
    double MaxWidth() { return m_maxWidth; }
    void ItemsRangeStartIndex(int32_t value);
    void MinWidth(double value);
    void MaxWidth(double value);
    void SetDesiredAspectRatios(winrt::array_view<double const> const& values);
    void SetMinWidths(winrt::array_view<double const> const& values);
    void SetMaxWidths(winrt::array_view<double const> const& values);
#pragma endregion

    int ItemsRangeLength() { return m_itemsRangeLength; }
    void ResetLinedFlowLayout() { m_linedFlowLayout.set(nullptr); };

private:
    void SetItemsRangeEstablishedLength(int value);

    int m_itemsRangeStartIndex{};
    int m_itemsRangeRequestedStartIndex{};
    int m_itemsRangeEstablishedLength{};
    int m_itemsRangeLength{};
    int m_itemsRangeRequestedLength{};
    double m_minWidth{ -1.0 };
    double m_maxWidth{ -1.0 };
    tracker_ref<winrt::LinedFlowLayout> m_linedFlowLayout{ this };
};
