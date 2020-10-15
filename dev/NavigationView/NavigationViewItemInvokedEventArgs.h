// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemInvokedEventArgs.g.h"

class NavigationViewItemInvokedEventArgs
    : public ReferenceTracker<NavigationViewItemInvokedEventArgs, winrt::implementation::NavigationViewItemInvokedEventArgsT, winrt::composing, winrt::composable>
{
public:
    winrt::IInspectable InvokedItem();
    bool IsSettingsInvoked();

    void InvokedItem(const winrt::IInspectable& value);
    void IsSettingsInvoked(bool value);

    winrt::NavigationViewItemBase InvokedItemContainer();
    void InvokedItemContainer(winrt::NavigationViewItemBase const& value);

    winrt::NavigationTransitionInfo RecommendedNavigationTransitionInfo();
    void RecommendedNavigationTransitionInfo(winrt::NavigationTransitionInfo const& value);

private:
    bool m_isSettingsInvoked{ false };
    tracker_ref<winrt::IInspectable> m_invokedItem{ this };
    tracker_ref<winrt::NavigationViewItemBase> m_invokedItemContainer{ this };
    tracker_ref<winrt::NavigationTransitionInfo> m_recommendedNavigationTransitionInfo{ this };
};

