// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#pragma once

#include "TeachingTipClosingEventArgs.g.h"

class TeachingTipClosingEventArgs :
    public ReferenceTracker<TeachingTipClosingEventArgs, winrt::implementation::TeachingTipClosingEventArgsT, winrt::composable, winrt::composing>
{
public:
    winrt::TeachingTipCloseReason Reason();
    void Reason(const winrt::TeachingTipCloseReason& reason);
    bool Cancel();
    void Cancel(const bool cancel);

    winrt::Deferral GetDeferral();
    void SetDeferral(const winrt::Deferral& deferral);

    void DecrementDeferralCount();
    void IncrementDeferralCount();
private:

    winrt::TeachingTipCloseReason m_reason{ winrt::TeachingTipCloseReason::CloseButton };
    bool m_cancel{ false };
    tracker_ref<winrt::Deferral> m_deferral{ this };
    int m_deferralCount{ 0 };
};
