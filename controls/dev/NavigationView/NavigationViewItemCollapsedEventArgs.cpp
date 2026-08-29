// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemCollapsedEventArgs.h"

NavigationViewItemCollapsedEventArgs::NavigationViewItemCollapsedEventArgs(const winrt::NavigationView& navigationView)
{
    m_navigationView.set(navigationView);
}

winrt::NavigationViewItemBase NavigationViewItemCollapsedEventArgs::CollapsedItemContainer()
{
    return m_collapsedItemContainer.get();
}

void NavigationViewItemCollapsedEventArgs::CollapsedItemContainer(winrt::NavigationViewItemBase const& value)
{
    m_collapsedItemContainer.set(value);
}

winrt::IInspectable NavigationViewItemCollapsedEventArgs::CollapsedItem()
{
    if (auto const collapsedItem = m_collapsedItem.get())
    {
        return collapsedItem;
    }

    if (auto const nv = m_navigationView.get())
    {
        m_collapsedItem.set(nv.MenuItemFromContainer(m_collapsedItemContainer.get()));
        return m_collapsedItem.get();
    }

    return nullptr;
}
