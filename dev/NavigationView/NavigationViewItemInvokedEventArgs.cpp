// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemInvokedEventArgs.h"
#include "NavigationView.h"

#include "NavigationViewItemInvokedEventArgs.properties.cpp"

winrt::IInspectable NavigationViewItemInvokedEventArgs::InvokedItem()
{
    return m_invokedItem.get();
}

bool NavigationViewItemInvokedEventArgs::IsSettingsInvoked()
{
    return m_isSettingsInvoked;
}

void NavigationViewItemInvokedEventArgs::InvokedItem(const winrt::IInspectable& value)
{
    m_invokedItem.set(value);
}

void NavigationViewItemInvokedEventArgs::IsSettingsInvoked(bool value)
{
    m_isSettingsInvoked = value;
}

winrt::NavigationViewItemBase NavigationViewItemInvokedEventArgs::InvokedItemContainer()
{
    return m_invokedItemContainer.get();
}

void NavigationViewItemInvokedEventArgs::InvokedItemContainer(winrt::NavigationViewItemBase const& value)
{
    m_invokedItemContainer.set(value);
}

winrt::NavigationTransitionInfo NavigationViewItemInvokedEventArgs::RecommendedNavigationTransitionInfo()
{
    return m_recommendedNavigationTransitionInfo.get();
}

void NavigationViewItemInvokedEventArgs::RecommendedNavigationTransitionInfo(winrt::NavigationTransitionInfo const& value)
{
    m_recommendedNavigationTransitionInfo.set(value);
}
