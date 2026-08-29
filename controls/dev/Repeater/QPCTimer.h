// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// High performance time management using QueryPerformanceCounter
class QPCTimer final
{
public:
    QPCTimer();
    void Reset();
    int DurationInMilliSeconds() const;

private:
    LARGE_INTEGER m_start;
    LARGE_INTEGER m_frequency;
};