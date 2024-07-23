// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollingViewChangingEventArgs.h"

double ScrollingViewChangingEventArgs::HorizontalOffset() const
{
    return m_horizontalOffset;
}

void ScrollingViewChangingEventArgs::SetHorizontalOffset(double horizontalOffset)
{
    m_horizontalOffset = horizontalOffset;
}

double ScrollingViewChangingEventArgs::VerticalOffset() const
{
    return m_verticalOffset;
}

void ScrollingViewChangingEventArgs::SetVerticalOffset(double verticalOffset)
{
    m_verticalOffset = verticalOffset;
}

float ScrollingViewChangingEventArgs::ZoomFactor() const
{
    return m_zoomFactor;
}

void ScrollingViewChangingEventArgs::SetZoomFactor(float zoomFactor)
{
    m_zoomFactor = zoomFactor;
}

#ifdef DBG
int32_t ScrollingViewChangingEventArgs::CorrelationIdDbg() const
{
    return m_correlationIdDbg;
}

void ScrollingViewChangingEventArgs::SetCorrelationIdDbg(int32_t correlationIdDbg)
{
    m_correlationIdDbg = correlationIdDbg;
}
#endif // DBG
