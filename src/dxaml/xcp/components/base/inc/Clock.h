// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <chrono>

namespace Jupiter
{
    // Wrap QueryPerformanceCounter in a std::chrono-compatible interface
    // Once we get the UCRT, we should be able to use std::chrono::high_resolution_clock instead.

    // No longer are subject to the tyranny of converting QueryPerformanceFrequency everywhere
    // Jupiter::HighResolutionClock::time_point before = Jupiter::HighResolutionClock::now();
    // (do stuff)
    // Jupiter::HighResolutionClock::time_point now = Jupiter::HighResolutionClock::now();
    // 
    // Then, to check if 1 second has elapsed:
    // if (now - before >= std::chrono::seconds(1))
    // To check if 50 milliseconds have elapsed:
    // if (now - before >= std::chrono::milliseconds(50))
    struct HighResolutionClock
    {
        using rep = long long;
        using period = std::nano;
        using duration = std::chrono::nanoseconds;
        using time_point = std::chrono::time_point<HighResolutionClock>;
        static constexpr bool is_steady = true;

        static time_point now()
        {
            const auto frequency = GetFrequency();
            const auto counter = GetCounter();
            static_assert(period::num == 1, "This simple math assumes period::num == 1");

            const long long whole = (counter / frequency) * period::den;
            const long long part = (counter % frequency) * period::den / frequency;
            return time_point(duration(whole + part));
        }

    private:
        static long long GetCounter();
        static long long GetFrequency();

        // Until we have magic statics, we'll do the slightly ugly thing of
        // initializing this in a member variable's dynamic initializer
        static const long long m_frequency;
    };
}

typedef std::ratio_multiply<std::ratio<100>, std::nano>::type TimeSpanTickPeriod;
typedef std::chrono::duration<decltype(wf::TimeSpan::Duration), TimeSpanTickPeriod> TimeSpanDuration; // 100ns increments

// To convert from seconds to TimeSpan: std::chrono::duration_cast<TimeSpanDuration>(std::chrono::duration<double>(seconds)).count() 
// To convert from TimeSpan to seconds: std::chrono::duration_cast<std::chrono::duration<double>>(TimeSpanDuration(ts.Duration)).count();
// The helper methods below do this for you.
namespace TimeSpanUtil
{
    double ToSeconds(const wf::TimeSpan& ts);
    wf::TimeSpan FromSeconds(double seconds);
}