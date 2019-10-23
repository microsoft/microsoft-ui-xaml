// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollingZoomCompletedEventArgs.h"

winrt::ScrollingZoomInfo ScrollingZoomCompletedEventArgs::ZoomInfo()
{
    return winrt::ScrollingZoomInfo{ m_zoomFactorChangeId };
}

ScrollingPresenterViewChangeResult ScrollingZoomCompletedEventArgs::Result()
{
    return m_result;
}

void ScrollingZoomCompletedEventArgs::ZoomFactorChangeId(int32_t zoomFactorChangeId)
{
    m_zoomFactorChangeId = zoomFactorChangeId;
}

void ScrollingZoomCompletedEventArgs::Result(ScrollingPresenterViewChangeResult result)
{
    m_result = result;
}
