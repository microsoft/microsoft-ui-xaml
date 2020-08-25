#pragma once
#pragma once

#include "InfoBarClosingEventArgs.g.h"

class InfoBarClosingEventArgs :
    public ReferenceTracker<InfoBarClosingEventArgs, winrt::implementation::InfoBarClosingEventArgsT, winrt::composable, winrt::composing>
{
public:
    winrt::InfoBarCloseReason Reason();
    void Reason(const winrt::InfoBarCloseReason& reason);
    bool Cancel();
    void Cancel(const bool cancel);

    winrt::Deferral GetDeferral();
    void SetDeferral(const winrt::Deferral& deferral);

    void DecrementDeferralCount();
    void IncrementDeferralCount();

private:
    winrt::InfoBarCloseReason m_reason{ winrt::InfoBarCloseReason::CloseButton };
    bool m_cancel{ false };
    tracker_ref<winrt::Deferral> m_deferral{ this };
    int m_deferralCount{ 0 };
};
