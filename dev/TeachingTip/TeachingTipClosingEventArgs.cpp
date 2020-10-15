#include "pch.h"
#include "common.h"
#include "TeachingTipClosingEventArgs.h"

winrt::TeachingTipCloseReason TeachingTipClosingEventArgs::Reason()
{
    return m_reason;
}

void TeachingTipClosingEventArgs::Reason(const winrt::TeachingTipCloseReason& reason)
{
    m_reason = reason;
}

bool TeachingTipClosingEventArgs::Cancel()
{
    return m_cancel;
}

void TeachingTipClosingEventArgs::Cancel(const bool cancel)
{
    m_cancel = cancel;
}

winrt::Deferral TeachingTipClosingEventArgs::GetDeferral()
{
    m_deferralCount++;

    com_ptr<TeachingTipClosingEventArgs> strongThis = get_strong();

    winrt::Deferral instance{ [strongThis]()
    {
        strongThis->CheckThread();
        strongThis->DecrementDeferralCount();
    }
    };
    return instance;
}

void TeachingTipClosingEventArgs::SetDeferral(const winrt::Deferral& deferral)
{
    m_deferral.set(deferral);
}

void TeachingTipClosingEventArgs::DecrementDeferralCount()
{
    MUX_ASSERT(m_deferralCount > 0);
    m_deferralCount--;
    if (m_deferralCount == 0)
    {
        m_deferral.get().Complete();
    }
}

void TeachingTipClosingEventArgs::IncrementDeferralCount()
{
    m_deferralCount++;
}
