#pragma once

#include "InfoBarClosedEventArgs.g.h"

class InfoBarClosedEventArgs :
    public winrt::implementation::InfoBarClosedEventArgsT<InfoBarClosedEventArgs>
{
public:
    winrt::InfoBarCloseReason Reason();
    void Reason(const winrt::InfoBarCloseReason& reason);

private:
    winrt::InfoBarCloseReason m_reason{ winrt::InfoBarCloseReason::CloseButton };
};
