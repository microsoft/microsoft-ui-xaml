// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewSelectionChangedEventArgs.g.h"

class NavigationViewSelectionChangedEventArgs
    : public ReferenceTracker<NavigationViewSelectionChangedEventArgs, winrt::implementation::NavigationViewSelectionChangedEventArgsT, winrt::composing, winrt::composable>
{
public:
    winrt::IInspectable SelectedItem();
    bool IsSettingsSelected();

    void SelectedItem(winrt::IInspectable const& value);
    void IsSettingsSelected(bool value);

    winrt::NavigationViewItemBase SelectedItemContainer();
    void SelectedItemContainer(winrt::NavigationViewItemBase const& value);

    winrt::NavigationTransitionInfo RecommendedNavigationTransitionInfo();
    void RecommendedNavigationTransitionInfo(winrt::NavigationTransitionInfo const& value);

private:
    bool m_isSettingsSelected{};
    tracker_ref<winrt::IInspectable> m_selectedItem{ this };
    tracker_ref<winrt::NavigationViewItemBase> m_selectedItemContainer{ this };
    tracker_ref<winrt::NavigationTransitionInfo> m_recommendedNavigationTransitionInfo{ this };

};
