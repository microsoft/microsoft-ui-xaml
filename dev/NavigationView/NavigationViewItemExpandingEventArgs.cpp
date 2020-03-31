// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemExpandingEventArgs.h"

winrt::NavigationViewItemBase NavigationViewItemExpandingEventArgs::ExpandingItemContainer()
{
    return m_expandingItemContainer.get();
}

void NavigationViewItemExpandingEventArgs::ExpandingItemContainer(winrt::NavigationViewItemBase const& value)
{
    m_expandingItemContainer.set(value);
}

winrt::IInspectable NavigationViewItemExpandingEventArgs::ExpandingItem()
{
    if (auto const nv = m_navigationView.get())
    {
        return nv.MenuItemFromContainer(m_expandingItemContainer.get());
    }
    return nullptr;
}

void NavigationViewItemExpandingEventArgs::NavigationView(winrt::NavigationView const& navigationView)
{
    m_navigationView.set(navigationView);
}

