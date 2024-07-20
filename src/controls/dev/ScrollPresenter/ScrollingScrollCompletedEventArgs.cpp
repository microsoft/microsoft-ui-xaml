// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollingScrollCompletedEventArgs.h"

int32_t ScrollingScrollCompletedEventArgs::CorrelationId()
{
    return m_offsetsChangeCorrelationId;
}

ScrollPresenterViewChangeResult ScrollingScrollCompletedEventArgs::Result()
{
    return m_result;
}

void ScrollingScrollCompletedEventArgs::OffsetsChangeCorrelationId(int32_t offsetsChangeCorrelationId)
{
    m_offsetsChangeCorrelationId = offsetsChangeCorrelationId;
}

void ScrollingScrollCompletedEventArgs::Result(ScrollPresenterViewChangeResult result)
{
    m_result = result;
}
