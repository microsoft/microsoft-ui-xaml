// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollingZoomCompletedEventArgs.h"

winrt::ZoomInfo ScrollingZoomCompletedEventArgs::ZoomInfo()
{
    return winrt::ZoomInfo{ m_zoomFactorChangeId };
}

ScrollPresenterViewChangeResult ScrollingZoomCompletedEventArgs::Result()
{
    return m_result;
}

void ScrollingZoomCompletedEventArgs::ZoomFactorChangeId(int32_t zoomFactorChangeId)
{
    m_zoomFactorChangeId = zoomFactorChangeId;
}

void ScrollingZoomCompletedEventArgs::Result(ScrollPresenterViewChangeResult result)
{
    m_result = result;
}
