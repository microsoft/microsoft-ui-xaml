// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollCompletedEventArgs.h"

winrt::ScrollingScrollInfo ScrollCompletedEventArgs::ScrollInfo()
{
    return winrt::ScrollingScrollInfo{ m_offsetsChangeId };
}

ScrollingPresenterViewChangeResult ScrollCompletedEventArgs::Result()
{
    return m_result;
}

void ScrollCompletedEventArgs::OffsetsChangeId(int32_t offsetsChangeId)
{
    m_offsetsChangeId = offsetsChangeId;
}

void ScrollCompletedEventArgs::Result(ScrollingPresenterViewChangeResult result)
{
    m_result = result;
}
