#pragma once

#include "TeachingTipClosedEventArgs.g.h"

class TeachingTipClosedEventArgs :
    public winrt::implementation::TeachingTipClosedEventArgsT<TeachingTipClosedEventArgs>
{
public:
    TeachingTipClosedEventArgs();

    winrt::TeachingTipCloseReason Reason();
    void Reason(const winrt::TeachingTipCloseReason& reason);
private:

    winrt::TeachingTipCloseReason m_reason{ winrt::TeachingTipCloseReason::CloseButton };
};
