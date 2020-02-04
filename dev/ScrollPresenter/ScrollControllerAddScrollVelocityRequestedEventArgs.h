// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollControllerAddScrollVelocityRequestedEventArgs.g.h"

class ScrollControllerAddScrollVelocityRequestedEventArgs :
    public winrt::implementation::ScrollControllerAddScrollVelocityRequestedEventArgsT<ScrollControllerAddScrollVelocityRequestedEventArgs>
{
public:
    ~ScrollControllerAddScrollVelocityRequestedEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollControllerAddScrollVelocityRequestedEventArgs(
        float offsetVelocity,
        winrt::IReference<float> inertiaDecayRate);

    float OffsetVelocity() const;
    winrt::IReference<float> InertiaDecayRate() const;
    int CorrelationId() const;
    void CorrelationId(int correlationId);

private:
    float m_offsetVelocity{ 0.0f };
    winrt::IReference<float> m_inertiaDecayRate{};
    int m_correlationId{ -1 };
};
