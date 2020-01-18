// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollingScrollCompletedEventArgs.h"

int ScrollingScrollCompletedEventArgs::ScrollInfo()
{
    return m_offsetsChangeId;
}

ScrollPresenterViewChangeResult ScrollingScrollCompletedEventArgs::Result()
{
    return m_result;
}

void ScrollingScrollCompletedEventArgs::OffsetsChangeId(int32_t offsetsChangeId)
{
    m_offsetsChangeId = offsetsChangeId;
}

void ScrollingScrollCompletedEventArgs::Result(ScrollPresenterViewChangeResult result)
{
    m_result = result;
}
