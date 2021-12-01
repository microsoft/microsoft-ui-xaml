// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemExpandingEventArgs.g.h"

class NavigationViewItemExpandingEventArgs :
    public ReferenceTracker<NavigationViewItemExpandingEventArgs, winrt::implementation::NavigationViewItemExpandingEventArgsT, winrt::composing, winrt::composable>
{
public:
    NavigationViewItemExpandingEventArgs(const winrt::NavigationView& navigationView);

    winrt::NavigationViewItemBase ExpandingItemContainer();
    void ExpandingItemContainer(winrt::NavigationViewItemBase const& value);

    winrt::IInspectable ExpandingItem();

private:
    tracker_ref<winrt::NavigationViewItemBase> m_expandingItemContainer{ this };
    tracker_ref<winrt::IInspectable> m_expandingItem{ this };
    tracker_ref<winrt::NavigationView> m_navigationView{ this };
};
