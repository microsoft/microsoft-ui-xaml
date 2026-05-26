// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollingZoomCompletedEventArgs.h"

int32_t ScrollingZoomCompletedEventArgs::CorrelationId()
{
    return m_zoomFactorChangeCorrelationId;
}

ScrollPresenterViewChangeResult ScrollingZoomCompletedEventArgs::Result()
{
    return m_result;
}

void ScrollingZoomCompletedEventArgs::ZoomFactorChangeCorrelationId(int32_t zoomFactorChangeCorrelationId)
{
    m_zoomFactorChangeCorrelationId = zoomFactorChangeCorrelationId;
}

void ScrollingZoomCompletedEventArgs::Result(ScrollPresenterViewChangeResult result)
{
    m_result = result;
}
