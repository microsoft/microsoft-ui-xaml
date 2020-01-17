// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ScrollingZoomOptions.h"

#include "ScrollingZoomOptions.properties.cpp"

ScrollingZoomOptions::ScrollingZoomOptions(
    winrt::AnimationMode const& animationMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str());

    m_animationMode = animationMode;
}

ScrollingZoomOptions::ScrollingZoomOptions(
    winrt::AnimationMode const& animationMode,
    winrt::SnapPointsMode const& snapPointsMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str(),
        TypeLogging::SnapPointsModeToString(snapPointsMode).c_str());

    m_animationMode = animationMode;
    m_snapPointsMode = snapPointsMode;
}

winrt::AnimationMode ScrollingZoomOptions::AnimationMode() const
{
    return m_animationMode;
}

void ScrollingZoomOptions::AnimationMode(winrt::AnimationMode const& animationMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str());
    m_animationMode = animationMode;
}

winrt::SnapPointsMode ScrollingZoomOptions::SnapPointsMode() const
{
    return m_snapPointsMode;
}

void ScrollingZoomOptions::SnapPointsMode(winrt::SnapPointsMode const& snapPointsMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::SnapPointsModeToString(snapPointsMode).c_str());
    m_snapPointsMode = snapPointsMode;
}

