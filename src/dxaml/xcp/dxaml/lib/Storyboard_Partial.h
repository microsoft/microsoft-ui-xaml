// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:
//      "Native peer" to the core Storyboard

#pragma once

#include "Storyboard.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(Storyboard)
    {
    public:

        _Check_return_ HRESULT SeekImpl(_In_ wf::TimeSpan offset);
        _Check_return_ HRESULT GetCurrentStateImpl(_Out_ xaml_animation::ClockState* returnValue);
        _Check_return_ HRESULT GetCurrentTimeImpl(_Out_ wf::TimeSpan* returnValue);
        _Check_return_ HRESULT SeekAlignedToLastTickImpl(_In_ wf::TimeSpan offset);

    private:
        // Represents the number of ticks in 1 second. Used for GetCurrentTime's TimeSpan calculation.
        static constexpr UINT32 TicksPerSecond = 10000000;
    };
};
