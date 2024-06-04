// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Clock.h"
#include <Windows.h>

namespace Jupiter
{
    // Until we have magic statics, we'll do the slightly ugly thing of
    // initializing this in a member variable's dynamic initializer
    const long long HighResolutionClock::m_frequency = []
    {
        // QueryPerformanceFrequency is guaranteed to succeed on WinXP and later
        // So, we don't need to check for failure
        static_assert(NTDDI_VERSION >= NTDDI_WINXP, "Can't build for pre-XP");
        LARGE_INTEGER li;
        ::QueryPerformanceFrequency(&li);
        return li.QuadPart;
    }();

    long long HighResolutionClock::GetFrequency()
    {
        return m_frequency;
    }

    long long HighResolutionClock::GetCounter()
    {
        // QueryPerformanceFrequency is guaranteed to succeed on WinXP and later
        // So, we don't need to check for failure
        static_assert(NTDDI_VERSION >= NTDDI_WINXP, "Can't build for pre-XP");
        LARGE_INTEGER li;
        ::QueryPerformanceCounter(&li);
        return li.QuadPart;
    }
}

namespace TimeSpanUtil
{
    double ToSeconds(const wf::TimeSpan& ts)
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(TimeSpanDuration(ts.Duration)).count();
    }

    wf::TimeSpan FromSeconds(double seconds)
    {
        return wf::TimeSpan{std::chrono::duration_cast<TimeSpanDuration>(std::chrono::duration<double>(seconds)).count() };
    }
}