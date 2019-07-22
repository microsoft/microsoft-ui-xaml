#include "pch.h"
#include "common.h"
#include "TeachingTipClosedEventArgs.h"


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