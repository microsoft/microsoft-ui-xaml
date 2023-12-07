// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewPaneClosingEventArgs.g.h"

class NavigationViewPaneClosingEventArgs :
    public ReferenceTracker<NavigationViewPaneClosingEventArgs, winrt::implementation::NavigationViewPaneClosingEventArgsT, winrt::composing, winrt::composable>
{
public:
    bool Cancel();
    void Cancel(bool value);

    void SplitViewClosingArgs(winrt::SplitViewPaneClosingEventArgs const& value);

private:
    tracker_ref<winrt::SplitViewPaneClosingEventArgs> m_splitViewClosingArgs{ this };

    bool m_cancelled{};
};
