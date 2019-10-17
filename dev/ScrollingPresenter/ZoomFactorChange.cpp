// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollerTypeLogging.h"
#include "ZoomFactorChange.h"

ZoomFactorChange::ZoomFactorChange(
    float zoomFactor,
    winrt::IReference<winrt::float2> centerPoint,
    ScrollerViewKind zoomFactorKind,
    winrt::IInspectable const& options) :
        m_zoomFactor(zoomFactor),
        m_centerPoint(centerPoint),
        ViewChange(zoomFactorKind, options)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(), zoomFactor);
}

ZoomFactorChange::~ZoomFactorChange()
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}
