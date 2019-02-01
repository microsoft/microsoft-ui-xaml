// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewCollapsedEventArgs.g.h"
#include "NavigationViewItemBase.h"

class NavigationViewCollapsedEventArgs :
    public ReferenceTracker<NavigationViewCollapsedEventArgs, winrt::implementation::NavigationViewCollapsedEventArgsT, winrt::composing, winrt::composable>
{
public:
    winrt::NavigationViewItemBase CollapsedItemContainer();
    void CollapsedItemContainer(winrt::NavigationViewItemBase const& value);

private:
    tracker_ref<winrt::NavigationViewItemBase> m_collapsedItemContainer{ this };
};
