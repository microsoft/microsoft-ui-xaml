// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollerAnchorRequestedEventArgs.g.h"

class ScrollerAnchorRequestedEventArgs :
    public ReferenceTracker<ScrollerAnchorRequestedEventArgs, winrt::implementation::ScrollerAnchorRequestedEventArgsT, winrt::composable, winrt::composing>
{
public:
    ~ScrollerAnchorRequestedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollerAnchorRequestedEventArgs(const winrt::Scroller& scroller);

#pragma region IScrollerAnchorRequestedEventArgs
    winrt::IVector<winrt::UIElement> AnchorCandidates();
    winrt::UIElement AnchorElement();
    void AnchorElement(winrt::UIElement const& value);
#pragma endregion

    winrt::IVector<winrt::UIElement> GetAnchorCandidates();
    void SetAnchorCandidates(const std::vector<tracker_ref<winrt::UIElement>>& anchorCandidates);

    winrt::UIElement GetAnchorElement() const;
    void SetAnchorElement(const winrt::UIElement& anchorElement);

private:
    tracker_ref<winrt::IVector<winrt::UIElement>> m_anchorCandidates{ this };
    tracker_ref<winrt::UIElement> m_anchorElement{ this };
    tracker_ref<winrt::Scroller> m_scroller{ this };
};
