#include "pch.h"
#include "TeachingTipClosedEventArgs.h"
#include "common.h"


TeachingTipClosedEventArgs::TeachingTipClosedEventArgs()
= default;

winrt::TeachingTipCloseReason TeachingTipClosedEventArgs::Reason()
{
    return m_reason;
}

void TeachingTipClosedEventArgs::Reason(const winrt::TeachingTipCloseReason& reason)
{
    m_reason = reason;
}