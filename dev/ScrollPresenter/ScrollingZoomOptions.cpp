// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollerTypeLogging.h"
#include "ZoomOptions.h"

#include "ZoomOptions.properties.cpp"

ZoomOptions::ZoomOptions(
    winrt::AnimationMode const& animationMode)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str());

    m_animationMode = animationMode;
}

ZoomOptions::ZoomOptions(
    winrt::AnimationMode const& animationMode,
    winrt::SnapPointsMode const& snapPointsMode)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str(),
        TypeLogging::SnapPointsModeToString(snapPointsMode).c_str());

    m_animationMode = animationMode;
    m_snapPointsMode = snapPointsMode;
}

winrt::AnimationMode ZoomOptions::AnimationMode() const
{
    return m_animationMode;
}

void ZoomOptions::AnimationMode(winrt::AnimationMode const& animationMode)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str());
    m_animationMode = animationMode;
}

winrt::SnapPointsMode ZoomOptions::SnapPointsMode() const
{
    return m_snapPointsMode;
}

void ZoomOptions::SnapPointsMode(winrt::SnapPointsMode const& snapPointsMode)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::SnapPointsModeToString(snapPointsMode).c_str());
    m_snapPointsMode = snapPointsMode;
}

