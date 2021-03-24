// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemBase.h"
#include "NavigationView.h"
#include "IndexPath.h"

void NavigationViewItemBase::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    Loaded({ this, &NavigationViewItemBase::OnLoaded });
}

void NavigationViewItemBase::OnLoaded(winrt::IInspectable const&, winrt::RoutedEventArgs const&)
{
    if (!m_navigationView.get())
    {
        m_navigationView = winrt::make_weak(
            [this]()
            {
                auto parent = winrt::VisualTreeHelper::GetParent(*this);
                while (parent)
                {
                    if (auto const parentAsNavigationView = parent.try_as<winrt::NavigationView>())
                    {
                        return parentAsNavigationView;
                    }
                    parent = winrt::VisualTreeHelper::GetParent(parent);
                }
                return static_cast<winrt::NavigationView>(nullptr);
            }());
    }
}

NavigationViewRepeaterPosition NavigationViewItemBase::Position() const
{
    return m_position;
}

void NavigationViewItemBase::Position(NavigationViewRepeaterPosition value)
{
    if (m_position != value)
    {
        m_position = value;
        OnNavigationViewItemBasePositionChanged();
    }
}

winrt::NavigationView NavigationViewItemBase::GetNavigationView() const
{
    return m_navigationView.get();
}

void NavigationViewItemBase::Depth(int depth)
{
    if (m_depth != depth)
    {
        m_depth = depth;
        OnNavigationViewItemBaseDepthChanged();
    }
}

int NavigationViewItemBase::Depth() const
{
    return m_depth;
}

winrt::SplitView NavigationViewItemBase::GetSplitView() const
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

void NavigationViewItemBase::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.Property() == s_IsSelectedProperty)
    {
        OnNavigationViewItemBaseIsSelectedChanged();
    }
}
