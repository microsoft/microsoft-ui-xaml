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

void NavigationViewItemBase::Depth(int depth)
{
    m_depth = depth;
    OnNavigationViewItemBaseDepthChanged();
}

int NavigationViewItemBase::Depth()
{
    return m_depth;
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
