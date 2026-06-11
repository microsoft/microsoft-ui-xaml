#include "pch.h"
#include "common.h"
#include "InfoBarClosingEventArgs.h"

winrt::InfoBarCloseReason InfoBarClosingEventArgs::Reason()
{
    return m_reason;
}

void InfoBarClosingEventArgs::Reason(const winrt::InfoBarCloseReason& reason)
{
    m_reason = reason;
}

bool InfoBarClosingEventArgs::Cancel()
{
    return m_cancel;
}

void InfoBarClosingEventArgs::Cancel(const bool cancel)
{
    m_cancel = cancel;
}
