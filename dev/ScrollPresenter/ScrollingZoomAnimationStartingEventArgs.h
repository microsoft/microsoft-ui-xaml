// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingZoomAnimationStartingEventArgs.g.h"

class ScrollingZoomAnimationStartingEventArgs :
    public winrt::implementation::ScrollingZoomAnimationStartingEventArgsT<ScrollingZoomAnimationStartingEventArgs>
{
public:
    ScrollingZoomAnimationStartingEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollingZoomAnimationStartingEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IZoomAnimationStartingEventArgs overrides
    winrt::float2 CenterPoint();
    float StartZoomFactor();
    float EndZoomFactor();
    winrt::CompositionAnimation Animation();
    void Animation(winrt::CompositionAnimation const& value);
    winrt::ZoomInfo ZoomInfo();

    void SetZoomFactorChangeId(int32_t zoomFactorChangeId);
    winrt::CompositionAnimation GetAnimation() const;
    void SetAnimation(const winrt::CompositionAnimation& animation);
    void SetCenterPoint(const winrt::float2& centerPoint);
    void SetStartZoomFactor(const float& startZoomFactor);
    void SetEndZoomFactor(const float& endZoomFactor);

private:
    winrt::CompositionAnimation m_animation{ nullptr };
    winrt::float2 m_centerPoint{ };
    float m_startZoomFactor{ 1.0f };
    float m_endZoomFactor{ 1.0f };
    int32_t m_zoomFactorChangeId{ -1 };
};
