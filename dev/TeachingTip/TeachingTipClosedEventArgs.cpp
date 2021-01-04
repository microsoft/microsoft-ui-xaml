// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TeachingTipClosedEventArgs.h"

winrt::TeachingTipCloseReason TeachingTipClosedEventArgs::Reason()
{
    return m_reason;
}

void TeachingTipClosedEventArgs::Reason(const winrt::TeachingTipCloseReason& reason)
{
    m_reason = reason;
}
