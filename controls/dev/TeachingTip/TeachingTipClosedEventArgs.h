// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TeachingTipClosedEventArgs.g.h"

class TeachingTipClosedEventArgs :
    public winrt::implementation::TeachingTipClosedEventArgsT<TeachingTipClosedEventArgs>
{
public:
    winrt::TeachingTipCloseReason Reason();
    void Reason(const winrt::TeachingTipCloseReason& reason);
private:

    winrt::TeachingTipCloseReason m_reason{ winrt::TeachingTipCloseReason::CloseButton };
};
