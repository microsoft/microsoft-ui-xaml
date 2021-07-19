#include "pch.h"
#include "common.h"
#include "InfoBarClosedEventArgs.h"

winrt::InfoBarCloseReason InfoBarClosedEventArgs::Reason()
{
    return m_reason;
}

void InfoBarClosedEventArgs::Reason(const winrt::InfoBarCloseReason& reason)
{
    m_reason = reason;
}
