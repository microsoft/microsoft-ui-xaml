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

winrt::Deferral InfoBarClosingEventArgs::GetDeferral()
{
    m_deferralCount++;

    com_ptr<InfoBarClosingEventArgs> strongThis = get_strong();

    winrt::Deferral instance{ [strongThis]()
    {
        strongThis->CheckThread();
        strongThis->DecrementDeferralCount();
    }
    };
    return instance;
}

void InfoBarClosingEventArgs::SetDeferral(const winrt::Deferral& deferral)
{
    m_deferral.set(deferral);
}

void InfoBarClosingEventArgs::DecrementDeferralCount()
{
    MUX_ASSERT(m_deferralCount > 0);
    m_deferralCount--;
    if (m_deferralCount == 0)
    {
        m_deferral.get().Complete();
    }
}

void InfoBarClosingEventArgs::IncrementDeferralCount()
{
    m_deferralCount++;
}
