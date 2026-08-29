// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTrace.h"
#include "ScrollControllerPanRequestedEventArgs.h"

#include "ScrollControllerPanRequestedEventArgs.properties.cpp"

ScrollControllerPanRequestedEventArgs::ScrollControllerPanRequestedEventArgs(
    const winrt::PointerPoint& pointerPoint)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::PointerPointToString(pointerPoint).c_str());

    m_pointerPoint = pointerPoint;
}

winrt::PointerPoint ScrollControllerPanRequestedEventArgs::PointerPoint() const
{
    return m_pointerPoint;
}

bool ScrollControllerPanRequestedEventArgs::Handled() const
{
    return m_handled;
}

void ScrollControllerPanRequestedEventArgs::Handled(bool handled)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, handled);
    m_handled = handled;
}
