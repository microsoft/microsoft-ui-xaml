// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Clock.h"
#include <Windows.h>

namespace Jupiter
{
    long long HighResolutionClock::GetFrequency()
    {
        static const long long frequency = []
        {
            LARGE_INTEGER li;
            ::QueryPerformanceFrequency(&li);
            return li.QuadPart;
        }();
        return frequency;
    }

    long long HighResolutionClock::GetCounter()
    {
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