// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"
#include "ZoomOptions.g.h"

class ZoomOptions :
    public winrt::implementation::ZoomOptionsT<ZoomOptions>
{
public:
    ~ZoomOptions()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ZoomOptions(winrt::AnimationMode const& animationMode);
    ZoomOptions(winrt::AnimationMode const& animationMode, winrt::SnapPointsMode const& snapPointsMode);

    winrt::AnimationMode AnimationMode();
    void AnimationMode(winrt::AnimationMode const& animationMode);

    winrt::SnapPointsMode SnapPointsMode();
    void SnapPointsMode(winrt::SnapPointsMode const& snapPointsMode);

private:
    winrt::AnimationMode m_animationMode{ winrt::AnimationMode::Enabled };
    winrt::SnapPointsMode m_snapPointsMode{ winrt::SnapPointsMode::Default };
};

