// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewExpandingEventArgs.g.h"

class NavigationViewExpandingEventArgs :
    public ReferenceTracker<NavigationViewExpandingEventArgs, winrt::implementation::NavigationViewExpandingEventArgsT, winrt::composing, winrt::composable>
{
public:
    winrt::NavigationViewItemBase ExpandingItemContainer();
    void ExpandingItemContainer(winrt::NavigationViewItemBase const& value);

private:
    tracker_ref<winrt::NavigationViewItemBase> m_expandingItemContainer{ this };
};
