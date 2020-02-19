// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemBase.h"
#include "NavigationView.h"
#include "IndexPath.h"

NavigationViewRepeaterPosition NavigationViewItemBase::Position()
{
    return m_position;
}

void NavigationViewItemBase::Position(NavigationViewRepeaterPosition value)
{
    if (m_position != value)
    {
        m_position = value;
        OnNavigationViewRepeaterPositionChanged();
    }
}

winrt::NavigationView NavigationViewItemBase::GetNavigationView()
{
    return m_navigationView.get();
}

winrt::SplitView NavigationViewItemBase::GetSplitView()
{
    winrt::SplitView splitView{ nullptr };
    auto navigationView = GetNavigationView();
    if (navigationView)
    {
        splitView = winrt::get_self<NavigationView>(navigationView)->GetSplitView();
    }
    return splitView;
}

void NavigationViewItemBase::SetNavigationViewParent(winrt::NavigationView const& navigationView)
{
    m_navigationView = winrt::make_weak(navigationView);
}

winrt::NavigationViewItem NavigationViewItemBase::GetParentNavigationViewItem()
{
    // TODO: Change to just search up tree until we find a NavigationViewItem
    if (auto const ir = winrt::VisualTreeHelper::GetParent(*this))
    {
        if (auto const grid = winrt::VisualTreeHelper::GetParent(ir))
        {
            if (auto const nvi = winrt::VisualTreeHelper::GetParent(grid).try_as<winrt::NavigationViewItem>())
            {
                return nvi;
            }
        }
    }
    return nullptr;
}
