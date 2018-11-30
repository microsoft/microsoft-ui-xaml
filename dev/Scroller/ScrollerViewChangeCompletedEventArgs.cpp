// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollerTrace.h"
#include "ScrollerViewChangeCompletedEventArgs.h"

int32_t ScrollerViewChangeCompletedEventArgs::ViewChangeId()
{
    return m_viewChangeId;
}

winrt::ScrollerViewChangeResult ScrollerViewChangeCompletedEventArgs::Result()
{
    return m_result;
}

void ScrollerViewChangeCompletedEventArgs::ViewChangeId(int32_t viewChangeId)
{
    m_viewChangeId = viewChangeId;
}

void ScrollerViewChangeCompletedEventArgs::Result(const winrt::ScrollerViewChangeResult& result)
{
    m_result = result;
}
