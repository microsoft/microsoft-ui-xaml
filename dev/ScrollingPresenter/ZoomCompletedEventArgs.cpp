// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ZoomCompletedEventArgs.h"

winrt::ZoomInfo ZoomCompletedEventArgs::ZoomInfo()
{
    return winrt::ZoomInfo{ m_zoomFactorChangeId };
}

ScrollerViewChangeResult ZoomCompletedEventArgs::Result()
{
    return m_result;
}

void ZoomCompletedEventArgs::ZoomFactorChangeId(int32_t zoomFactorChangeId)
{
    m_zoomFactorChangeId = zoomFactorChangeId;
}

void ZoomCompletedEventArgs::Result(ScrollerViewChangeResult result)
{
    m_result = result;
}
