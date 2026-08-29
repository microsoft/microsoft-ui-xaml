// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingAnchorRequestedEventArgs.g.h"

class ScrollingAnchorRequestedEventArgs :
    public ReferenceTracker<ScrollingAnchorRequestedEventArgs, winrt::implementation::ScrollingAnchorRequestedEventArgsT, winrt::composable, winrt::composing>
{
public:
    ~ScrollingAnchorRequestedEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollingAnchorRequestedEventArgs(const winrt::ScrollPresenter& scrollPresenter);

#pragma region IScrollingAnchorRequestedEventArgs
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
    tracker_ref<winrt::ScrollPresenter> m_scrollPresenter{ this };
};
