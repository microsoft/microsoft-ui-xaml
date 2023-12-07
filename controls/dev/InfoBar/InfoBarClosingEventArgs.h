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

private:
    winrt::InfoBarCloseReason m_reason{ winrt::InfoBarCloseReason::CloseButton };
    bool m_cancel{ false };
};
