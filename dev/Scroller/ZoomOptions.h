// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"
#include "ZoomOptions.g.h"

class ZoomOptions :
    public winrt::implementation::ZoomOptionsT<ZoomOptions>
{
public:
    explicit ZoomOptions(winrt::AnimationMode const& animationMode);
    explicit ZoomOptions(winrt::AnimationMode const& animationMode, winrt::SnapPointsMode const& snapPointsMode);

    ~ZoomOptions()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    static constexpr winrt::AnimationMode s_defaultAnimationMode{ winrt::AnimationMode::Auto };
    static constexpr winrt::SnapPointsMode s_defaultSnapPointsMode{ winrt::SnapPointsMode::Default };

    [[nodiscard]] winrt::AnimationMode AnimationMode() const;
    void AnimationMode(winrt::AnimationMode const& animationMode);

    [[nodiscard]] winrt::SnapPointsMode SnapPointsMode() const;
    void SnapPointsMode(winrt::SnapPointsMode const& snapPointsMode);

private:
    winrt::AnimationMode m_animationMode{ s_defaultAnimationMode };
    winrt::SnapPointsMode m_snapPointsMode{ s_defaultSnapPointsMode };
};

