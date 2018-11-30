// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs.g.h"

class ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs :
    public winrt::implementation::ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgsT<ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs>
{
public:
    ~ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs(
        float additionalVelocity,
        winrt::IReference<float> inertiaDecayRate);

    float AdditionalVelocity();
    winrt::IReference<float> InertiaDecayRate();
    int32_t ViewChangeId();
    void ViewChangeId(int32_t viewChangeId);

private:
    float m_additionalVelocity{ 0.0f };
    winrt::IReference<float> m_inertiaDecayRate{};
    int32_t m_viewChangeId{ -1 };
};
