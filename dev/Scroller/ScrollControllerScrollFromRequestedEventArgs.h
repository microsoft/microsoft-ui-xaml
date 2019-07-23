// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollControllerScrollFromRequestedEventArgs.g.h"

class ScrollControllerScrollFromRequestedEventArgs :
    public winrt::implementation::ScrollControllerScrollFromRequestedEventArgsT<ScrollControllerScrollFromRequestedEventArgs>
{
public:
    ~ScrollControllerScrollFromRequestedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollControllerScrollFromRequestedEventArgs(
        float offsetVelocity,
        winrt::IReference<float> inertiaDecayRate);

    [[nodiscard]] float OffsetVelocity() const;
    [[nodiscard]] winrt::IReference<float> InertiaDecayRate() const;
    [[nodiscard]] winrt::ScrollInfo Info() const;
    void Info(winrt::ScrollInfo info);

private:
    float m_offsetVelocity{ 0.0F };
    winrt::IReference<float> m_inertiaDecayRate{};
    winrt::ScrollInfo m_info{ -1 };
};
