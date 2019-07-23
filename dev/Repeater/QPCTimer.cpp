// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "QPCTimer.h"
#include "common.h"
#include <windows.h>

QPCTimer::QPCTimer()
{
    if ((QueryPerformanceFrequency(&m_frequency) == 0) || (QueryPerformanceCounter(&m_start) == 0))
    {
        throw winrt::hresult_error(E_FAIL);
    }
}

void QPCTimer::Reset()
{
    QueryPerformanceCounter(&m_start);
}

int QPCTimer::DurationInMilliSeconds() const
{
    LARGE_INTEGER now;
    auto success = QueryPerformanceCounter(&now);
    int elapsedMilliSeconds = 0;

    if (success != 0)
    {
        double elapsedSeconds = static_cast<DOUBLE>(now.QuadPart - m_start.QuadPart) / static_cast<DOUBLE>(m_frequency.QuadPart);
        elapsedMilliSeconds = static_cast<int>(elapsedSeconds * 1000);
    }
    else
    {
        // Hardware that doesn't understand QPC?? Very unlikely, but in this case, let's not crash
        // and just return that no time has elapsed. This will trigger people relying on budget to see that they
        // have a lot of budget and atleast not have them encounter a situation where they do not get to perform
        // certain work.
    }

    return elapsedMilliSeconds;
}