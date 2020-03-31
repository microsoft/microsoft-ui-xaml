// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemCollapsedEventArgs.g.h"
#include "NavigationViewItemBase.h"

class NavigationViewItemCollapsedEventArgs :
    public ReferenceTracker<NavigationViewItemCollapsedEventArgs, winrt::implementation::NavigationViewItemCollapsedEventArgsT, winrt::composing, winrt::composable>
{
public:
    winrt::NavigationViewItemBase CollapsedItemContainer();
    void CollapsedItemContainer(winrt::NavigationViewItemBase const& value);

    winrt::IInspectable CollapsedItem();

    void NavigationView(const winrt::NavigationView& navigationView);

private:
    tracker_ref<winrt::NavigationViewItemBase> m_collapsedItemContainer{ this };
    tracker_ref<winrt::NavigationView> m_navigationView{ this };
};
