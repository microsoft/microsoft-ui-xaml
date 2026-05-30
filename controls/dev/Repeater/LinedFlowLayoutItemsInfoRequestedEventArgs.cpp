// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "LinedFlowLayout.h"
#include "LinedFlowLayoutItemsInfoRequestedEventArgs.h"
#include "LinedFlowLayoutTrace.h"

LinedFlowLayoutItemsInfoRequestedEventArgs::LinedFlowLayoutItemsInfoRequestedEventArgs(
    winrt::LinedFlowLayout const& linedFlowLayout,
    int itemsRangeStartIndex,
    int itemsRangeRequestedLength) :
    m_itemsRangeStartIndex(itemsRangeStartIndex),
    m_itemsRangeRequestedStartIndex(itemsRangeStartIndex),
    m_itemsRangeRequestedLength(itemsRangeRequestedLength)
{
    MUX_ASSERT(itemsRangeStartIndex >= 0);
    MUX_ASSERT(itemsRangeRequestedLength > 0);

    LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, itemsRangeStartIndex, itemsRangeRequestedLength);

    m_linedFlowLayout.set(linedFlowLayout);
}

LinedFlowLayoutItemsInfoRequestedEventArgs::~LinedFlowLayoutItemsInfoRequestedEventArgs()
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

#pragma region ILinedFlowLayoutItemsInfoRequestedEventArgs

void LinedFlowLayoutItemsInfoRequestedEventArgs::ItemsRangeStartIndex(int32_t value)
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, value);

    if (value < 0)
    {
        throw winrt::hresult_invalid_argument(L"'ItemsRangeStartIndex' must be positive.");
    }

    if (value > m_itemsRangeStartIndex)
    {
        throw winrt::hresult_invalid_argument(L"'ItemsRangeStartIndex' cannot be increased.");
    }

    if (m_itemsRangeEstablishedLength != 0 && value + m_itemsRangeEstablishedLength < m_itemsRangeRequestedStartIndex + m_itemsRangeRequestedLength)
    {
        throw winrt::hresult_invalid_argument(L"Value is too small given the array length already provided.");
    }

    m_itemsRangeStartIndex = value;
}

void LinedFlowLayoutItemsInfoRequestedEventArgs::MinWidth(double value)
{
    if (value < 0.0)
    {
        m_minWidth = -1.0;
    }
    else
    {
        m_minWidth = value;
    }
}

void LinedFlowLayoutItemsInfoRequestedEventArgs::MaxWidth(double value)
{
    if (value < 0.0)
    {
        m_maxWidth = -1.0;
    }
    else
    {
        m_maxWidth = value;
    }
}

void LinedFlowLayoutItemsInfoRequestedEventArgs::SetDesiredAspectRatios(winrt::array_view<double const> const& values)
{
    if (auto linedFlowLayout = winrt::get_self<LinedFlowLayout>(m_linedFlowLayout.get()))
    {
        SetItemsRangeEstablishedLength(values.size());

        m_itemsRangeLength = m_itemsRangeEstablishedLength;

        linedFlowLayout->SetDesiredAspectRatios(values);
    }
}

void LinedFlowLayoutItemsInfoRequestedEventArgs::SetMinWidths(winrt::array_view<double const> const& values)
{
    if (auto linedFlowLayout = winrt::get_self<LinedFlowLayout>(m_linedFlowLayout.get()))
    {
        SetItemsRangeEstablishedLength(values.size());

        linedFlowLayout->SetMinWidths(values);
    }
}

void LinedFlowLayoutItemsInfoRequestedEventArgs::SetMaxWidths(winrt::array_view<double const> const& values)
{
    if (auto linedFlowLayout = winrt::get_self<LinedFlowLayout>(m_linedFlowLayout.get()))
    {
        SetItemsRangeEstablishedLength(values.size());

        linedFlowLayout->SetMaxWidths(values);
    }
}

void LinedFlowLayoutItemsInfoRequestedEventArgs::SetItemsRangeEstablishedLength(int value)
{
    if (value != m_itemsRangeEstablishedLength)
    {
        if (value < m_itemsRangeRequestedLength && m_itemsRangeStartIndex == m_itemsRangeRequestedStartIndex)
        {
            throw winrt::hresult_invalid_argument(L"The provided array length must be greater than or equal to 'ItemsRangeRequestedLength'.");
        }

        if (m_itemsRangeStartIndex + value < m_itemsRangeRequestedStartIndex + m_itemsRangeRequestedLength && m_itemsRangeStartIndex < m_itemsRangeRequestedStartIndex)
        {
            throw winrt::hresult_invalid_argument(L"The provided array length is too small given the decreased 'ItemsRangeStartIndex' and the 'ItemsRangeRequestedLength' values.");
        }

        if (m_itemsRangeEstablishedLength > 0)
        {
            throw winrt::hresult_invalid_argument(L"All provided arrays must have the same length.");
        }

        m_itemsRangeEstablishedLength = value;
    }
}

#pragma endregion
