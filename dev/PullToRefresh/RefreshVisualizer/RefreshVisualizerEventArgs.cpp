// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "RefreshVisualizer.h"
#include "RefreshVisualizerEventArgs.h"
#include "PTRTracing.h"

//////////////////////////////////////////////////////////
/////////        RefreshStateChanged        /////////////
//////////////////////////////////////////////////////////

RefreshStateChangedEventArgs::RefreshStateChangedEventArgs(winrt::RefreshVisualizerState oldValue, winrt::RefreshVisualizerState newValue)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    m_oldState = oldValue;
    m_newState = newValue;
}

winrt::RefreshVisualizerState RefreshStateChangedEventArgs::OldState()
{
    return m_oldState;
}

winrt::RefreshVisualizerState RefreshStateChangedEventArgs::NewState()
{
    return m_newState;
}

//////////////////////////////////////////////////////////
/////////          RefreshRequested          /////////////
//////////////////////////////////////////////////////////
RefreshRequestedEventArgs::RefreshRequestedEventArgs(const winrt::Deferral& handler)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    m_deferral.set(handler);
}

winrt::Deferral RefreshRequestedEventArgs::GetDeferral()
{
    m_deferralCount++;

    com_ptr<RefreshRequestedEventArgs> strongThis = get_strong();

    winrt::Deferral instance{ [strongThis]()
        {
            strongThis->CheckThread();
            strongThis->DecrementDeferralCount();
        }
    };
    return instance;
}

void RefreshRequestedEventArgs::DecrementDeferralCount()
{
    MUX_ASSERT(m_deferralCount >= 0);
    m_deferralCount--;
    if (m_deferralCount == 0)
    {
        m_deferral.get().Complete();
    }
}

void RefreshRequestedEventArgs::IncrementDeferralCount()
{
    m_deferralCount++;
}


