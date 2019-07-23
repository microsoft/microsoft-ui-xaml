// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "NavigationView.h"
#include "NavigationViewSelectionChangedEventArgs.h"
#include "common.h"

winrt::IInspectable NavigationViewSelectionChangedEventArgs::SelectedItem()
{
    return m_selectedItem.get();
}

bool NavigationViewSelectionChangedEventArgs::IsSettingsSelected()
{
    return m_isSettingsSelected;
}

void NavigationViewSelectionChangedEventArgs::SelectedItem(winrt::IInspectable const& value)
{
    m_selectedItem.set(value);
}

void NavigationViewSelectionChangedEventArgs::IsSettingsSelected(bool value)
{
    m_isSettingsSelected = value;
}

winrt::NavigationViewItemBase NavigationViewSelectionChangedEventArgs::SelectedItemContainer()
{
    return m_selectedItemContainer.get();
}

void NavigationViewSelectionChangedEventArgs::SelectedItemContainer(winrt::NavigationViewItemBase const& value)
{
    m_selectedItemContainer.set(value);
}

winrt::NavigationTransitionInfo NavigationViewSelectionChangedEventArgs::RecommendedNavigationTransitionInfo()
{
    return m_recommendedNavigationTransitionInfo.get();
}

void NavigationViewSelectionChangedEventArgs::RecommendedNavigationTransitionInfo(winrt::NavigationTransitionInfo const& value)
{
    m_recommendedNavigationTransitionInfo.set(value);
}
