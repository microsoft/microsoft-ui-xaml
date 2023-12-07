// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "ScrollControllerPanRequestedEventArgs.g.h"
#include "ScrollPresenterTrace.h"

class ScrollControllerPanRequestedEventArgs :
    public winrt::implementation::ScrollControllerPanRequestedEventArgsT<ScrollControllerPanRequestedEventArgs>
{
public:
    ~ScrollControllerPanRequestedEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollControllerPanRequestedEventArgs(
        const winrt::PointerPoint& pointerPoint);

    winrt::PointerPoint PointerPoint() const;
    bool Handled() const;
    void Handled(bool handled); 

private:
    winrt::PointerPoint m_pointerPoint{ nullptr };
    bool m_handled{ false };
};
