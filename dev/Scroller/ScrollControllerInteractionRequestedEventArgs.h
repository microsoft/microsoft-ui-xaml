// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollControllerInteractionRequestedEventArgs.g.h"

class ScrollControllerInteractionRequestedEventArgs :
    public winrt::implementation::ScrollControllerInteractionRequestedEventArgsT<ScrollControllerInteractionRequestedEventArgs>
{
public:
    ~ScrollControllerInteractionRequestedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    explicit ScrollControllerInteractionRequestedEventArgs(
        const winrt::PointerPoint& pointerPoint);

    [[nodiscard]] winrt::PointerPoint PointerPoint() const;
    [[nodiscard]] bool Handled() const;
    void Handled(bool handled); 

private:
    winrt::PointerPoint m_pointerPoint{ nullptr };
    bool m_handled{ false };
};
