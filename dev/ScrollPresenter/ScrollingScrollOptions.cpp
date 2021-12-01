// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ScrollingScrollOptions.h"

#include "ScrollingScrollOptions.properties.cpp"

ScrollingScrollOptions::ScrollingScrollOptions(
    winrt::ScrollingAnimationMode const& animationMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str());

    m_animationMode = animationMode;
}

ScrollingScrollOptions::ScrollingScrollOptions(
    winrt::ScrollingAnimationMode const& animationMode,
    winrt::ScrollingSnapPointsMode const& snapPointsMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str(),
        TypeLogging::SnapPointsModeToString(snapPointsMode).c_str());

    m_animationMode = animationMode;
    m_snapPointsMode = snapPointsMode;
}

winrt::ScrollingAnimationMode ScrollingScrollOptions::AnimationMode() const
{
    return m_animationMode;
}

void ScrollingScrollOptions::AnimationMode(winrt::ScrollingAnimationMode const& animationMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str());

    m_animationMode = animationMode;
}

winrt::ScrollingSnapPointsMode ScrollingScrollOptions::SnapPointsMode() const
{
    return m_snapPointsMode;
}

void ScrollingScrollOptions::SnapPointsMode(winrt::ScrollingSnapPointsMode const& snapPointsMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::SnapPointsModeToString(snapPointsMode).c_str());

    m_snapPointsMode = snapPointsMode;
}
