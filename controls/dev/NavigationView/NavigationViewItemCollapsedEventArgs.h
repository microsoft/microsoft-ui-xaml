// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemCollapsedEventArgs.g.h"

class NavigationViewItemCollapsedEventArgs :
    public ReferenceTracker<NavigationViewItemCollapsedEventArgs, winrt::implementation::NavigationViewItemCollapsedEventArgsT, winrt::composing, winrt::composable>
{
public:
    NavigationViewItemCollapsedEventArgs(const winrt::NavigationView& navigationView);

    winrt::NavigationViewItemBase CollapsedItemContainer();
    void CollapsedItemContainer(winrt::NavigationViewItemBase const& value);

    winrt::IInspectable CollapsedItem();

private:
    tracker_ref<winrt::NavigationViewItemBase> m_collapsedItemContainer{ this };
    tracker_ref<winrt::IInspectable> m_collapsedItem{ this };
    tracker_ref<winrt::NavigationView> m_navigationView{ this };
};
